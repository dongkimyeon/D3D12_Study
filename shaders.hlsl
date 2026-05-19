cbuffer Constants : register(b0)
{
    // 더 이상 C++에서 단일 색상을 받지 않으므로 제거하거나 무시하도록 처리
    // 행렬의 바이트 정렬을 맞추기 위해 더미 변수로 두셔도 무방합니다.
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

    // 두 방향광: L1은 우상단, L2는 전방+살짝 위 — 4개 벽면 방향이 모두 다른 밝기를 가짐
    float3 L1 = normalize(float3(1.0, 0.8,  0.0));
    float3 L2 = normalize(float3(0.0, 0.3,  1.0));
    float lighting = 0.10
        + max(dot(N, L1), 0.0) * 0.65
        + max(dot(N, L2), 0.0) * 0.45;

    float3 litColor = input.color.rgb * saturate(lighting);

    // 지수 안개 — 가까운 벽은 밝고 먼 벽은 어두워져 원근감 강화
    float viewDist  = 1.0 / input.position.w;
    float3 fogColor = float3(0.01, 0.01, 0.02);
    float3 finalColor = lerp(fogColor, litColor, saturate(exp(-viewDist * 0.06)));

    return float4(finalColor, input.color.a);
}