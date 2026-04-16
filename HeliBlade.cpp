#include "HeliBlade.h"

HeliBlade::HeliBlade()
{
}

HeliBlade::~HeliBlade()
{
}

void HeliBlade::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("Helicopter/HeliBlade.obj", device);

	scale = { 0.01f, 0.01f, 0.01f };


}

void HeliBlade::Update(float dt)
{
	GameObject::Update(dt);
}

void HeliBlade::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
