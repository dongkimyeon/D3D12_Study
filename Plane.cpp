#include "Plane.h"

Plane::Plane()
{

}

Plane::~Plane()
{

}

void Plane::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	
	
	vertices = {
		{ -5.0f, 0.0f, -5.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f }, // 좌하단
		{ -5.0f, 0.0f,  5.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f }, // 좌상단
		{  5.0f, 0.0f,  5.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f }, // 우상단
		{  5.0f, 0.0f, -5.0f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f, 1.0f }  // 우하단
	};


	indices = {
		0, 1, 2,
		0, 2, 3
	};

	CreateBuffersFromData(device);
	BuildNormalBuffer(device);

}

void Plane::Update(float dt)
{
	GameObject::Update(dt);
}

void Plane::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
