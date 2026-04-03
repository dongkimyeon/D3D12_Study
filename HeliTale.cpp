#include "HeliTale.h"

HeliTale::HeliTale()
{
}

HeliTale::~HeliTale()
{
}

void HeliTale::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("Helicopter/HeliTail.obj", device);

	scale = { 0.05f, 0.05f, 0.05f }; 

}	

void HeliTale::Update(float dt)
{
	GameObject::Update(dt);
}

void HeliTale::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
