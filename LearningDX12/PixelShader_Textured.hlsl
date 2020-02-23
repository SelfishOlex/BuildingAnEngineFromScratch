struct PixelShaderInput
{
    float2 uv    : TEXCOORD;
};

Texture2D<float4> simpleTexture : register(t0);
SamplerState textureSampler     : register(s0);

float4 main( PixelShaderInput IN ) : SV_Target
{
    return simpleTexture.Sample(textureSampler, IN.uv);
}
