// flame.frag — EŞİKLİ ALEV (brief B.3 kanonik tarifi: Febucci 2019 + DOOM PSX).
// Alev dili çizilmiş bir şekil DEĞİL, gezinen bir eşiktir: sabit yakıt gradyanı
// × yukarı kayan gürültü bir eşiği geçince silüet doğar; dilin kopuşu bedava.
// Beş damganın (FLAME/ICE/POISON/VENOM/PINK) ortak temeli — rampa değişir.
//
// Bağlama (r2d.h SPRITE-FX sözleşmesi):
//   t0/s0 = GÜRÜLTÜ dokusu (fxtex_noise, REPEAT işaretli — çizim bu dokuyla
//           yapılır: r2d_draw_texture(noiseTex, ...))
//   t1/s1 = renk RAMPASI (fxtex_ramp, NEAREST + genişlik = kademe sayısı →
//           rampanın kendisi pixel-art merdivenini verir, ekstra kuantalama yok)
//   FxU p[8] (b0, space3):
//     p[0].x = zaman (sn, DUVAR saati)
//     p[0].y = akış frekansı Hz (12-15: brief "adım başına 1 sanat pikseli")
//     p[0].z/.w = gürültü uv ölçeği x/y
//     p[1].x = eşik CUT (0.30 tipik; yağmurda yükseltilebilir)
//     p[1].y = nefes 0..1 (fx_breath — partikül/ısı/ışıkla AYNI osilatör,
//              GDD §6.6 Karar 2: ayrı hesaplanırsa üç katman bağımsız titrer)
//     p[1].z/.w = quad'ın sanat pikseli sayısı w/h (uv bu ızgaraya çivilenir —
//              pixel-art disiplini: alev sanat ızgarasında yaşar, ara değer yok)
//     p[2].x = yatay daraltma (1.6 tipik; büyükçe = ince alev)
//     p[2].y = parlaklık çarpanı (ADD katmanı — 1 tipik)
//     p[2].z = faz kayması (istasyon başına farklı: her ocak farklı görünür)
//     p[2].w = boş
//
// select() YOK (HANDOVER kuralı) — açık ?: .

Texture2D<float4> Noise : register(t0, space2);
SamplerState      NSmp  : register(s0, space2);
Texture2D<float4> Ramp  : register(t1, space2);
SamplerState      RSmp  : register(s1, space2);

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
    // 0) SANAT IZGARASI: uv quad'ın sanat piksel sayısına çivilenir. Bu
    // olmadan alev dünya pikselinde "kaynar" ve pixel-art'tan kopar (brief:
    // "kaymayı da sanat pikseline yuvarla").
    float2 grid = float2(p[1].z, p[1].w);
    float2 q = (floor(i.uv * grid) + 0.5) / grid;

    // Zaman da kuantalanır: 12-15 Hz'de adım adım akış — sürekli akış
    // "akmıyor, kaynıyor" görünür (brief B.3 pixel-art tuzağı).
    float tq = floor(p[0].x * p[0].y) / p[0].y;

    // 1) YAKIT alanı: alt sıcak (v=1 quad'ın altı), üst soğuk; yukarı doğru
    // daralır (uçlar dil dil kopar, taban geniş yanar). En alt sıralar da
    // HAFİF daralır: quad'ın dibi düz bir çizgi değil yuvarlak bir yatak
    // olsun (Hakan testi: "alev kampı kapatmış" — taban odunla buluşurken
    // köşeli durmasın).
    float narrow = p[2].x * (1.0 + (1.0 - q.y) * 1.2
                                 + saturate((q.y - 0.86) * 5.0) * 0.9);
    float fuel = saturate(q.y * 1.15) * saturate(1.0 - abs(q.x - 0.5) * narrow);

    // 2) YUKARI KAYAN gürültü, iki oktav iki hızda (tek katman "nefes alan
    // leke" olur, iki katman dil üretir). Faz kayması ocak başına.
    float2 nuv1 = q * float2(p[0].z, p[0].w) + float2(p[2].z, -tq * 0.55);
    float2 nuv2 = q * float2(p[0].z * 1.9, p[0].w * 1.9)
                + float2(-p[2].z * 0.7, -tq * 0.83);
    float n = dot(Noise.Sample(NSmp, nuv1), float4(0.40, 0.27, 0.15, 0.08)) * 0.72
            + Noise.Sample(NSmp, nuv2).g * 0.28;

    // 3) EŞİK: ısı = yakıt × gürültü × (taban + nefes). Nefes kabarınca alev
    // kabarır — ısı bükülmesi ve ışıkla aynı kaynaktan (fx_breath).
    float heat = fuel * n * (2.1 + 0.7 * p[1].y);
    if (heat < p[1].x)
        return float4(0.0, 0.0, 0.0, 0.0);   // silüet burada doğuyor (ADD: 0 = yok)

    // 4) RENK: ısı rampadan okunur. Rampa NEAREST + N texel olduğu için
    // merdiven bedava (DOOM ateşi deseni: tek kanal ısı + 1D palet).
    // Uçlar (eşiğe yakın) rampanın soğuk ucu, çekirdek sıcak ucu — burn
    // edge ayrı bir band değil, merdivenin kendisi.
    float rt = saturate((heat - p[1].x) / (1.6 - p[1].x));
    float3 col = Ramp.Sample(RSmp, float2(rt, 0.5)).rgb;
    return float4(col * p[2].y, 1.0) * i.col;
}
