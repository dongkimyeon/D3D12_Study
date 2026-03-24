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

	UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(OBJVertex));
	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vertexBufferSize, 1, 1, 1,
								DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(OBJVertex);
	vbView.SizeInBytes = vertexBufferSize;

	UpdateVertexBuffer();

	UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
	D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, indexBufferSize, 1, 1, 1,
								DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &iRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));

	void* iData;
	indexBuffer->Map(0, nullptr, &iData);
	memcpy(iData, indices.data(), indexBufferSize);
	indexBuffer->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = indexBufferSize;
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


