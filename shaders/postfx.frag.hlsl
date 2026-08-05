// postfx.frag — "grade": sahne target'ını tam ekran örnekleyip doygunluk +
// sıcaklık + kontrast/parlaklık + retro modlar + vinyet uygular.
// Düzen R2DPostFXU (r2d.c) ile aynı.
//
// Bağlama: fragment doku/sampler (t0/s0, space2) · uniform buffer (b0, space3).

Texture2D<float4> Tex : register(t0, space2);
SamplerState      Smp : register(s0, space2);

#define MAX_DISTORT 8

struct PostU {
    float4 a;   // x=doygunluk y=sıcaklık z=kontrast w=parlaklık
    float4 b;   // x=vinyet gücü y=vinyet yarıçapı z=scanline w=gameboy
    float4 c;   // x=posterize y=kromatik sapma z=gren w=zaman (kare sayacı)
    float4 d;   // x=bloom gücü y=bloom eşiği z=bloom yarıçapı w=ısı genliği
    float4 e;   // x=ısı hızı y,z,w=(boş)
    float4 f;   // x=aktif bükülme sayısı
    float4 src[MAX_DISTORT];    // x,y=ekran px · z=yarıçap · w=genlik
    float4 meta[MAX_DISTORT];   // x=stil · y=faz · z=frekans · w=aniso
};

// BÜKÜLME STİLLERİ (r2d.h R2DDistortStyle ile BİREBİR)
#define DS_HEAT  0
#define DS_SHOCK 1
#define DS_SWIRL 2
#define DS_LENS  3
// Struct'ı değişkene sarmak MSL'deki `u.` erişimini birebir korur; HLSL'de
// çıplak cbuffer üyeleri global kapsama açılıp yerel `c`/`d` ile çakışırdı.
cbuffer PostUB : register(b0, space3) {
    PostU u;
};

// Bloom halka yönleri ve GameBoy paleti: MSL'de fonksiyon içi yerel dizilerdi,
// HLSL'de dosya kapsamında `static const` en güvenli biçim.
static const float2 BLOOM_DIRS[6] = {
    float2( 1.0,  0.0),   float2( 0.5,  0.866), float2(-0.5,  0.866),
    float2(-1.0,  0.0),   float2(-0.5, -0.866), float2( 0.5, -0.866)
};
static const float3 DMG_PAL[4] = {
    float3(0.06, 0.22, 0.06), float3(0.19, 0.38, 0.19),
    float3(0.55, 0.67, 0.06), float3(0.61, 0.74, 0.06)
};

