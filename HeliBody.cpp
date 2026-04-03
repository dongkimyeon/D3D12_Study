#include "HeliBody.h"

HeliBody::HeliBody()
{
}

HeliBody::~HeliBody()
{
}

void HeliBody::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("Helicopter/Helibody.obj", device);
	scale = { 0.05f, 0.05f, 0.05f };

}

void HeliBody::Update(float dt)
{
	GameObject::Update(dt);
}

void HeliBody::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
