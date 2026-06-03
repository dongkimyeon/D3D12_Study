cbuffer Constants : register(b0)
{
    float4x4 viewProj;            // offset  0-15
    float3   camPos;              // offset 16-18
    float    pad0;                // offset 19
    float3   pointLightPos;       // offset 20-22
    float    pointLightRange;     // offset 23
    float3   pointLightColor;     // offset 24-26
    float    pointLightIntensity; // offset 27
    float3   dirLightDir;         // offset 28-30
    float    pad1;                // offset 31
    float3   dirLightColor;       // offset 32-34
    float    dirLightIntensity;   // offset 35
    float    ambientIntensity;    // offset 36
    float3   pad2;                // offset 37-39
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
    float4 svPos    : SV_POSITION;
    float3 worldPos : TEXCOORD0;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
};

PSInput vs_main(VSInput input)
{
    float4x4 world = float4x4(input.iRow0, input.iRow1, input.iRow2, input.iRow3);

    PSInput output;
    float4 wPos     = mul(float4(input.pos, 1.0f), world);
    output.svPos    = mul(wPos, viewProj);
    output.worldPos = wPos.xyz;
    output.normal   = normalize(mul(float4(input.normal, 0.0f), world).xyz);
    output.color    = input.col;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 n = normalize(input.normal);
    float3 v = normalize(camPos - input.worldPos);

    // --- Directional light ---
    float3 L     = normalize(dirLightDir);
    float  ndotl = max(dot(n, L), 0.0f);
    float3 diff  = dirLightColor * dirLightIntensity * ndotl;

    float3 H     = normalize(L + v);
    float  spec  = pow(max(dot(n, H), 0.0f), 64.0f);
    float3 spec3 = dirLightColor * dirLightIntensity * spec * 0.5f;

    // --- Point light ---
    float3 toLight   = pointLightPos - input.worldPos;
    float  dist      = length(toLight);
    float3 Lp        = toLight / dist;
    float  atten     = saturate(1.0f - dist / pointLightRange);
    atten = atten * atten;
    float  ndotlp    = max(dot(n, Lp), 0.0f);
    float3 pointDiff = pointLightColor * ndotlp * atten * pointLightIntensity;

    float3 Hp        = normalize(Lp + v);
    float  specp     = pow(max(dot(n, Hp), 0.0f), 128.0f);
    float3 pointSpec = pointLightColor * specp * atten * pointLightIntensity * 0.6f;

    // --- Ambient ---
    float3 ambient = float3(1.0f, 1.0f, 1.0f) * ambientIntensity;

    float3 final = input.color.rgb * (ambient + diff + spec3 + pointDiff + pointSpec);
    return float4(final, input.color.a);
}
