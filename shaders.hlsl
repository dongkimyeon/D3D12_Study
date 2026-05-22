cbuffer Constants : register(b0)
{
    float4 dummyColor; // 기존의 color. 
    float4x4 worldViewProj;
};

struct VSInput
{
    float3 pos : POS;
    float3 normal : NORMAL;
    float4 col : COL;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

PSInput vs_main(VSInput input)
{
    PSInput output;
    output.position = mul(float4(input.pos, 1.0f), worldViewProj);
    output.normal = input.normal;
    // dummyColor is (1,1,1,1) for normal objects, per-instance tint for Cube
    output.color = input.col * dummyColor;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);

    float3 L = normalize(float3(1.0, 1.0, -1.0));
    float ambient = 0.2;
    float diffuse = max(dot(N, L), 0.0) * 0.8;
    float lighting = ambient + diffuse;

    float3 litColor = input.color.rgb * saturate(lighting);

    return float4(litColor, input.color.a);
}