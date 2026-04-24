#include "Gizumo.h"


Gizumo::Gizumo()
{
}

void Gizumo::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);

	// 기즈모를 (0, 0, 0) 원점에 배치
	SetPosition(0.0f, 0.0f, 0.0f);

	vertices = {
		{ 100.0f, 0.0f,  0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f }, 
		{  0.0f, 100.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f }, 
		{  0.0f, 0.0f,  100.0f,  0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f }, 
		{  -100.0f, 0.0f,  0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },
		{  0.0f, -100.0f,  0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f },
		{  0.0f, 0.0f,  -100.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f },
	};

	// 면이 아닌 선(Line)을 그리도록 인덱스 수정 (원점에서 각 축의 끝으로)
	indices = {
		3, 0, // 원점 -> X축
		4, 1, // 원점 -> Y축
		5, 2  // 원점 -> Z축
	};

	CreateBuffersFromData(device);
}

void Gizumo::Update(float dt)
{
	GameObject::Update(dt);
}

void Gizumo::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	// 기즈모는 선으로 그려야 하므로 Topology를 LINELIST로 변경
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

	GameObject::Render(commandList, view, proj);

	// 다른 오브젝트들을 그릴 때 영향을 주지 않도록 다시 TRIANGLELIST로 복구
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}


