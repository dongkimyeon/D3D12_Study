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
    // 버텍스 버퍼에 들어있는 개별 정점 색상을 픽셀 쉐이더로 넘김
    output.color = input.col;
    return output;
}

float4 ps_main(PSInput input) : SV_TARGET
{
    float3 lightDir = normalize(float3(1, 1, -1));
    float ndotl = max(dot(input.normal, lightDir), 0.3f);
    
    // 이전에 곱하던 전역 color(현재 항상 0)를 지우고
    // Vertex Shader가 넘겨준 정점 고유의 색상(input.color)값에 명암만 적용합니다.
    return float4(input.color.rgb * ndotl, 1.0f);
}