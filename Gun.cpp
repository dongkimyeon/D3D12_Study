#include "Gun.h"

Gun::Gun()
{
}

Gun::~Gun()
{
}

void Gun::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("AK-74.obj", device);
	BakeScale(0.0015f, 0.0015f, 0.0015f);
	BakeRotation(0.0f, 90.0f, 0.0f);
	for (auto& v : vertices) {
		v.r = 0.0f; v.g = 1.0f; v.b = 0.0f; v.a = 1.0f;
	}
	UpdateVertexBuffer();
}

void Gun::Update(float dt)
{

	GameObject::Update(dt);
}

void Gun::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
