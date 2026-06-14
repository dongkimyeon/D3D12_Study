cbuffer Constants : register(b0)
{
    float4x4 viewProj;
};

struct VSInput
{
    float3 pos    : POS;
    float3 normal : NORMAL;
    float4 col    : COL;
    
    float4 iRow0  : INSTANCE_WORLD0;
    float4 iRow1  : INSTANCE_WORLD1;
    float4 iRow2  : INSTANCE_WORLD2;
    float4 iRow3  : INSTANCE_WORLD3;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

PSInput vs_main(VSInput input)
{
    float4x4 world = float4x4(input.iRow0, input.iRow1, input.iRow2, input.iRow3);

    PSInput output;
    float4 worldPos    = mul(float4(input.pos, 1.0f), world);
    output.position    = mul(worldPos, viewProj);
    output.normal      = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.color       = input.col;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(1, 1, -1));
    float ndotl = max(dot(input.normal, lightDir), 0.3f);
    return float4(input.color.rgb * ndotl, input.color.a);
}
