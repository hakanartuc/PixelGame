// light.frag — ışık pass'i: piksel başına ambient + Σ ışık katkısı.
// Multiply blend ile sahne rengiyle çarpılır (pipeline r2d.c'de kurulur).
// Dizi boyutları R2D_MAX_LIGHTS / R2D_MAX_OCCLUDERS (r2d.h) ile AYNI olmak zorunda.
// Uniform düzeni R2DLightU (r2d.c) ile birebir aynı — hepsi float4, 16 bayt stride.
//
// Bağlama: fragment uniform buffer → (b0, space3); doku YOK.

struct LightU {
    float4 ambient;      // rgb = ortam çarpanı, w = kamera ofset X
    float4 count;        // x=ışık sayısı y=kuantize z=ızgara w=kamera ofset Y
    float4 mode;         // x=gamma y=düşüş z=dither w=düşüş parametresi
    float4 mode2;        // x=A/B bölme y=anizotropi z=dikey ofset w=tıkama
    float4 pos[32];      // xy = ekran px, z = yarıçap px, w = dikey ofset
    float4 col[32];      // rgb = renk, w = yoğunluk
    float4 shape[32];    // x = siluet genliği, yz = dönen faz vektörü, w = anizotropi
    float4 occ[16];      // ışığı kesen dikdörtgenler: xy=köşe, zw=boyut
};
// DİKKAT: cbuffer üyeleri HLSL'de GLOBAL kapsama açılır. Struct'ı bir değişkenin
// içine sarmak MSL'deki `lu.` erişimini birebir korur ve yerel değişkenlerle
// (c, d, r...) isim çakışmasını engeller.
cbuffer LightUB : register(b0, space3) {
    LightU lu;
};

// Bayer 4×4 ordered dither matrisi. MSL'de fonksiyon içinde yerel diziydi;
// HLSL'de dosya kapsamında `static const` en güvenli biçim (DXC ve glslang
// ikisi de dinamik indekslemeyi burada sorunsuz çözüyor).
static const float BAYER4[16] = {
     0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
     3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
};

// Segment (pa→pb) bir AABB'yi kesiyor mu — slab testi, dallanmasız.
// Işık tıkama için: pikselden ışığa giden doğru bir gövdeden geçiyorsa
// o piksel gölgede. Ray-box ucuz (birkaç karşılaştırma), doku okuması yok.
bool seg_hits_box(float2 pa, float2 pb, float4 b)
{
    float2 d = pb - pa;
    float2 ad = abs(d);
    if (ad.x < 1e-4) d.x = (d.x < 0.0 ? -1e-4 : 1e-4);
    if (ad.y < 1e-4) d.y = (d.y < 0.0 ? -1e-4 : 1e-4);
    float2 t0 = (b.xy - pa) / d;
    float2 t1 = (b.xy + b.zw - pa) / d;
    float2 tmin = min(t0, t1), tmax = max(t0, t1);
    float tn = max(tmin.x, tmin.y);
    float tf = min(tmax.x, tmax.y);
    return (tf >= tn) && (tn < 1.0) && (tf > 0.0);
}

// sRGB ⇄ lineer (IEC 61966-2-1). Işık katkıları FİZİKSEL olarak lineer
// uzayda toplanır; sRGB kodunda toplamak merkezi +0.22 fazla parlatıp
// kırpıyor (düz beyaz plato) ve dış halkayı 0.03 karartıyor. Yani ışık
// "kubbe" değil "kenarı sıkıştırılmış patlamış disk" gibi okunuyordu.
// Not: çarpma gama ile komütatiftir, o yüzden multiply kompozisyonu doğru;
// hata YALNIZCA toplama satırındaydı.
//
// TAŞIMA NOTU — `select()` TUZAĞI:
// MSL sürümü `select(falseVal, trueVal, cond)` kullanıyordu (yanlış-değer ÖNCE).
// HLSL 2021'in `select(cond, trueVal, falseVal)` imzası ise TERS sıralı. İki dil
// arasında kopyalanan bir `select()` çağrısı DERLEME HATASI VERMEZ — sessizce
// sRGB ile lineer kolları takas eder, yani ışık rengi yanlış çıkar ve sebebi
// bulunamaz. Bu yüzden `select()` bilerek KULLANILMIYOR: kol seçimi skaler
// yardımcıda açık `?:` ile yazılıyor. Sıra tuzağı yok, okunurluk yerinde,
// ve MSL'in bileşen-bazlı (bool3) davranışı bileşen bileşen çağrıyla korunuyor.
float s2l1(float c) { return (c <= 0.04045) ? (c / 12.92) : pow((c + 0.055) / 1.055, 2.4); }
float l2s1(float c) { return (c <= 0.0031308) ? (12.92 * c) : (1.055 * pow(c, 1.0 / 2.4) - 0.055); }
float3 s2l(float3 c) { return float3(s2l1(c.r), s2l1(c.g), s2l1(c.b)); }
float3 l2s(float3 c) { return float3(l2s1(c.r), l2s1(c.g), l2s1(c.b)); }

