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

