#include "Gizumo.h"

Gizumo::Gizumo()
{
}

void Gizumo::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);

	SetPosition(0.0f, 0.0f, 0.0f);

	vertices = {
		{ 100.0f, 0.0f,  0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f }, 
		{  0.0f, 100.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f }, 
		{  0.0f, 0.0f,  100.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f }, 
		{  -100.0f, 0.0f,  0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },
		{  0.0f, -100.0f,  0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f },
		{  0.0f, 0.0f,  -100.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },
	};

	indices = {
		3, 0,
		4, 1,
		5, 2
	};

	CreateBuffersFromData(device);
}

void Gizumo::Update(float dt)
{
	GameObject::Update(dt);
}

void Gizumo::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	GameObject::Render(commandList, view, proj);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

