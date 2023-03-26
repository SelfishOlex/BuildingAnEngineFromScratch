
struct PixelShaderInput
{
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float4 position : SV_Position;
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

float4 PSMain(PixelShaderInput IN) : SV_TARGET
{
    float3 lightVec = -Lights.m_directionalLight.m_direction;
    float ndotl = max(dot(lightVec, IN.normal), 0.0f);
    float3 lightStrength = Lights.m_directionalLight.m_intensity * ndotl * Lights.m_directionalLight.m_color;

    float distanceFromEye = distance(IN.position, float4(Lights.m_eyePosition, 1));
    float fogEnd = 2000;
    float fogStart = 190;

    float fogFactor = clamp((fogEnd - distanceFromEye) / (fogEnd - fogStart), 0.1, 1);

    //return simpleTexture.Sample(textureSampler, IN.uv) * ndotl;
    float4 color = (simpleTexture.Sample(textureSampler, IN.uv) + float4(Lights.m_directionalLight.m_color * ndotl /** Lights.m_directionalLight.m_intensity*/, 1)) * fogFactor;
    color.w = 1;
    return color;
    /*return simpleTexture.Sample(textureSampler, IN.uv) + float4(lightStrength, 1);*/
    //return float4(Lights.m_directionalLight.m_color, 1);
    //return float4(Lights.m_eyePosition), 1);
}



struct ModelViewProjection
{
    matrix MVP;
};

// this line required shader model 5.1
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD, float3 normal    : NORMAL)
{
    PixelShaderInput result;
    
    // transform to world space
    result.Position = mul(ModelViewProjectionCB.MVP, float4(position, 1.0f));
    result.uv = uv;
    result.normal = normalize(mul((float3x3)ModelViewProjectionCB.MVP, normal));

    return result;
}

