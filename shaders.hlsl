// 버텍스 셰이더 입력 구조체
// OpenGL: layout(location = 0) in vec2 pos; 와 유사
// HLSL: 시맨틱(Semantic)을 사용하여 속성 매핑
struct VS_Input
{
    float2 pos : POS;      // POS 시맨틱 - C++ 코드의 INPUT_ELEMENT_DESC와 매칭
    float4 color : COL;    // COL 시맨틱
};

// 버텍스 셰이더 출력 / 픽셀 셰이더 입력 구조체
struct VS_Output
{
    float4 position : SV_POSITION;  // SV_POSITION: 시스템 값(System Value) - 클립 공간 좌표
                                     // OpenGL의 gl_Position과 동일
    float4 color : COL;              // 픽셀 셰이더로 전달될 색상 (자동 보간됨)
};


// 버텍스 셰이더 (Vertex Shader)
// OpenGL의 vertex shader와 동일한 역할
// 각 정점마다 한 번씩 실행되며, 정점을 클립 공간으로 변환
VS_Output vs_main(VS_Input input)
{
    VS_Output output;
    
    // 2D 좌표(xy)를 4D 클립 공간 좌표(xyzw)로 변환
    // z = 0.0 (깊이), w = 1.0 (동차 좌표)
    // OpenGL: gl_Position = vec4(pos, 0.0, 1.0); 와 동일
    output.position = float4(input.pos, 0.0f, 1.0f);
    
    // 색상을 그대로 전달 (래스터라이저가 자동으로 보간)
    output.color = input.color;    

    return output;
}

// 픽셀 셰이더 (Pixel Shader = Fragment Shader)
// OpenGL의 fragment shader와 동일한 역할
// 각 픽셀마다 한 번씩 실행되며, 최종 색상을 결정
// SV_TARGET: 시스템 값 - 렌더 타겟(프레임버퍼)에 출력
// OpenGL의 out vec4 FragColor; 와 동일
float4 ps_main(VS_Output input) : SV_TARGET
{
    // 보간된 색상을 그대로 반환
    // OpenGL: FragColor = input.color; 와 동일
    return input.color;   
}
