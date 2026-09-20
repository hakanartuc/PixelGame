// dissolve.frag — SPRITE-FX yolunun İLK shader'ı ve altyapı üçlüsünün
// doğrulama aracı: FxU uniform payload'ı + 3 doku slotu + REPEAT kaydırmalı
// gürültü + rampa örneklemesi tek geçişte sınanıyor. Kalıcı kullanım adayı:
// çözülme/yanma ölümü (GDD §6.6 gövde efektleri — imza kararından sonra).
//
// Bağlama (r2d.h SPRITE-FX sözleşmesi):
//   t0/s0 = çizilen sprite · t1/s1 = gürültü (REPEAT) · t2/s2 = kenar rampası
//   FxU p[8] (b0, space3):
//     p[0].x = eşik 0..1 (0 = sağlam, 1 = tamamen çözülmüş)
//     p[0].y = kenar bandı genişliği (0.06 tipik)
//     p[0].z = gürültü uv ölçeği (sprite uv'si × bu; 1 tipik)
//     p[0].w = zaman (sn) — gürültü kayması için
//     p[1].xy = gürültü kayma hızı (uv/sn)
//     p[1].z = kenar bandı kademe sayısı (2-3 pixel-art; 0 = kademesiz)
//     p[2].rgb = rampa yoksa düz kenar rengi · p[2].w = kenar parlaklık çarpanı
//
// select() YOK (HANDOVER kuralı: HLSL/MSL argüman sırası tuzağı) — açık ?: .

Texture2D<float4> Tex   : register(t0, space2);
SamplerState      Smp   : register(s0, space2);
Texture2D<float4> Noise : register(t1, space2);
SamplerState      NSmp  : register(s1, space2);
Texture2D<float4> Ramp  : register(t2, space2);
SamplerState      RSmp  : register(s2, space2);

cbuffer FxU : register(b0, space3) {
    float4 p[8];
};

struct PSIn {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

float4 main(PSIn i) : SV_Target0
{
    float4 c = Tex.Sample(Smp, i.uv) * i.col;

    // Gürültü: sprite uv'si ölçeklenip zamanla kaydırılır — REPEAT sampler
    // sayesinde 1'i aşan uv baştan döşenir (CLAMP'te kenar pikseli smear olur,
    // bu shader aynı zamanda REPEAT'in görsel testi).
    float2 nuv = i.uv * p[0].z + p[1].xy * p[0].w;
    float4 n4 = Noise.Sample(NSmp, nuv);
    // Dört oktavı FBM ağırlığıyla karıştır (fxtex_noise kanal sözleşmesi)
    float n = dot(n4, float4(0.533, 0.267, 0.133, 0.067));

    float thr  = p[0].x;
    float band = p[0].y;

    // Eşiğin altı yok; eşik + bant arası yanan kenar; üstü sağlam.
    if (n < thr) {
        c.a = 0.0;
        return c;
    }
    float edge = (band > 0.0001) ? saturate((n - thr) / band) : 1.0;
    if (edge < 1.0) {
        // Pixel-art disiplini: kenar KADEMELİ (2-3 basamak), yumuşak degrade değil
        float steps = p[1].z;
        float q = (steps > 0.5) ? floor(edge * steps) / steps : edge;
        // Kenar rengi: rampa dokusu (u = kenar ilerlemesi) × düz renk çarpanı.
        // Rampa bağlanmadıysa slot 1x1 beyazdır → düz renk tek başına kalır.
        float3 rampCol = Ramp.Sample(RSmp, float2(q, 0.5)).rgb;
        float3 edgeCol = rampCol * p[2].rgb * p[2].w;
        // Kenarda sprite rengi söner, yanma rengi doğar (alfa sprite'ın kalır)
        c.rgb = edgeCol * c.a;
        c.a *= 0.55 + 0.45 * q;   // kenar hafif incelir ama kaybolmaz
    }
    return c;
}
