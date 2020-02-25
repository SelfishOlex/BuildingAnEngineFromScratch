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
    float3 m_direction;
    float m_intensity;
};
struct LightInfo
{
    DirectionalLight m_directionalLight;
    float3 m_eyePosition;
};
ConstantBuffer<LightInfo> Lights : register(b1);

float4 main( PixelShaderInput IN ) : SV_Target
{
    float3 lightVec = -Lights.m_directionalLight.m_direction;
    float ndotl = max(dot(lightVec, IN.normal), 0.0f);
    float3 lightStrength = Lights.m_directionalLight.m_intensity * ndotl * Lights.m_directionalLight.m_color;

    //return simpleTexture.Sample(textureSampler, IN.uv) * ndotl;
    return simpleTexture.Sample(textureSampler, IN.uv) + float4(Lights.m_directionalLight.m_color * ndotl /** Lights.m_directionalLight.m_intensity*/, 1);
    /*return simpleTexture.Sample(textureSampler, IN.uv) + float4(lightStrength, 1);*/
    //return float4(Lights.m_directionalLight.m_color, 1);
    //return float4(Lights.m_eyePosition), 1);
}
