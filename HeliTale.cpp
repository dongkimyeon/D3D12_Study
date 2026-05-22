#include "HeliTale.h"

HeliTale::HeliTale()
{
	rotationSpeed = XMConvertToRadians(360); // 초당 360도 회전
}

HeliTale::~HeliTale()
{
}

void HeliTale::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("Helicopter/HeliTail.obj", device);
	
	scale = { 0.01f, 0.01f, 0.01f };
}

void HeliTale::Update(float dt)
{
	GameObject::Update(dt);
	 
	//rotation.y += rotationSpeed * dt;

}

void HeliTale::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
