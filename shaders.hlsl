cbuffer Constants : register(b0)
{
    float4 color;
    float4x4 worldViewProj; // 추가: MVP 행렬
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
    output.color = input.col;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(1, 1, -1));
    float ndotl = max(dot(input.normal, lightDir), 0.3f);
    return float4(input.color.rgb * ndotl * color.rgb, 1.0f);
}