struct PSIn {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

// NOKTASAL BÜKÜLME ALANLARI — genel motor katmanı (tek efektin parçası değil).
// Isı, şok dalgası, burgaç ve mercek AYNI matematikten türüyor; fark yalnız
// ofsetin yönü ve sönüm eğrisi. Kaynak listesi kare başına çağırandan gelir
// (ışık listesiyle aynı desen), motor durum tutmaz.
float2 distort_offset(float2 pos, float t)
{
    float2 off = float2(0.0, 0.0);
    int n = (int)(u.f.x + 0.5);
    [loop] for (int k = 0; k < n && k < MAX_DISTORT; k++) {
        float2 c = u.src[k].xy;
        float  R = u.src[k].z;
        float  amp = u.src[k].w;
        float  style = u.meta[k].x;
        float  phase = u.meta[k].y;
        float  freq = u.meta[k].z;
        float  aniso = u.meta[k].w;

        // Anizotropi: 3/4 kamerada zemin efektleri YATAY basık okunur
        float2 d = pos - c;
        d.y /= max(aniso, 0.01);
        float r = length(d);
        if (r > R) continue;
        float fall = 1.0 - r / R;             // 1 merkez → 0 kenar
        fall = fall * fall;                    // yumuşak sönüm
        float2 dir = (r > 0.001) ? d / r : float2(0.0, -1.0);

        if (style < DS_HEAT + 0.5) {
            // ISI: dikey titreme, kaynağın ÜSTÜNDE güçlenir (sıcak hava kalkar).
            // İki frekans üst üste — tek sinüs akordeon gibi duruyor.
            float up = saturate((c.y - pos.y) / R);        // 0 altta → 1 tepede
            float w1 = sin(pos.y * 0.09 * freq + t * 3.1 + phase * 6.2831) * 0.62;
            float w2 = sin(pos.y * 0.21 * freq - t * 5.3 + phase * 3.1) * 0.38;
            off.x += (w1 + w2) * amp * fall * (0.25 + 0.75 * up);
            off.y += sin(pos.x * 0.13 * freq + t * 2.2) * amp * 0.35 * fall * up;
        } else if (style < DS_SHOCK + 0.5) {
            // ŞOK: merkezden dışa yürüyen HALKA. phase halkanın yarıçap oranı
            // (0 merkez → 1 kenar); çağıran onu zamanla artırır. Halkanın
            // kalınlığı dışa doğru artıyor ve genliği düşüyor (enerji dağılır).
            float ring = phase * R;
            float w = R * 0.10 + ring * 0.06;
            float band = 1.0 - saturate(abs(r - ring) / max(w, 1.0));
            band = band * band;
            off += dir * (amp * band * (1.0 - phase));
        } else if (style < DS_SWIRL + 0.5) {
            // BURGAÇ: teğetsel kaydırma — merkeze yakın daha çok döner.
            float2 tang = float2(-dir.y, dir.x);
            off += tang * (amp * fall * (0.6 + 0.4 * sin(t * 2.0 + phase * 6.2831)));
        } else {
            // MERCEK: radyal şişme. amp>0 dışa (kabarık), amp<0 içe (çukur).
            off += dir * (amp * fall * (1.0 - fall) * 4.0);
        }
    }
    return off;
}

float4 main(PSIn i) : SV_Target0
{
    // TAM EKRAN ısı bükülmesi (eski, global topuz — sahne geneli için korunuyor)
    float2 uv = i.uv;
    if (u.d.w > 0.0) {
        float tt = u.c.w * u.e.x * 0.06;
        float depth = uv.y;                 // 0 üst, 1 alt
        float w1 = sin(uv.y * 38.0 + tt * 3.1) * 0.6;
        float w2 = sin(uv.y * 91.0 - tt * 5.7) * 0.4;
        uv.x += (w1 + w2) * u.d.w * depth;
        uv.y += sin(uv.x * 55.0 + tt * 2.3) * u.d.w * 0.35 * depth;
    }
    // NOKTASAL BÜKÜLME ALANLARI: global ısıdan SONRA, örneklemeden ÖNCE.
    // Ofset ekran px cinsinden hesaplanıp uv'ye çevriliyor.
    if (u.f.x > 0.5) {
        float2 px = distort_offset(i.pos.xy, u.c.w * 0.0166667);
        // Ekran boyutu: uv ile pos'un oranından türetiliyor (ek uniform yok)
        float2 dim = (i.uv.x > 0.0001 && i.uv.y > 0.0001)
                   ? i.pos.xy / i.uv : float2(1920.0, 1080.0);
        uv += px / max(dim, float2(1.0, 1.0));
    }

    // Kromatik sapma: R sola, B sağa kaymış örneklenir (VHS/retro kenar taşması)
    float3 c;
    if (u.c.y > 0.0) {
        c.r = Tex.Sample(Smp, uv - float2(u.c.y, 0.0)).r;
        c.g = Tex.Sample(Smp, uv).g;
        c.b = Tex.Sample(Smp, uv + float2(u.c.y, 0.0)).b;
    } else {
        c = Tex.Sample(Smp, uv).rgb;
    }
    // BLOOM: iki halka × 6 tap = 12 örnek. Eşiğin üstündeki fazla toplanıp eklenir.
    // Halkalar farklı yarıçapta (r ve 2r) — tek halka yıldız deseni yapıyor.
    if (u.d.x > 0.0) {
        float3 bl = float3(0.0, 0.0, 0.0);
        for (int k = 0; k < 6; k++) {
            float2 o = BLOOM_DIRS[k] * u.d.z;
            float3 s1 = Tex.Sample(Smp, uv + o).rgb;
            float3 s2 = Tex.Sample(Smp, uv + o * 2.0).rgb;
            float l1 = dot(s1, float3(0.299, 0.587, 0.114));
            float l2 = dot(s2, float3(0.299, 0.587, 0.114));
            // Parlak-geçiş: eşiğin altındaki hiç katkı vermez (max ile kesiliyor).
            // Renk korunur: fazlayı luma oranıyla ölçekliyoruz, böylece kırmızı alev
            // kırmızı taşıyor — beyaza doymuyor.
            bl += s1 * max(l1 - u.d.y, 0.0);
            bl += s2 * max(l2 - u.d.y, 0.0) * 0.5;   // dış halka yarım ağırlık
        }
        c += bl * (u.d.x / 9.0);   // 6*(1.0+0.5) = 9 normalizasyon
    }
    float luma = dot(c, float3(0.299, 0.587, 0.114));
    c = lerp(float3(luma, luma, luma), c, u.a.x);
    c += float3(u.a.y, 0.0, -u.a.y);
    c = (c - 0.5) * u.a.z + 0.5 + u.a.w;
    // Posterize: kanal başına N kademe (8-bit görünüm)
    if (u.c.x > 1.5) {
        float n = u.c.x - 1.0;
        c = round(clamp(c, 0.0, 1.0) * n) / n;
    }
    // GameBoy modu: parlaklığı 4 kademeye kuantala, DMG yeşil paletine bas
    if (u.b.w > 0.5) {
        float l = dot(clamp(c, 0.0, 1.0), float3(0.299, 0.587, 0.114));
        c = DMG_PAL[clamp(int(l * 4.0), 0, 3)];
    }
    // CRT tarama çizgisi: her 3 piksel satırının biri koyulaşır
    if (u.b.z > 0.0 && frac(i.pos.y / 3.0) < 0.334) c *= 1.0 - u.b.z;
    // Film greni: kare sayacıyla kayan hash gürültüsü (animasyonlu kumlanma)
    if (u.c.z > 0.0) {
        float gn = frac(sin(dot(i.pos.xy + u.c.w,
                               float2(12.9898, 78.233))) * 43758.5453);
        c += (gn - 0.5) * u.c.z;
    }
    // Vinyet: uv merkez uzaklığı 0 (orta) → 1 (köşe); radius'tan köşeye yumuşak iner
    float vd = length(i.uv - 0.5) * 1.41421356;
    c *= 1.0 - u.b.x * smoothstep(u.b.y, 1.0, vd);
    return float4(clamp(c, 0.0, 1.0), 1.0);
}
