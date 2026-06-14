#include "HeliTale.h"

HeliTale::HeliTale()
{
	rotationSpeed = XMConvertToRadians(360);
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

