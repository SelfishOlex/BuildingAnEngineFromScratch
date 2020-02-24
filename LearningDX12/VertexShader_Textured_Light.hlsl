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
    float2 uv    : TEXCOORD;
    float4 Position : SV_Position;
};

VertexShaderOutput main(VertexPosColor IN)
{
    VertexShaderOutput OUT;

    OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
    OUT.uv = IN.uv;

    return OUT;
}
