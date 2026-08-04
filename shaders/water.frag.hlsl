// water.frag — SU HACMİ. Ekranın tamamını yazar: karada zemini olduğu gibi
// geçirir, suda ise ALTINDAKİ zemini kendi dalga normaline göre kaydırarak
// örnekler. Bütün mesele bu: su altındaki şeyi ÖRTMÜYOR, BÜKÜYOR. Sualtı
// hissi tanıdık bir nesnenin büküldüğünü görmekten doğar — örtülen kaya
// sualtı değil, mavi levhadır.
//
// Tek geçişte gelen katmanlar: kırılma · derinlik gradyanı · güneş açısına göre
// parlama · kostik ışık ağı · kıyıya vurup çekilen köpük · SU ÇİZGİSİ (ıslak
// bant + koyu hat + menisküs).
//
// Bağlama: t0/s0 = zemin target'ı (NEAREST, pixel-art) · t1/s1 = derinlik
// maskesi (LINEAR — karo başına 2×2 teksel, yumuşak geçiş ONDAN geliyor) ·
// uniform buffer b0, space3. Düzen R2DWaterU (r2d.c) ile birebir.
//
// UYARI: select() KULLANMA. HLSL 2021'de select(cond,a,b), MSL'de
// select(a,b,cond) — argüman sırası TERS ve derleyici uyarı vermez. Hep ?: yaz.
// UYARI: frac(sin(dot(...))) hash KULLANMA — üç arka uçta farklı sonuç veriyor.

Texture2D<float4> Floor : register(t0, space2);
SamplerState      FloorS : register(s0, space2);
Texture2D<float4> Mask  : register(t1, space2);
SamplerState      MaskS : register(s1, space2);

struct WaterU {
    float4 a;   // x=zaman(sn) y=zoom z=ekranW w=ekranH
    float4 b;   // x=camX y=camY z=maske genişlik(TEKSEL) w=maske yükseklik(TEKSEL)
    float4 c;   // xy=güneş yönü (birim) z=güneş şiddeti w=sanat pikseli (ekran px)
    float4 d;   // rgb=sığ su rengi w=sığ örtme
    float4 e;   // rgb=derin su rengi w=derin örtme
    float4 f;   // x=dalga genliği y=kırılma gücü z=kostik gücü w=köpük gücü
    float4 g;   // x=parlama gücü y=renk kademesi z=köpük hızı w=köpük genliği
    float4 h;   // rgb=güneş rengi w=karo boyu (dünya px)
    // 'i' ve 'j' bilinçli atlandı: main(PSIn i) parametresiyle karıştırılmasın.
    float4 k;   // x=ıslak bant y=koyu hat z=menisküs w=katman bit maskesi
    float4 l;   // x=kıyı gölgesi gücü y=ışıltı çizgisi gücü z=ark sırası gücü w=duvar anahtarı
    float4 m;   // xy=lamba1 dünya konumu zw=lamba2 dünya konumu
    float4 n;   // x=lamba1 şiddet y=lamba2 şiddet z=karanlık(0..1) w=yağmur(0..1)
};
cbuffer WaterUB : register(b0, space3) {
    WaterU u;
};

// KATMAN BİT MASKESİ (u.k.w). r2d.h R2D_WATER_L_* ile BİREBİR aynı olmak
// zorunda — Ctrl+B izole modu bu bitleri sürüyor.
static const uint LB_REFRACT = 1u;    // kırılma (sualtı bükülmesi)
static const uint LB_DEPTH   = 2u;    // derinlik örtmesi / su rengi
static const uint LB_CAUSTIC = 4u;    // kostik ışık ağı
static const uint LB_GLINT   = 8u;    // güneş parlaması
static const uint LB_FOAM    = 16u;   // kıyı köpüğü
static const uint LB_WET     = 32u;   // ıslak bant (kıyının kara tarafı)
static const uint LB_DARK    = 64u;   // koyu hat (suyun silueti)
static const uint LB_MENI    = 128u;  // menisküs (parlak kıyı çizgisi)
static const uint LB_BANK    = 256u;  // kıyı gölgesi (kuzey kıyının çukura düşen gölgesi)
static const uint LB_DASH    = 512u;  // ışıltı çizgicikleri (seyrek, ayrık, sürüklenen)
static const uint LB_ARC     = 1024u; // dalga-yankı arkları (kıyıya paralel koyu sıralar)
static const uint LB_RING    = 2048u; // halka dalgacık (balık/damla izi)
static const uint LB_LAMP    = 4096u; // gece lamba yansıması (titrek sıcak sütun)

bool layer_on(uint mask, uint bit) { return (mask & bit) != 0u; }

// TAMSAYI hash — frac(sin(dot(...))) BİLEREK kullanılmıyor: kayan nokta hash'i
// üç arka uçta (MSL/SPIR-V/DXIL) aynı sonucu vermiyor; 1e-7'lik fark 43758 ile
// çarpılıp frac'lanınca bambaşka sayı oluyor. Bit işlemleri her yerde birebir.
uint uhash2(uint a, uint b)
{
    uint h = a * 0x9E3779B9u ^ b * 0x85EBCA6Bu;
    h ^= h >> 15; h *= 0x2545F491u; h ^= h >> 13;
    return h;
}

// SANAT IZGARASI ÇİVİSİ — pixel-art disiplininin temeli. Dünya konumu sanat
// pikselinin MERKEZİNE yuvarlanır; bundan türeyen her alan (dalga, kostik,
// bantlar) sanat pikseli içinde SABİT kalır. Bu olmadan prosedürel alanlar
// ekran çözünürlüğünde pürüzsüz eğri çizer ve "photoshop degrade" görünür —
// zemin dokusu 16 px'ken suyun 1080p eğrilerle akması iki ayrı dünyadır.
// Dünya uzayında çivili: kamera kesirli konumdayken desen crawl etmez.
float2 snap_art(float2 p, float a) { return floor(p / a) * a + a * 0.5; }

// BAYER 4x4 — pixel-art degrade disiplini: kademeler arası sınır 2-3 px'lik
// DAMA deseniyle geçer (elle taranmış görünüm), bant içleri düz kalır.
// floor(v*n + bayer) biçiminde KADEME SEÇİMİNİN İÇİNE gömülür — discard yok,
// ek geçiş yok. Dünya-çivili: kamera kayarken desen kaymaz (IGN dersi).
static const float B4[16] = { 0, 8, 2,10, 12, 4,14, 6, 3,11, 1, 9, 15, 7,13, 5 };
float bayer4(float2 world, float a)
{
    int2 g = int2(floor(world / a));
    return (B4[(g.x & 3) + ((g.y & 3) << 2)] + 0.5) / 16.0;
}

