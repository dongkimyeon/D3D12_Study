#include "stdafx.h"
#include "Cube.h"
#include <random>

Cube::Cube()
{
}

Cube::~Cube()
{
}

void Cube::Initialize(ComPtr<ID3D12Device> device)
{
    GameObject::Initialize(device);

    // 모델을 불러오면 내부에 vertexBuffer가 일차적으로 생성됨
    LoadFromOBJ("model.obj", device);

    // ---------------------------------------------------------
    // 각 정점마다 고유의 랜덤 색상을 생성하여 재설정
    // ---------------------------------------------------------
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    for (auto& v : vertices)
    {
        v.r = dis(gen);
        v.g = dis(gen);
        v.b = dis(gen);
        v.a = 1.0f;
    }

    // 변경된 정점 데이터를 VRAM 버퍼에 갱신 (핵심!)
    UpdateVertexBuffer();
}

void Cube::Update(float dt)
{
    GameObject::Update(dt);
}

void Cube::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    // 루트 상수(Constant)로 전달하던 컬러 코드는 더 이상 사용하지 않음 (쉐이더가 정점 색상을 직접 읽어야 함)
    // commandList->SetGraphicsRoot32BitConstants(0, 4, color.data(), 0);

    // 부모 클래스의 기본 렌더링(메쉬 그리기) 호출
    GameObject::Render(commandList, view, proj);
}