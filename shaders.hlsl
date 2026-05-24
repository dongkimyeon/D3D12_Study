cbuffer ViewProjCB : register(b0)
{
    float4x4 viewProj;
};

struct VSInput
{
    float3 pos    : POS;
    float3 normal : NORMAL;
    float4 col    : COL;
    float4 w0     : INSTANCE_WORLD;
    float4 w1     : INSTANCE_WORLD1;
    float4 w2     : INSTANCE_WORLD2;
    float4 w3     : INSTANCE_WORLD3;
    float4 tint   : INSTANCE_COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

PSInput vs_main(VSInput input)
{
    PSInput output;
    float4x4 world = float4x4(input.w0, input.w1, input.w2, input.w3);
    float4 worldPos = mul(float4(input.pos, 1.0f), world);
    output.position = mul(worldPos, viewProj);
    output.normal   = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.color    = input.col * input.tint;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 N = normalize(input.normal);
    float3 L = normalize(float3(1.0, 1.0, -1.0));
    float ambient = 0.2;
    float diffuse = max(dot(N, L), 0.0) * 0.8;
    float3 litColor = input.color.rgb * saturate(ambient + diffuse);
    return float4(litColor, input.color.a);
}
