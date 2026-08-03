// sprite.frag — genel çizim: doku örneği × vertex rengi.
// Bağlama: fragment doku/sampler space2 (bkz. sprite.vert.hlsl başlığı).

Texture2D<float4> Tex : register(t0, space2);
SamplerState      Smp : register(s0, space2);

struct PSIn {
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : TEXCOORD1;
};

float4 main(PSIn i) : SV_Target0
{
    return Tex.Sample(Smp, i.uv) * i.col;
}