struct PSIn {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

float4 main(PSIn i) : SV_Target0
{
    int   n      = int(lu.count.x);
    float qsteps = lu.count.y;
    float grid   = lu.count.z;
    int   gmode  = int(lu.mode.x);   // 0=sRGB toplama 1=lineer 2=lineer+ACES
    int   fmode  = int(lu.mode.y);   // 0=plato+ss 1=Karis 2=URP pow 3=knee
    int   dmode  = int(lu.mode.z);   // 0=IGN 1=Bayer 2=dünya-IGN 3=yok
    float fparam = lu.mode.w;
    // A/B KARŞILAŞTIRMA: bölme çizgisinin SOLU daima orijinal ayarlar, sağı
    // seçili modlar. Tek ekranda yan yana fark görülür — varyantların çoğu
    // tek başına bakıldığında ayırt edilemiyor.
    float splitX = lu.mode2.x;
    bool  isLeft = (splitX > 0.5 && i.pos.x < splitX);
    if (isLeft) { gmode = 0; fmode = 0; dmode = 0; fparam = 0.0; }
    bool  lin    = (gmode > 0);
    float3 c = lin ? s2l(lu.ambient.rgb) : lu.ambient.rgb;
    // Kuantize modda ışığı SANAT pikselinin merkezinden örnekle
    float2 sp = i.pos.xy;
    if (grid > 0.5) sp = floor(sp / grid) * grid + grid * 0.5;
    for (int li = 0; li < n; li++) {
        // EĞİK PERSPEKTİF DÜZELTMESİ. Kamera tam kuş bakışı DEĞİL (evler yandan
        // görünüyor, çatıları var) — zemin dikey olarak kısaltılmış çiziliyor.
        // Yani zeminde dairesel olan aydınlık alan ekranda YATAY ELİPS olmalı.
        // aniso < 1 → dikey yönde ışık daha çabuk biter (basık elips)
        // aniso > 1 → dikey uzar (alevin yukarı yayılması)
        // yOff  < 0 → ışık merkezi yukarı kayar (kaynak zeminde değil, hazne/alev
        //             yüksekliğinde) — 3/4 perspektifte yükseklik ekranda yukarıdır
        float2 v = sp - lu.pos[li].xy;
        // Şekil ışık BAŞINA (pos.w = dikey ofset, shape.w = anizotropi);
        // mode2.y/z global çarpan-ekleyen (dev tuşu ile test için).
        v.y -= lu.pos[li].w + lu.mode2.z;
        float aniso = max(lu.shape[li].w * lu.mode2.y, 0.05);
        v.y /= aniso;
        float d = length(v);
        float r = lu.pos[li].z;
        float amp = lu.shape[li].x;
        if (d >= r * (1.0 + amp)) continue;
        // SİLUET: halkanın kenarı kusursuz daire olmasın. Açıya bağlı loblar,
        // ama atan2 YOK — çift/üçlü açı özdeşlikleriyle (nx²−ny² = cos2a vb.).
        // Faz vektörü CPU'dan gelir, yani per-pixel transcendental hiç yok.
        if (amp > 0.0) {
            float2 nv = v / max(d, 0.001);
            float c2 = nv.x * nv.x - nv.y * nv.y;
            float s2 = 2.0 * nv.x * nv.y;
            float c3 = nv.x * (2.0 * c2 - 1.0);
            float s3 = nv.y * (2.0 * c2 + 1.0);
            float cp = lu.shape[li].y, sp2 = lu.shape[li].z;
            float wob = (c2 * cp + s2 * sp2) * 0.6
                      + (c3 * cp - s3 * sp2) * 0.4;
            r *= 1.0 + amp * wob;
        }
        if (d >= r) continue;
        float att;
        if (fmode == 1) {
            // KARIS pencereli ters-kare (UE4 SIGGRAPH 2013, dnk.9; Frostbite dnk.26).
            // Referans uzunluk şart: piksel biriminde +1 hiç iş görmez.
            float u = d / fparam;
            float w = saturate(1.0 - pow(d / r, 4.0));
            att = (w * w) / (u * u + 1.0);
        } else if (fmode == 2) {
            // URP 2D Light2D ailesi: att = (1-t)^k, k=1 tam lineer
            att = pow(1.0 - d / r, fparam);
        } else if (fmode == 3) {
            // KENAR DİZİ: iç bölge smoothstep, dış %25'te ani çöküş → sınır okunur
            float b = 1.0 - d / r;
            float a = b * b * (3.0 - 2.0 * b);
            float w = saturate(b / 0.25);
            att = a * w * w;
        } else {
            // MEVCUT: iç %35 plato → smoothstep
            float t = min((1.0 - d / r) / 0.65, 1.0);
            att = t * t * (3.0 - 2.0 * t);
        }
        // KESİN KENAR — HER MODA. light.h light_atten() ile aynı eşik (0.06).
        // Yalnız mod 0'da olması bastırma yarıçapını moda göre kaydırıyordu.
        if (att < 0.06) continue;
        // ── IŞIK TIKAMA ── pikselden ışığa giden doğru bir gövdeden geçiyorsa
        // bu piksel gölgede. Bu olmadan ışık duvarların içinden geçiyordu ve her
        // halka kusursuz bir oval oluyordu. Tam karanlık değil: gölgeye ışık sızar
        // (yansıma yaklaşımı), yoksa siyah kamalar oyuncuyu kör eder.
        float occN = lu.mode2.w;
        if (occN > 0.5 && !isLeft) {
            int on = int(occN);
            for (int k = 0; k < on; k++) {
                if (seg_hits_box(i.pos.xy, lu.pos[li].xy, lu.occ[k])) {
                    att *= 0.22;
                    break;
                }
            }
        }
        float3 lc = lin ? s2l(lu.col[li].rgb) : lu.col[li].rgb;
        c += lc * (att * lu.col[li].w);
    }
    // ACES filmic (Narkowicz 2016). DİKKAT: çıktı clamp'li ve blend çarpımsal
    // olduğu için bu mod 1.0 tavanını AÇMIYOR — ACES(1.0)=0.804, ACES(2.0)=0.915,
    // yani tepeyi ~%20 karartıp yumuşak bir omuz veriyor. Gerçek overbright için
    // RGBA16F hedef gerekir (ayrı iş). Lineer girdi ister.
    if (gmode == 2)
        c = clamp((c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14),
                  0.0, 1.0);
    if (lin) c = l2s(c);
    // KUANTİZE — l2s SONRASINDA olmak zorunda. sRGB kodlaması kendisi kabaca
    // algısal olarak düzgün olduğu için eşit aralıklı kuantizasyon burada
    // algısal olarak da doğru; lineer uzayda kuantalarsak bantlar tepe ışıkta
    // yığılır, gölgede 1-2 kademeye düşer.
    if (qsteps > 0.5) {
        c = floor(c * qsteps + 0.5) / qsteps;
        return float4(clamp(c, 0.0, 1.0), 1.0);
    }
    // DITHER — 8-bit hedefte banding kırar
    if (dmode != 3) {
        float dn;
        if (dmode == 1) {
            // Bayer 4×4 ordered: 8/16-bit döneminin görsel dili, kasıtlı desen
            int2 bi = int2(i.pos.xy) & 3;
            dn = BAYER4[bi.y * 4 + bi.x] / 16.0;
        } else {
            // IGN (Jimenez, CoD:AW 2014). dmode==2: indeks DÜNYA koordinatından —
            // ekran uzayı deseni kamera hareketinde "crawl" ediyor ("kirli cam").
            float2 q = i.pos.xy;
            if (dmode == 2) q = floor((q + float2(lu.ambient.w,
                                                  lu.count.w)) / 8.0);
            dn = frac(52.9829189 * frac(dot(q, float2(0.06711056,
                                                      0.00583715))));
        }
        c += (dn - 0.5) * (2.0 / 255.0);
    }
    if (splitX > 0.5 && abs(i.pos.x - splitX) < 1.5)
        c = float3(1.0, 0.85, 0.2);   // bölme çizgisi
    return float4(clamp(c, 0.0, 1.0), 1.0);
}
