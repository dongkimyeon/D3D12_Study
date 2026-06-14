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
	LoadFromOBJ("Helicopter/HeliBody.obj", device);

	scale = { 0.01f, 0.01f, 0.01f };
}

