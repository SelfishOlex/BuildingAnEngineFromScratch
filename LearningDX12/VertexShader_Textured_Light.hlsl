struct ModelViewProjection
{
    matrix MVP;
};

// this line required shader model 5.1
ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct VertexPosColor
{
    float3 Position  : POSITION;
    float2 uv        : TEXCOORD;
    float3 normal    : NORMAL;
};

struct VertexShaderOutput
{
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
    float4 Position : SV_Position; // it seems like this needs to be last

    // SV_Position: https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics#system-value-semantics
    // When SV_Position is declared for input to a shader, it can have one of two interpolation modes specified:
    // linearNoPerspective or linearNoPerspectiveCentroid, where the latter causes centroid-snapped xyzw values
    // to be provided when multisample antialiasing. When used in a shader, SV_Position describes the pixel location.
    // Available in all shaders to get the pixel center with a 0.5 offset.
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    // transform to world space
    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.uv = IN.uv;
    OUT.normal = normalize(mul((float3x3)ModelViewProjectionCB.MVP, IN.normal));

    return OUT;
}