struct PSIn {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

// SU ÇİZGİSİ ÖLÇEĞİ — maskenin G kanalının biriminden TÜRETİLDİ. Bir kez burada,
// bütün bantlar bundan besleniyor (SLOPE_NORM dersinin aynısı: sihirli sayı tek
// yerde durur).
//
// TÜREME (çözünürlük 2 teksel/karo):
//   G = kıyı uzaklığı / WMASK_SHORE_HT, birim YARIM KARO, WMASK_SHORE_HT = 6.
//   Bir yarım karo = TILE_SIZE/2 = 32 dünya px = 8 sanat px (1 sanat px = 4
//   dünya px, çünkü karo dokusu 16 px ve karo 64 dünya px).
//   → 1 sanat px = (1/6)/8 = 1/48 G birimi.
//   Kara tekselinin merkezi tileP = x0-0.25'te ve G=0; ilk su tekselinin merkezi
//   tileP = x0+0.25'te ve G=1/6. LINEAR örnekleyici ikisinin ORTASINDA, yani
//   tileP = x0'da — TAM KARO SINIRINDA, yani görsel su çizgisinde — G=1/12
//   veriyor. Demek ki su çizgisi 4 sanat px'lik rampa değerine oturuyor.
//   → SHORE_LINE = 1/12 = 4 * SHORE_PX.
//
// ESKİ ÇÖZÜNÜRLÜKTE (1 teksel/karo) bu sabit 1/6 sanılıyordu ama muv hesabı
// teksel merkezini karo ORTASINA değil karo SOL/ÜST KENARINA düşürüyordu; su
// çizgisinde G gerçekte 1/3 çıkıyordu ve "1/6'nın üstünde açılır" diye konan
// eşikler (kostik 0.10..0.20) aslında SUYUN DIŞINDA, çimenin üstünde kalıyordu.
// 2 teksel/karo + aşağıdaki muv düzeltmesi ikisini birlikte kapatıyor.
// WMASK_SHORE_HT = 24 (tilemap.c) → 1 sanat px = 1/(8*24) = 1/192,
// su çizgisi = 1/(2*24) = 1/48 = 4 sanat px. HT değişirse İKİSİ de değişir.
static const float SHORE_PX   = 1.0 / 192.0;  // 1 sanat pikseli, G birimi
static const float SHORE_LINE = 4.0 / 192.0;  // görsel su çizgisi (= 1/48)

// Bant konumları — sanat pikseli cinsinden, su çizgisinden İÇERİ (su tarafı).
// SIRA ALttP'DEN ÖLÇÜLDÜ ve İLK KURGUMUZUN TERSİ: parlak menisküs KARAYA
// YAPIŞIK (kontura bitişik 1-2 px), koyu bant ondan SONRA ve GENİŞ (7-10 px
// navy gölge bandı — 'hat' değil). Biz önce koyuyu koymuştuk; Lake Hylia'nın
// üç kenar profili de tersini söylüyor.
static const float MENI_C = 0.8, MENI_H = 0.6;   // 0.2..1.4 px, karaya yapışık
static const float DARK_C = 3.2, DARK_H = 1.8;   // 1.4..5.0 px, geniş navy bant
// Islak bant su çizgisinden KARA tarafına doğru. Tavan 4 sanat px: maskenin kara
// tarafındaki rampası TAM olarak yarım teksel = 4 sanat px sürüyor, ondan öteye
// G sabit 0 ve "karaya ne kadar girdim" bilgisi YOK. Brief 5 px istiyordu; 5
// yazınca shore→0 noktasında bant birden 0.5 kademesine atlıyor ve çimenin
// ortasında halka çıkıyor. 5 px için maskenin G kanalının İŞARETLİ (kara tarafı
// negatif) olması gerekir — o daha büyük bir değişiklik, bugünkü işin dışında.
static const float WET_WIDE = 4.0;

// DALGA ALANI. Üç yönlü sinüs; ikisi ters yönde akıyor (girişim → durgun göl
// dokusu, tek yönlü akıntı değil). Analitik türev döndürüyor: sonlu fark üç
// örnekleme demekti, türev bedava. Dönüş: x=yükseklik y=d/dx z=d/dy
static const float2 WDIR0 = float2( 0.9239,  0.3827);   // 22.5°
static const float2 WDIR1 = float2(-0.3827,  0.9239);   // 112.5°
static const float2 WDIR2 = float2( 0.7071, -0.7071);   // -45°

float3 wave_field(float2 p, float t)
{
    // Dalga sayıları dünya pikseli başına radyan. 0.021 ≈ 300 px dalga boyu
    // (uzun sırt), 0.058 ≈ 108 px, 0.140 ≈ 45 px (kırışık). Üç ölçek üst üste
    // olmadan su ya kauçuk ya da parazit gibi duruyor.
    const float K0 = 0.021, K1 = 0.058, K2 = 0.140;
    const float S0 = 0.55,  S1 = 1.30,  S2 = 2.60;   // rad/sn
    const float W0 = 0.52,  W1 = 0.31,  W2 = 0.17;   // toplam 1 → genlik tavanı tam

    float p0 = dot(p, WDIR0) * K0 + t * S0;
    float p1 = dot(p, WDIR1) * K1 - t * S1;
    float p2 = dot(p, WDIR2) * K2 + t * S2;

    float h  = sin(p0) * W0 + sin(p1) * W1 + sin(p2) * W2;
    float2 g = cos(p0) * W0 * K0 * WDIR0
             + cos(p1) * W1 * K1 * WDIR1
             + cos(p2) * W2 * K2 * WDIR2;
    return float3(h, g.x, g.y);
}

// KOSTİK — güneşin dalga yüzeyinden kırılıp tabanda çizdiği ışık ağı. Üç
// sinüsün mutlak değeri sıfıra yakınken parlıyor (ridged): yumuşak lekeler
// değil, ince ağ istiyoruz. Sığda görünür, derinde kaybolur.
float caustic(float2 p, float t)
{
    // Ölçek: 0.11 rad/px ≈ 57 px dalga boyu ≈ bir karonun altı. İlk denemede
    // 0.045 (≈140 px) yazıldı ve ağ değil BULUT gibi durdu — kostik ince
    // olmalı, iri olursa su değil mermer damarı görünüyor.
    const float CK = 0.11;
    float2 q = p * CK;
    float v = sin(q.x * 1.7 + t * 1.30)
            + sin(q.y * 1.3 - t * 1.10)
            + sin((q.x + q.y) * 0.9 + t * 1.70);
    // abs → sırt. Kuvvet 5: ağ İNCE kalsın, yüzeyi kaplamasın. 3'te yüzeyin
    // yarısı parlıyordu ve toplam beyaza doyuyordu.
    // 1.05: ağ İNCE ve SEYREK — 0.80'de yüzeyin öyle büyük payını kaplıyordu ki
    // ışık değil DESEN gibi okunuyordu ("gradient kaplama" şikâyetinin parçası).
    float ridge = 1.0 - saturate(abs(v) * 1.05);
    float r2 = ridge * ridge;
    return r2 * r2 * ridge;
}

float4 main(PSIn i) : SV_Target0
{
    // Ekran pikselinden dünya konumu. Zemin target'ı tam ekran olduğu için
    // i.uv doğrudan ekran uv'si.
    float2 world = float2(u.b.x + i.pos.x / u.a.y,
                          u.b.y + i.pos.y / u.a.y);
    // SU, SANAT ÇÖZÜNÜRLÜĞÜNDE ÇİZİLİR. Maske örneklemesi, dalga alanı, kostik,
    // bantlar — hepsi bu çivili konumdan besleniyor; sanat pikseli içinde her
    // değer sabit, yani her kenar TAM sanat pikselinde kırılıyor. ("Pixel-art
    // gibi durmuyor" şikâyetinin cevabı: alanları kademelendirmek yetmiyor,
    // örnekleme ızgarasının kendisi kabalaşmalı.)
    float artW = u.h.w / 16.0;                 // 1 sanat px = 4 dünya px
    float2 wq  = snap_art(world, artW);

    // DERİNLİK. Maske karo başına 2×2 teksel taşıyor, LINEAR örnekleniyor:
    // teksel merkezleri arasında kendiliğinden yumuşak rampa çıkıyor. Köşeli
    // basamak siluet TAM OLARAK bu yüzden yok — kenarı biz yumuşatmıyoruz,
    // örnekleyici yumuşatıyor. 0 = kara, 1 = en derin.
    //
    // muv TÜREMESİ: u.b.zw artık TEKSEL sayısı (mapW*2). Teksel j'nin merkezi
    // karo uzayında tileP = (j+0.5)/2 olmalı, yani j = 2*tileP - 0.5 →
    // uv = (j+0.5)/N = 2*tileP/N. Eski satır (tileP+0.5)/N idi ve teksel
    // merkezini karo sınırına oturtuyordu: yarım karo kayma, bütün kıyı
    // sabitleri yanlış yerdeydi (bkz. SHORE_LINE türeme notu).
    float2 tileP = wq / u.h.w;
    float2 muv   = tileP * 2.0 / float2(u.b.z, u.b.w);
    float4 msk   = Mask.Sample(MaskS, muv);
    float depth  = msk.r;   // derinlik: rengi ve örtmeyi belirler
    float shore  = msk.g;   // KIYI uzaklığı: su çizgisini belirler (derinlikten AYRI)
    float agit   = msk.b;   // ÇIRPINTI: gövdenin mizacı
    // SÖZLEŞME (tilemap.c wmask_agitation ile ortak): çırpıntı < 0.05 →
    // gövdenin KENDİ kıyı sanatı var (vaha atlası gibi), shader kıyı
    // bantlarını çizmez. Yoksa iki kıyı işlemi üst üste biniyor ve yapay
    // lacivert şeritler çıkıyor.
    bool ownShoreArt = agit < 0.05;
    float bank   = msk.a;   // KUZEY KIYI YAKINLIĞI: çukurun kıyı gölgesi

    uint L = (uint)(u.k.w + 0.5);

    float3 floorCol = Floor.Sample(FloorS, i.uv).rgb;
    // Kıyı rampasının tamamen dışı: dokunmadan geçir. Hem R hem G aynı teksel
    // ızgarasından geldiği için ikisi birlikte sıfırlanıyor.
    if (depth <= 0.0005 && shore <= 0.0005) return float4(floorCol, 1.0);

    float t = u.a.x;

    // GELGİT — bant kenarlarının salınımı. TEK sinüs "pompa" gibi duruyor
    // (simetrik giriş/çıkış); ikinci harmonik keskin sırt + uzun çukur veriyor,
    // yani su hızla yalayıp yavaş çekiliyor.
    // 8 fps + TAM SANAT PİKSELİ: pixel-art'ta kenar hareketi ±1 px/8fps kuralı.
    // Sürekli hareket eden kenar keskin değil LAPA görünür.
    float tq = floor(t * 8.0) / 8.0;
    float ph = tq * u.g.z + dot(wq, float2(0.011, 0.007));
    float tide = saturate(0.5 * (sin(ph) - 0.25 * sin(2.0 * ph)) + 0.5);
    float edgeAP = min(floor(tide * 3.0), 2.0) - 1.0;   // -1, 0, +1 sanat px
    // DURGUN gölette gelgit YOK — su çizgisi sabit durur (Hakan: 'köşelerde
    // dalgalanmaya gerek yok'). Kenar oynaması dalgalı suyun işi.
    edgeAP *= (agit > 0.35) ? 1.0 : 0.0;
    float edge   = edgeAP * SHORE_PX;

    // ---------------------------------------------------------------------
    // (a) ISLAK BANT — kıyının KARA tarafı. Buradaki piksel SU DEĞİL, ıslak
    // toprak: o yüzden floorCol'un kendisine, tint'ten ÖNCE uygulanıyor ve su
    // renginden hiç pay almıyor.
    // Islak yüzey iki şey yapar: KOYULAŞIR ve DOYGUNLAŞIR (ince su filmi
    // difüz yansımayı kesip gövde rengini öne çıkarır). Düz çarpım yalnız
    // koyulaştırıp DESATÜRE ettiği için "gölge" gibi okunuyordu — sahibinin
    // "gölge yok" dediği şey aslında bu bandın hiç olmamasıydı.
    // ---------------------------------------------------------------------
    if (shore < SHORE_LINE) {
        if (layer_on(L, LB_WET) && u.k.x > 0.0 && !ownShoreArt) {
            // Gelgit yalnız İÇERİ çekebilir (min(edge,0)): bandın dış kenarı
            // 4 sanat px'te maskenin kara rampası bittiği için dışarı taşamaz.
            float span = SHORE_LINE + min(edge, 0.0);
            float wetT = saturate((shore - (SHORE_LINE - span)) / max(span, 1e-5));
            wetT = ceil(wetT * 2.0) / 2.0;          // 2 kademe — el çizimi gibi
            float3 wetC = pow(max(floorCol, 0.0), 1.0 + 0.55);
            float lum = dot(wetC, float3(0.299, 0.587, 0.114));
            wetC = lum + (wetC - lum) * 1.35;       // doygunluk: ıslak, gölge değil
            floorCol = lerp(floorCol, saturate(wetC), wetT * u.k.x);
        }
        return float4(saturate(floorCol), 1.0);
    }

    // Buradan sonrası SU. Su çizgisi tam karo sınırına oturuyor (SHORE_LINE
    // türemesi), yani su tinti artık çimenin üstüne yarım karo sızmıyor.

    // KUZEY DUVAR YÜZÜ (araştırma R3, Yol B prototipi). Ölçülen gerçek:
    // ALttP/Stardew/Forager üçü de çukur hissini GÖLGEYLE değil, kameraya
    // bakan kenara çizilmiş MALZEME KESİTİYLE veriyor (ALttP: 1-1.5 karo
    // fistolu yamaç; Stardew: üstü beyaz ışıklı taş sırası). Gölge yardımcı
    // işaret, duvar asıl işaret — bizim '3 kademeli gölge yetmedi' gözlemi
    // literatürle birebir. Burada suyun ilk 6 sanat px'i, kuzeyinde kara
    // olan yerde, OPAK toprak kesiti olarak boyanıyor: tepede koyu kontur,
    // ortada aydınlık strata, dipte koyu (su çizgisine inerken kararır).
    // Dip çizgisi cetvel olmasın diye sütun başına 0/1 px hash kırılması.
    // OPAK YAZIM: kırılma/tint/kostik bulaşmaz; eklemeli bütçeye girmez.
    // Kalıcı çözüm karo sanatı (Yol A) — bu prototip görsel dili doğruluyor.
    float bq3 = floor(saturate(bank) * 3.0 + 0.5) / 3.0;
    float wallOfs = 0.0;                       // bant konumlarına eklenir (G birimi)
    // u.l.w: duvar anahtarı. RAFA KALDIRILDI (silinmedi): Hakan'ın referans
    // görüntüsü çukuru duvarla değil KIYI GÖLGESİ + batık taş rafıyla
    // anlatıyor; prototip duvar oyunda iskele tahtası gibi okundu. Karo
    // sanatı (Yol A) gelince kıyas için u.l.w=1 ile geri açılabilir.
    if (u.l.w > 0.5 && u.l.x > 0.0 && layer_on(L, LB_BANK) && bq3 > 0.9) {
        const float WALL_H = 6.0;              // sanat px
        float rowAP = floor((shore - SHORE_LINE) / SHORE_PX);
        uint hc = uhash2((uint)(int)(floor(world.x / artW) + 8192.0), 77u);
        float hJit = (float)(hc & 1u);
        if (rowAP < WALL_H - hJit) {
            float3 wallC = (rowAP < 1.0) ? float3(0.13, 0.11, 0.09)
                         : (rowAP < 4.0) ? float3(0.42, 0.31, 0.19)
                         :                 float3(0.24, 0.17, 0.11);
            return float4(wallC, 1.0);
        }
        wallOfs = WALL_H * SHORE_PX;           // menisküs/koyu bant duvardan sonra
    }

    // ÇIRPINTI — gövdenin mizacı (0 ayna gibi durgun, 1 açık su). Gölet deniz
    // değil: dalgası küçük, YAVAŞ, ve köpüğü yok. Durgun yüzey ışığı dibe
    // geçirdiği için kostik TERS yönde güçleniyor — gölette dibi görmenin
    // sebebi bu, suyun inceliği değil.
    // Zaman ölçeği gövde başına: durgun su daha yavaş kıpırdar. Gövdeler
    // komşu olmadığı için faz farkı dikiş yapmıyor.
    // 0.12: "GÖLETTE DALGA OLMAZ" (Hakan) — gölet fiilen durgun, kalan tek
    // hareket çok yavaş bir kıpırtı. 0.40'ta hâlâ deniz ritmi okunuyordu.
    float3 w = wave_field(wq, t * lerp(0.12, 1.0, agit));
    // Genlik ölçeği: hem yükseklik hem eğim. Eğim küçülünce kırılma ve parlama
    // kendiliğinden zayıflıyor (ikisi de eğimden türüyor) — ayrıca kısmak
    // gerekmiyor, denizin dinamiği tek yerden geliyor.
    // Genlik ölçeği YALNIZ YÜKSEKLİĞE uygulanıyor, EĞİME DEĞİL.
    // HATA GEÇMİŞİ: eğim de ölçeklenince gölette kırılma ofsetinin tepesi
    // 2.4 px'e düşüyordu ve sanat pikseline (4 px) yuvarlanınca SIFIRA
    // gidiyordu — yani göletin durgunluğunu kurmak için sualtı bükülmesini
    // öldürmüş oluyordum. Durgunluk dalganın YAVAŞLIĞINDAN ve yüksekliğinden
    // gelir; eğim yüzeyin dokusudur, onu silmek suyu cama çevirir.
    w.x *= lerp(0.26, 1.0, agit);

    // YÜKSEKLİK. Dalga sırtı SIĞ, çukuru DERİN görünüyor — yükseklik hissi
    // buradan doğuyor. Derinliği sabit bir sayı değil, dalganın kendisi
    // değiştiriyor; sırt açık, çukur koyu çıkıyor ve köpük eşiği de bununla
    // tırtıklanıyor (düz halka yerine kırılan dalga).
    float dw = saturate(depth + w.x * u.f.x);

    // EĞİM NORMALİZASYONU — BİR KEZ, burada. Bütün katmanlar bundan besleniyor.
    // HATA GEÇMİŞİ: bu çarpan yalnız parlama satırına uygulanmıştı, kırılmaya
    // değil. Ham eğim en fazla ΣW·K ≈ 0.053 olduğu için kırılma ofseti 0.074
    // px çıkıyordu ve sanat pikseline yuvarlanınca TAM SIFIR oluyordu: shader'ın
    // "bütün mesele bu" dediği katman hiç çalışmadı. Her katmanın kendi sihirli
    // sayısını taşıması bu hatanın sebebiydi; tek kaynak artık burası.
    const float SLOPE_NORM = 18.98;          // 1/(0.52*0.021 + 0.31*0.058 + 0.17*0.140)
    float2 n2 = float2(w.y, w.z) * SLOPE_NORM;   // eğim ≈ birim disk içinde
    float  sl = length(n2);
    float2 nd = sl > 1e-5 ? n2 / sl : float2(0.0, 0.0);
    float  fa = saturate(dot(nd, u.c.xy));       // güneşe dönüklük — paylaşılıyor

    // KIYI GÖLGESİ — kamera 3/4 eğik, gölet zemine açılmış bir ÇUKUR: kuzey
    // kıyısının iç duvarı suya gölge düşürür. Bu katman olmadan gölet çukur
    // değil, çimenin üstüne yapıştırılmış bir çıkartma gibi duruyordu (Hakan'ın
    // "köşelerin açıya göre gölgesi" dediği şey). BURADA hesaplanıyor çünkü
    // kostik ve parlama da bundan besleniyor: GÖLGEDE GÜNEŞ YOK, dolayısıyla
    // gölgede ne kostik ağı olur ne parıltı — bu tutarlılık gölgeyi "koyu bant"
    // olmaktan çıkarıp "üstüme kıyı düşüyor" yapan şeyin ta kendisi.
    // 3 kademe: pixel-art gölgesi airbrush değil, kademeli boyanır.
    float bshad = 0.0;
    if (u.l.x > 0.0 && layer_on(L, LB_BANK) && !ownShoreArt) {
        bshad = bq3 * u.l.x;   // kademe yukarıda (duvarla paylaşılıyor)
    }

    // KIRILMA. Yüzey eğimi kadar kaydırıp altını örnekliyoruz.
    // GEÇİT ARTIK KIYI UZAKLIĞI (G), derinlik (R) değil: R gövdeye göre ölçekli
    // ve küçük gövdelerde neredeyse sabit kalıyor, o yüzden ofseti süremiyordu.
    // G gövdeden bağımsız, su çizgisinde SHORE_LINE → çimen suyun içine sızmıyor.
    // Eşikler artık SHORE_LINE'a GÖRE yazılıyor (eski 0.0/0.22 çifti, yarım karo
    // kayma yüzünden aslında karanın üstünde başlıyordu).
    // SNAP — pixel-art'ın şartı: ofset TAM sanat pikseline yuvarlanıyor,
    // kesirli ofset NEAREST örneklemede dalgalanma değil TİTREME görünür.
    // Gölet biraz daha az bükülüyor (0.70) ama SIFIR değil — fark mizaç kadar,
    // varlık kadar değil.
    if (layer_on(L, LB_REFRACT)) {
        // Geçit kıyıdan 2 px sonra başlar, 10 px'te tam: tepe ofset ~3.5 sanat
        // px olduğu için daha erken açılırsa ÇİMEN piksellerini suya çekiyor
        // (kenarda ışıltılı kıpırtı olarak görülüyordu).
        float2 off = n2 * u.f.y
                   * smoothstep(SHORE_LINE + 2.0 * SHORE_PX,
                                SHORE_LINE + 10.0 * SHORE_PX, shore)
                   * lerp(0.70, 1.0, agit);
        float art = max(u.c.w, 1.0);
        off = round(off / art) * art;
        float2 offUV = off / float2(u.a.z, u.a.w);
        floorCol = Floor.Sample(FloorS, i.uv + offUV).rgb;
    }

    // KOSTİK — TABANDA, yani tint'ten ÖNCE floorCol'a ekleniyor. Sıra fizik:
    // kostik ışığı taban yüzeyinde doğuyor ve göze giderken su sütunu onu
    // yutuyor. Önceki sürümde tint'ten SONRA eklendiği için yutulmuyordu ve
    // suyun üstünde bir çıkartma gibi duruyordu; oradaki smoothstep(0,0.10)
    // çarpanı da yanlış sıranın semptom yamasıydı — sıra düzelince gereksiz.
    if (u.f.z > 0.0 && layer_on(L, LB_CAUSTIC)) {
        // Zaman 8 fps'e kuantalı (tq): ağ TAM piksel adımlarıyla kayar.
        float ca = caustic(wq + n2 * 14.0, tq * lerp(0.55, 1.0, agit));
        // 2 kademe + Bayer: pixel-art'ta öğe ya VARDIR ya YOKTUR; sürekli
        // parlaklık rampası kostiği 'photoshop eğrisi' yapıyordu.
        ca = floor(saturate(ca * 3.0) * 2.0 + bayer4(wq, artW)) / 2.0;
        // Durgun suda kostik GÜÇLENİYOR: yüzey kırışmadığı için ışık dağılmadan
        // dibe iniyor. Gölette dibi net görmenin sebebi bu.
        // Geçit SU ÇİZGİSİ (G), derinlik değil: kuru toprakta kostik olmaz.
        // Su çizgisinden 1 sanat px içeride açılıyor, 6 px'te tam.
        // (1-bshad): gölgede güneş yok → kostik ağı da yok. Gölgenin içinde
        // parlamaya devam eden kostik, gölgeyi anında sahteleştiriyor.
        // 1.25 (eski 1.7): gölette kostik güçlü kalsın ama BASKIN olmasın —
        // 1.7'de tek görünen şey oydu ve su değil desenli kaplama okunuyordu.
        // Gölette kostik SÖNÜK (0.45): durgun gölette baskın ışık oyunu
        // olmaz — 1.25'te bütün yüzey 'dalgalanıyor' okunuyordu (Hakan).
        floorCol += u.h.rgb * (ca * u.f.z * u.c.z * lerp(0.10, 0.85, agit)
                               * smoothstep(SHORE_LINE + 1.0 * SHORE_PX,
                                            SHORE_LINE + 6.0 * SHORE_PX, shore)
                               * (1.0 - bshad * 0.9));
    }

    // DERİNLİK GRADYANI / SOĞURMA. Sığda su rengi ince, taban (ve kostiği)
    // okunuyor; derinde kalınlaşıp kapatıyor. Rampa artık gövde başına
    // normalize (bkz. tilemap.c wld_water_mask_fill): 8 karoluk gölet de
    // merkezinde tam derinliğe erişiyor, menzilin %12'sinde kalmıyor.
    float3 col = floorCol;
    if (layer_on(L, LB_DEPTH)) {
        // BASAMAKLI BANTLAR — sürekli gradyan DEĞİL. "Gradient kaplama gibi
        // duruyor" (Hakan) şikâyetinin kökü buydu: pürüzsüz örtme rampası
        // boyanmış su değil, radyal dolgu gibi okunuyor. 4 kademe = elle
        // boyanmış derinlik terasları; dw dalga yüksekliğini içerdiği için
        // bant SINIRLARI yavaşça soluk alıyor (canlı, ama kademeli).
        // BAYER YOK — BİLEREK (iki kez yaşandı): ordered dither ARA değeri
        // her pikselde damaya çevirir; göletin düz derinlik platosu hep bir
        // bandın ORTASINA denk geldiği için bütün orta koca bir dama
        // dikdörtgeni oluyordu. Sert bant sınırı pixel-art için zaten doğru;
        // sınır dalga kıpırtısıyla organik kırılıyor.
        // BANT SAYISI MİZAÇLA: durgun/küçük gövdede 4 kademe HALKA gibi
        // diziliyor ve yapay duruyor (Hakan, vaha) — küçük gövde tek yumuşak
        // geçiş ister. Deniz 4 bant (teras okuması iyi), gölet/vaha 2.
        float bands = lerp(2.0, 4.0, agit);
        // BANT KENARLARI ORGANİK. Mesafe alanının eş-yükselti çizgileri düz
        // kıyıda DÜZ olduğu için bantlar ekranı boydan boya kesen yatay
        // TERASLARA dönüşüyordu (Hakan: "denizin ortasında beliren yatay
        // genişlikler"). Kuantalamadan ÖNCE derinliğe yumuşak mekânsal
        // sapma ekliyoruz: bant sınırı deniz tabanı konturu gibi kıvrılıyor.
        // Eğim sınırı ark katmanındakiyle aynı mantık — dalga boyları uzun
        // ve genlik küçük, yoksa sınır dikleşip dikey şerit yapar.
        float dpert = 0.055 * sin(dot(wq, float2(0.0074, 0.0031)))
                    + 0.032 * sin(dot(wq, float2(0.0043, -0.0096)) + 2.2)
                    + 0.018 * sin(dot(wq, float2(0.0151, 0.0067)) + 4.1);
        float dwq = floor((dw + dpert) * bands + 0.5) / bands;
        dwq = saturate(dwq);
        // Üst iki bant BİRLEŞİK (×1.34 tavanlı): en derin bant ayrı renk
        // alınca göletin ortasında keskin kenarlı koyu bir DİKDÖRTGEN
        // beliriyordu (BFS mesafe alanı köşeli teras çiziyor). Derin bölge
        // tek tonda birleşince teras çizgisi rafta kalıyor, ortada değil.
        float dwT = min(dwq * 1.34, 1.0);
        float3 tint = lerp(u.d.rgb, u.e.rgb, dwT);
        float  cover = lerp(u.d.w, u.e.w, dwT);
        // İKİ SU MODELİ, mizaçla harmanlı (Hakan: "su gibi değil, ŞEFFAF su
        // gibi olmalı"):
        //   ÖRTÜ (deniz): taban su rengiyle ÖRTÜLÜR — derin deniz opaktır.
        //   SÜZGEÇ (gölet): taban RENKLİ CAMDAN süzülür — çarpımsal geçirim
        //     (su kırmızıyı yutar → her şey mavi-yeşile kayar, YAPI korunur)
        //     + derinlikle artan hafif mavi saçılma. Gri sis örtüsü değil.
        // Eski hâl yalnız örtüydü ve gölet 'gri levha' okunuyordu: gri taban
        // × gri-mavi örtü = gri. Süzgeçte taban görünür AMA rengi su rengi.
        // Açıldı (Hakan: 'biraz daha açık ve şeffaf'): süzgeç daha çok ışık
        // geçiriyor, derin uç daha açık maviye süzüyor.
        float3 transmit = lerp(float3(0.84, 0.95, 0.98),
                               float3(0.44, 0.66, 0.84), dwT);
        float3 scatterC = lerp(float3(0.00, 0.02, 0.05),
                               float3(0.05, 0.14, 0.25), dwT);
        float3 colTrans = floorCol * transmit + scatterC;
        float3 colCover = lerp(floorCol, tint, cover);
        // Referans (Hakan'ın örneği): şeffaflık HER YERDE değil, yalnız
        // SIĞLIK RAFINDA — kıyı şeridinde batık taşlar loş görünür, ORTA
        // OPAK KOYU LACİVERTTİR. Önceki hâl ortada da yarı şeffaftı ve gri
        // taban bütün göleti griye çekiyordu. Derinlik arttıkça süzgeçten
        // örtüye geçilir; deniz (agit) zaten hep örtü.
        // DİKKAT: geçiş SÜREKLİ dw'den, bantlı dwq'dan DEĞİL. dwq'dan sürünce
        // Bayer dither şeffaf↔opak sınırını damalıyor — renk bandında hoş olan
        // dama, YAPI var/yok sınırında koca bir artefakt dikdörtgeni oluyor
        // (yaşandı: gölet ortası tam band sınırındaydı, dama alanı ekranı
        // kapladı). Renk bantları dwq'da kalır, yapısal geçiş dw'de.
        float toCover = max(smoothstep(0.30, 0.70, agit),
                            smoothstep(0.30, 0.62, dw));
        col = lerp(colTrans, colCover, toCover);

        // RÜZGÂR ŞERİTLERİ — çok uzun dalga boylu (≈780 px), çok yavaş,
        // 3 kademeli ton lekesi (±%5). Tekdüze orta bölgeyi kıran katman:
        // durgun su bile TEK TON değildir, gök yansıması geniş lekeler halinde
        // gezer. Sürekli gradyan değil, kademeli — o yüzden leke, sis değil.
        float lane = sin(dot(wq, WDIR0) * 0.008 + t * lerp(0.03, 0.12, agit));
        col *= 1.0 + 0.05 * floor(lane + 0.5);   // Bayer'siz — aynı sebep
    }

    // DALGA-YANKI ARKLARI (araştırma R2) — kıyı konturuna PARALEL, ~8 sanat
    // px aralıklı, 1 px kalın koyu sıralar; kıyıdan 2-3 sıra sonra söner.
    // ALttP'nin imzası: suyu 'kabın içinde duran yüzey' yapan şey, kıyı
    // çizgisinin suya yankısı. G kanalı kıyı uzaklığını taşıdığı için kontur
    // takibi bedava. Fisto: ±1 px'lik TAM piksel oynama — cetvel olmasın.
    // Dışa akış 2 px/sn, 8 fps adımlı (tq): dalga kıyıdan uzaklaşıyor hissi.
    // ── İLERLEYEN KABARMALAR (swell) — DENİZ SIRTLARI. Bu iş CPU'da
    // denendi ve YEDİ TUR sürdü; her turda "suyun üstüne yapıştırılmış
    // beyaz çubuk" gibi durdu. Sebep mimariydi: CPU geçişi su geçişinden
    // SONRA çiziyor, yani sırtlar suyun renk hattından, derinlik
    // bantlarından ve kıyı geometrisinden hiç geçmiyor. Burada, shader'ın
    // içinde, sırtlar suyun KENDİ alanından türüyor:
    //   · konum KIYI UZAKLIĞI alanından (shore) → kıyıya kendiliğinden
    //     paralel, koyu ve girintiyi kendiliğinden izler (CPU'da bunu elle
    //     kurmak imkânsızdı: orada yalnız sütun başına tek y vardı)
    //   · renk çarpımsal + hafif eklemeli → suyun tonunun İÇİNDE kalır
    //   · derinlik/gölge/çırpıntı hepsi zaten burada
    if (u.l.z > 0.0 && layer_on(L, LB_ARC)) {
        float sAP = (shore - SHORE_LINE - wallOfs) / SHORE_PX;

        // HIZ: 1.8 sanat px/sn, dalga boyu 34 px → sırt aralığı ~19 sn.
        // HATA GEÇMİŞİ: 3.4 px/sn + geniş yumuşak gövde bandı kullanılmıştı;
        // sonuç "hızlıca gidip gelen doğal olmayan gölge" (Hakan). Pixel-art'ta
        // dalga GENİŞ TON SÜPÜRMESİ olarak okunmaz — KESKİN İNCE KÖPÜK HATTI
        // + hemen arkasında ince koyu çukur olarak okunur. Geniş gövde
        // parlatması tamamen kaldırıldı.
        // KIVRIM — sırt düz olmasın. Düz kumsalda kıyı uzaklığı alanının
        // eş-yükselti çizgileri ZORUNLU olarak yataydır; dalga hissi için
        // fazı x boyunca kaydırmak gerekiyor. DALGA BOYU KISA olmalı: uzun
        // olursa ekranda tek yönlü RAMPA gibi görünür (ilk turun hatası —
        // 766 px yarı periyot ekrandan geniş olduğu için eğim sanılıyordu).
        // Üç oktav, en uzunu ~570 px: ekranda birkaç kıvrım görünür.
        // Zaman bu fonksiyonlara GİRMEZ (girerse desen yana yürür).
        // KIVRIM EĞİMİ SINIRI (bu hesap ihlal edilirse artefakt kesin):
        // Sırt çizgisi ph'nin eş-yükselti çizgisidir. Dikey türev
        //   dph/dy = 1/(34 sanat px * 4 dünya px) = 0.00735 /dünya px.
        // Yatay türev |dbend/dx| bunu AŞARSA çizgi 45°'den dikleşir ve
        // KIYIYA DİK tuhaf şeritler çıkar (yaşandı: türev 0.0106 idi).
        // Kural: |dbend/dx| ≤ ~0.35 × dph/dy  →  ≤ 0.0026.
        // Aşağıdaki toplam: 0.10*0.0105 + 0.05*0.0210 = 0.0021 ✓
        float bend = 0.10 * sin(wq.x * 0.0105)
                   + 0.05 * sin(wq.x * 0.0210 + 1.7);
        float ph = frac((sAP + tq * 1.8) / 34.0 + bend);

        // Kıyı boyunca yumuşak zarf (zaman GİRMEZ — girerse desen yana yürür)
        float sw1 = sin(dot(wq, float2(0.0062, 0.0021)));
        float sw2 = sin(dot(wq, float2(0.0141, -0.0037)) + 2.3);
        float env = saturate(0.20 + 0.80 * (0.55 * sw1 + 0.45 * sw2 + 0.5));

        // Sığda kabarma güçlenir (dalga kırılır)
        float shallow = saturate(1.0 - sAP / 34.0);
        float gate = u.l.z * (1.0 - bshad) * smoothstep(0.20, 0.45, agit) * env;

        // KÖPÜK HATTI — SERT eşik, 1-2 sanat px genişlik. Sığda kalınlaşır.
        // Sert eşik şart: smoothstep rampası bulanık gölge bandı yapıyor.
        // Kalınlık da kıvrımla birlikte oynuyor: sırt yer yer kabarık
        float lipW = (0.026 + 0.020 * shallow)
                   * (0.75 + 0.55 * (0.5 + 0.5 * sin(wq.x * 0.0163 + 2.9)));
        if (ph < lipW) {
            float fa = (0.34 + 0.66 * shallow) * gate;
            col += float3(0.84, 0.90, 0.95) * min(fa * 0.34, 0.20);
        }
        // ÇUKUR — köpüğün hemen ARKASINDA ince koyu hat (sırtın gölgesi),
        // çarpımsal ve navy'ye kayan (palet disiplini). Genişliği köpüğün
        // yarısı kadar: kalın olursa yine "gölge bandı" olur.
        else if (ph < lipW + 0.026) {
            col *= lerp(float3(1.0, 1.0, 1.0), float3(0.80, 0.84, 0.94),
                        (0.45 + 0.55 * shallow) * gate);
        }
    }

    // Kıyı gölgesi RENGE burada biner: derinlik örtmesinden sonra (gölge suyun
    // rengini de karartır), yüzey parlamalarından önce (onlar zaten gölgeyle
    // ayrı ayrı sönüyor). Hafif maviye kırıyor: gölgede su gök ışığıyla,
    // yani soğuk aydınlanır — düz siyah çarpım "is lekesi" gibi duruyor.
    if (bshad > 0.0) {
        col *= lerp(float3(1.0, 1.0, 1.0), float3(0.30, 0.36, 0.56), bshad);
    }

    // PARLAMA — güneş açısı. Yüzey eğimi güneşe dönükse parlıyor; güneş yönü
    // oyun saatinden geliyor, yani sabah ile akşam AYNI GÖRÜNMÜYOR. Kademeye
    // kuantalanıyor: sürekli gradyan pixel-art'ta yağlı boya gibi duruyor.
    if (u.g.x > 0.0 && layer_on(L, LB_GLINT)) {
        // fa ve sl yukarıda BİR KEZ hesaplandı; normalize eğim zaten 0..1
        // aralığında, o yüzden burada ek çarpan YOK. Eskiden bu satır kendi
        // 320'lik (sonra 19'luk) çarpanını taşıyordu — sihirli sayının tek
        // katmanda kalması, kırılmanın hiç çalışmamasının sebebiydi.
        float  sp = pow(fa, 8.0) * saturate(sl);
        sp = floor(sp * 3.0) / 3.0;                        // 3 kademe
        // lerp(0.35,...): gölette parıltı seyrek ve sönük — ayna yavaş parıldar.
        // (1-bshad): gölgede güneş yok, parlama da yok.
        col += u.h.rgb * (sp * u.g.x * u.c.z
                          * lerp(0.08, 1.0, agit) * (1.0 - bshad));
    }

    // IŞILTI ÇİZGİCİKLERİ — pixel-art suyunun İMZASI. Bütün diğer katmanlar
    // SÜREKLİ alan; sürekli alan yığını "gradient kaplama" gibi okunuyor
    // (Hakan'ın şikâyeti). Su hissini veren şey AYRIK öğedir: kısa yatay açık
    // çizgicikler — belirir, akıntıyla sürüklenir, kaybolur. Elle çizilmiş su
    // animasyonlarındaki tek tek parlak tarama çizgilerinin prosedürel hâli.
    // Hücre 16×8 sanat px; hash hücrelerin ~%25'ine tek çizgi veriyor, çizgi
    // 1 sanat px kalın, 4-7 px uzun, AÇIK/KAPALI yanıp sönüyor (yumuşak
    // solma değil — pixel-art'ta öğe ya vardır ya yoktur).
    if (u.l.y > 0.0 && layer_on(L, LB_DASH)) {
        // Sürüklenme SANAT PİKSELİ adımlarıyla: kesirli kayma titreme yapar.
        float drift = floor(t * lerp(1.5, 6.0, agit) / artW) * artW;
        float ax = floor((world.x - drift) / artW);   // sanat-px ızgara konumu
        float ay = floor(world.y / artW);
        float cx = floor(ax / 16.0), cy = floor(ay / 8.0);
        uint hh = uhash2((uint)(int)(cx + 8192.0), (uint)(int)(cy + 8192.0));
        // İKİ ÖĞE TÜRÜ (Hakan'ın referans görüntüsünden):
        //   BEYAZ KÖPÜK BENEĞİ — 2-3 px düzensiz beyaz leke, elle konmuş
        //   gibi; yanıp söner ve sürüklenir. Referansta yüzeyin tek parlak
        //   dokusu bu benekler (kostik/parlama bandı YOK).
        //   TURKUAZ YOSUN TUTAMI — suyun İÇİNDE dikey ince şerit, yavaşça
        //   salınır; yalnız durgun suda (deniz dalgası tutam bırakmaz).
        uint kind = (hh >> 2u) & 3u;   // 0-1: benek · 2: yosun · 3: boş
        if ((hh & 3u) < 2u && kind < 2u) {
            float inX = ax - cx * 16.0;
            float inY = ay - cy * 8.0;
            float dy  = 2.0 + (float)((hh >> 4u) & 3u);
            float x0  = 2.0 + (float)((hh >> 8u) & 7u);
            float ph  = t * (0.30 + (float)((hh >> 16u) & 3u) * 0.12)
                      + (float)(hh & 255u) * 0.0246;
            // Benek: 2 px yatay + hash'e göre 1 px alt çıkıntı (L biçimi).
            bool core = (inY == dy) && (inX >= x0 && inX < x0 + 2.0);
            bool nub  = (inY == dy + 1.0) && (inX == x0 + (float)((hh >> 20u) & 1u));
            if (sin(ph) > 0.20 && (core || nub)) {
                // EKLEMELİ BÜTÇE: benek 0.12 + kostik 0.14 + parlama 0.16 =
                // 0.42 ≤ 0.45 (gölette kostik/parlama zaten ~0).
                col += float3(0.90, 0.95, 1.00) * (u.l.y * (1.0 - bshad));
            }
        } else if ((hh & 3u) < 2u && kind == 2u && ((hh >> 6u) & 1u) == 0u
                   && agit < 0.5) {   // yarıya inildi: referansta tutam SEYREK
            float inX = ax - cx * 16.0;
            float inY = ay - cy * 8.0;
            float xw  = 4.0 + (float)((hh >> 8u) & 7u);
            // Satır başına ±1 px salınım — tutam suda yatar kalkar
            float sway = floor(sin(ay * 0.45 + t * 0.7
                                   + (float)(hh & 255u) * 0.0246) + 0.5);
            if (inY >= 1.0 && inY <= 6.0 && inX == xw + sway) {
                // Suyun İÇİNDE: eklemeli değil KARIŞIM — parlamaz, boğuk
                col = lerp(col, float3(0.22, 0.52, 0.46), 0.65);
            }
        }
    }

    // HALKA DALGACIK — balık/damla izi (Stardew: 8 kare @100ms; bizde
    // prosedürel karşılığı). Seyrek hücrelerde 1 px'lik halka tam piksel
    // adımlarıyla genişleyip sönüyor; merkez hash'ten kayık, her ~3 sn'de
    // yeni zar. Balık gölgeleriyle aynı suda yaşar — biri altta yüzer,
    // öteki yüzeyde iz bırakır.
    if (layer_on(L, LB_RING)) {
        // YAĞMUR (u.n.w): damla halkaları — yağmurda hücrelerin çoğu aktif ve
        // halka ömrü kısa (sık, hızlı, küçük). Kuru havada seyrek balık izi.
        float rper = lerp(2.8, 1.0, u.n.w);
        float rcell = 32.0 * artW;
        float2 rc = floor(world / rcell);
        float ep = floor(t / rper);
        uint rh = uhash2((uint)(int)(rc.x + 8192.0) ^ ((uint)(int)ep) * 0x27D4EB2Du,
                         (uint)(int)(rc.y + 8192.0));
        if ((rh & 7u) < (uint)(1.0 + u.n.w * 4.0)) {
            float tt = t - ep * rper;
            float2 ctr = (rc + 0.5) * rcell
                       + (float2((float)((rh >> 8u) & 15u),
                                 (float)((rh >> 12u) & 15u)) - 7.5) * artW;
            float dpx = length(snap_art(world, artW) - ctr) / artW;
            float rr = 1.0 + floor(tt * 3.0);          // 1..9 sanat px, tam adım
            float fade = saturate(1.0 - tt / 2.2);
            if (rr < 9.0 && abs(dpx - rr) < 0.6) {
                col += float3(0.85, 0.92, 0.98) * (0.10 * fade * (1.0 - bshad));
            }
        }
    }

    // ---------------------------------------------------------------------
    // (b) KOYU HAT — suyun KENDİ SİLUETİ. Su çizgisinden 1 sanat px içeride
    // başlıyor, 1.2 px kalın, keskin (smoothstep DEĞİL: pixel-art konturu
    // anti-alias'lanmaz).
    // NEDEN ÇARPIMSAL: pixel-art kontur kuralı "koyu çizgi aydınlık tarafta
    // daha açık, karanlık tarafta daha koyu" — yani hat altındaki rengin
    // fonksiyonu olmalı. Sabit koyu renk lerp'lenirse hat her yerde aynı çıkar
    // ve suyun üstüne çizilmiş bir cetvel gibi durur.
    // ---------------------------------------------------------------------
    if (u.k.y > 0.0 && layer_on(L, LB_DARK) && !ownShoreArt) {
        float dpos = SHORE_LINE + wallOfs + (DARK_C + edgeAP) * SHORE_PX;
        float dband = abs(shore - dpos) < (DARK_H * SHORE_PX) ? 1.0 : 0.0;
        // Durgun suda hat TAM güçte ve kesintisiz; çırpıntılı suda 0.55× ve
        // dalga tarafından kırılıyor (kıyıya vuran su net kontur bırakmaz).
        float dk = dband * u.k.y * lerp(1.0, 0.55 * saturate(0.6 + w.x), agit);
        // Düz gri çarpım değil NAVY: pixel-art kuralı — koyulaşan ton maviye
        // kayar (ALttP gölge bandı (48,72,128), tabanın morlaşmış koyusu).
        col *= lerp(float3(1.0, 1.0, 1.0), float3(0.55, 0.60, 0.82), saturate(dk));
    }

    // KÖPÜK — kıyıya vurup GERİ ÇEKİLEN dalga. Hile şu: köpüğün eriştiği
    // derinlik eşiği zamanla salınıyor. Eşik büyürken bant karaya doğru
    // ilerliyor, küçülürken geri çekiliyor. Faz dünya konumuna göre kayıyor,
    // yoksa bütün kıyı aynı anda kırılır ve halka gibi durur.
    // Köpük yalnız ÇIRPINTILI suda var. Gölette dalga kıyıya vurmaz, yalanır —
    // o yüzden eşik 0.35'in altında hiç köpük çizilmiyor (gölet 0.10, deniz 1.0).
    float foam = 0.0;
    if (u.f.w > 0.0 && agit > 0.35 && layer_on(L, LB_FOAM)) {
        float fph = t * u.g.z + wq.x * 0.011 + wq.y * 0.007;
        // Eşik KIYI uzaklığında ölçülüyor, derinlikte değil — yoksa sığ bir
        // havuzun tamamı köpük sayılıyor (ilk sürümün hatası).
        float reach = 0.10 + u.g.w * (0.5 + 0.5 * sin(fph));
        // Dalga sırtı köpüğü öne itiyor, çukur geri çekiyor
        float sw = saturate(shore - w.x * u.f.x * 0.6);
        // Ana bant: eşiğin kıyı tarafı. Kuantalı — el çizimi gibi, yumuşak değil.
        float band = 1.0 - smoothstep(reach - 0.07, reach, sw);
        band = floor(band * 2.0 + 0.35) / 2.0;
        // İnce dudak: bandın tam ucunda kalan parlak çizgi (çekilen suyun izi)
        float lip = 1.0 - smoothstep(0.010, 0.026, abs(sw - reach));
        foam = saturate(band * 0.75 + lip * 0.55) * u.f.w;
        col = lerp(col, float3(0.88, 0.94, 0.97), foam);
    }

    // ---------------------------------------------------------------------
    // (c) MENİSKÜS — koyu hattın SU tarafında ince parlak çizgi. Suyun kenara
    // tırmanan yüzey gerilimi göğü yansıtıyor. Gölette köpüğün YERİNİ tutuyor:
    // gölette köpük yok ama kıyı çizgisinin parlaması var, kıyıyı okutan şey o.
    // Köpükten SONRA ve (1-foam) ile maskeli: köpüğün üstünde ikinci bir beyaz
    // çizgi çift kontur gibi duruyor ve toplamı doyuruyordu.
    // EKLEMELİ BÜTÇE: kostik 0.14 + parlama 0.16 + menisküs 0.13 = 0.43 ≤ 0.45.
    // Tavan R8G8B8A8_UNORM yüzünden: aşınca su beyaza doyuyor (iki kez yaşandı).
    // ---------------------------------------------------------------------
    if (u.k.z > 0.0 && layer_on(L, LB_MENI) && !ownShoreArt) {
        float mpos = SHORE_LINE + wallOfs + (MENI_C + edgeAP) * SHORE_PX;
        float mband = abs(shore - mpos) < (MENI_H * SHORE_PX) ? 1.0 : 0.0;
        // (1-bshad*0.7): menisküs göğü yansıtır; gölgedeki kıyıda gök yok,
        // çizgi sönük — ama tamamen silinmez (0.7), kıyı yine okunmalı.
        col += float3(0.90, 0.95, 1.00) * (mband * u.k.z * (1.0 - foam)
                                           * (1.0 - bshad * 0.7));
    }

    // LAMBA YANSIMASI — gece, ışık kaynağının GÜNEYİNE düşen titrek sıcak
    // sütun (göl kenarında fener klasiği). Yatayda dar (dalga eğimiyle
    // yalpalar — cetvel değil), dikeyde uzun ve sönümlü, 3 kademeli.
    // Karanlık çarpanı (u.n.z): gündüz kendiliğinden kapanır.
    if (layer_on(L, LB_LAMP) && u.n.z > 0.05) {
        [unroll] for (int li = 0; li < 2; li++) {
            float2 lp = (li == 0) ? u.m.xy : u.m.zw;
            float  Li = (li == 0) ? u.n.x : u.n.y;
            if (Li <= 0.0) continue;
            float dyW = wq.y - lp.y;
            if (dyW < 0.0) continue;
            // Sütun kaynağın TAM altından değil, ~1/3 karo aşağıdan doğar:
            // meşale taşıyan oyuncunun kendi ayağının altını turuncuya
            // boyuyordu (Hakan'ın yakaladığı blok bunun doymuş hali).
            float rise = smoothstep(6.0, 22.0, dyW);
            float fy = saturate(1.0 - dyW / 130.0);
            float dxW = abs(wq.x - lp.x + n2.x * 6.0);
            float fx = 1.0 - smoothstep(3.0 * artW, 6.0 * artW, dxW);
            float glow = floor(fx * fy * fy * rise * 3.0) / 3.0;
            // Gece telafisi (ışık pass'i sahneyi ambient'la çarpıyor) AMA
            // EKLEMELİ BÜTÇE TAVANIYLA: 0.45. İlk telafi (×4.5) tavansızdı,
            // katkı 1.6'ya fırladı, piksel doydu ve basamaklı turuncu
            // DİKDÖRTGEN çıktı — beyaza doyma hatasının üçüncü tekrarı,
            // bu kez renkli. Tavan, kural: eklemeli katkı 0.45'i AŞAMAZ.
            float amt = glow * 0.35 * Li * u.n.z * (1.0 + u.n.z * 2.5);
            amt = min(amt, 0.45);
            if (amt > 0.0)
                col += float3(1.0, 0.72, 0.42) * (amt * (1.0 - bshad));
        }
    }

    // Renk kademesi: paletli görünüm. 0 = kapalı (serbest gradyan).
    if (u.g.y > 1.5) {
        float n = u.g.y - 1.0;
        col = round(saturate(col) * n) / n;
    }
    return float4(saturate(col), 1.0);
}
