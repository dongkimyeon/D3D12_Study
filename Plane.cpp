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
