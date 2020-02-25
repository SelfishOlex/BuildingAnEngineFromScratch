struct PixelShaderInput
{
    float2 uv     : TEXCOORD;
    float3 normal : NORMAL;
};

Texture2D<float4> simpleTexture : register(t0);
SamplerState textureSampler     : register(s0);

// Light info
struct DirectionalLight
{
    float3 m_color;
    float3 m_directionalLight;
    float m_intensity;
};
struct LightInfo
{
    int m_directionalLightCount;
    DirectionalLight m_directionLights[4];

    float3 m_eyePosition;
};
ConstantBuffer<LightInfo> Lights : register(b1);

float4 main( PixelShaderInput IN ) : SV_Target
{
    float3 lightVec = -Lights.m_directionLights[0].m_directionalLight;
    float ndotl = max(dot(lightVec, IN.normal), 0.0f);
    float3 lightStrength = Lights.m_directionLights[0].m_intensity * ndotl * Lights.m_directionLights[0].m_color;

    return simpleTexture.Sample(textureSampler, IN.uv) + float4(lightStrength, 1);
}
