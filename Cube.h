#pragma once
#include "GameObject.h"

class Cube : public GameObject
{
public:
    Cube();
    virtual ~Cube();

    virtual void Initialize(ComPtr<ID3D12Device> device) override;
    virtual void Update(float dt) override;

    // Render 함수 오버라이딩 추가
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

private:
    // 큐브 고유의 색상 (랜덤으로 결정)
	std::array<float, 4> color;
};