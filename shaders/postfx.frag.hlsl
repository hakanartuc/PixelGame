// postfx.frag — "grade": sahne target'ını tam ekran örnekleyip doygunluk +
// sıcaklık + kontrast/parlaklık + retro modlar + vinyet uygular.
// Düzen R2DPostFXU (r2d.c) ile aynı.
//
// Bağlama: fragment doku/sampler (t0/s0, space2) · uniform buffer (b0, space3).

Texture2D<float4> Tex : register(t0, space2);
SamplerState      Smp : register(s0, space2);

struct PostU {
    float4 a;   // x=doygunluk y=sıcaklık z=kontrast w=parlaklık
    float4 b;   // x=vinyet gücü y=vinyet yarıçapı z=scanline w=gameboy
    float4 c;   // x=posterize y=kromatik sapma z=gren w=zaman (kare sayacı)
    float4 d;   // x=bloom gücü y=bloom eşiği z=bloom yarıçapı w=ısı genliği
    float4 e;   // x=ısı hızı y,z,w=(boş)
};
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

float4 main(PSIn i) : SV_Target0
{
    // ISI BÜKÜLMESİ: uv'yi kaydır. Aşağı doğru güçlenir (sıcak hava yerden kalkar).
    // İki frekans üst üste — tek sinüs akordeon gibi duruyor.
    float2 uv = i.uv;
    if (u.d.w > 0.0) {
        float tt = u.c.w * u.e.x * 0.06;
        float depth = uv.y;                 // 0 üst, 1 alt
        float w1 = sin(uv.y * 38.0 + tt * 3.1) * 0.6;
        float w2 = sin(uv.y * 91.0 - tt * 5.7) * 0.4;
        uv.x += (w1 + w2) * u.d.w * depth;
        uv.y += sin(uv.x * 55.0 + tt * 2.3) * u.d.w * 0.35 * depth;
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
