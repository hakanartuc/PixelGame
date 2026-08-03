// sprite.vert — TÜM pass'lerin ortak vertex shader'ı (sprite, ışık, post-fx).
// Piksel uzayındaki konumu NDC'ye çevirir; uv ve rengi olduğu gibi geçirir.
//
// SDL_GPU HLSL bağlama sözleşmesi (kaynak: vendor/SDL/include/SDL3/SDL_gpu.h,
// SDL_CreateGPUShader doküman bloğu):
//   vertex   → doku/sampler (t/s, space0) · uniform buffer (b, space1)
//   fragment → doku/sampler (t/s, space2) · uniform buffer (b, space3)
// Vertex semantikleri TEXCOORD0'dan başlamak ZORUNDA (aynı doküman bloğu):
// D3D12'de sistem-değeri olmayan bütün semantikler TEXCOORD varsayılıyor.
//
// KOORDİNAT SİSTEMİ: SDL_gpu.h:269-287 — arka uçlar arası NDC farkını (Vulkan'da
// +Y aşağı) SDL kendisi çeviriyor, shader'da flip mantığı YOK. Bu yüzden aşağıdaki
// dönüşüm MSL sürümünden birebir kopyadır.

cbuffer VertexUB : register(b0, space1) {
    // xy = hedefin piksel boyutu. zw KULLANILMIYOR — yalnız hizalama dolgusu:
    // 8 baytlık bir uniform buffer sınırda (Vulkan'da UBO bağlama aralığı blok
    // boyutundan küçük kalabilir), float4 üç arka uçta da 16 bayt stride'da güvenli.
    float4 viewport;
};

struct VSIn {
    float2 pos : TEXCOORD0;
    float2 uv  : TEXCOORD1;
    float4 col : TEXCOORD2;
};

struct VSOut {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

VSOut main(VSIn v)
{
    VSOut o;
    float2 ndc = float2(v.pos.x / viewport.x * 2.0 - 1.0,
                        1.0 - v.pos.y / viewport.y * 2.0);
    o.pos = float4(ndc, 0.0, 1.0);
    o.uv  = v.uv;
    o.col = v.col;
    return o;
}
