#include "stdafx.h"
#include "GameObject.h"

GameObject::GameObject()
{
    position = { 0, 0, 0 };
    worldMatrix = XMMatrixIdentity();
}

GameObject::~GameObject()
{
}

void GameObject::Initialize(ComPtr<ID3D12Device> device)
{
    // 추가적인 초기화가 필요하면 작성
}

void GameObject::Update(float dt)
{

	
	XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);

	XMMATRIX mRot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);

	XMMATRIX mTrans = XMMatrixTranslation(position.x, position.y, position.z);

	// 2. SRT 순서로 결합하여 월드 행렬 갱신
	worldMatrix = mScale * mRot * mTrans;
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (indices.empty()) return; // 로드된 메쉬가 없다면 그리지 않음

	XMMATRIX mvp = worldMatrix * view * proj;
	XMFLOAT4X4 mvpFloat;
	XMStoreFloat4x4(&mvpFloat, XMMatrixTranspose(mvp));

	// 색상과 변환행렬을 루트 상수로 전달 (루트 파라미터가 0 인덱스 배열 하나로 구성되어 있다고 가정)
	commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpFloat.m[0][0], 4);

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);

	// ============================================
	// 법선 벡터 렌더링 (데이터가 존재하는 경우만)
	if (normalIndexCount > 0)
	{
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &normalVbView);
		commandList->IASetIndexBuffer(&normalIbView);
		commandList->DrawIndexedInstanced(normalIndexCount, 1, 0, 0, 0);

		// 기본 렌더 상태(삼각형)로 복구
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
}

void GameObject::LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device)
{
    OBJLoader::Load(filename, vertices, indices);

    // ---------------------------------
    // 정점 버퍼 생성
    UINT maxVertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(OBJVertex));
    D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxVertexBufferSize, 1, 1, 1,
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer));

    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(OBJVertex);
    vbView.SizeInBytes = maxVertexBufferSize;

    // 데이터를 바로 업로드합니다.
    UpdateVertexBuffer();

    // ---------------------------------
    // 인덱스 버퍼 생성
    UINT maxIndexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, maxIndexBufferSize, 1, 1, 1,
                                DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &iRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer));
    
    void* iData;
    indexBuffer->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), indices.size() * sizeof(uint16_t));
    indexBuffer->Unmap(0, nullptr);

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = maxIndexBufferSize;
}


void GameObject::UpdateVertexBuffer()
{
    if (vertexBuffer && !vertices.empty())
    {
        void* vData;
        // 업로드 힙이므로 직접 Map하여 갱신 가능합니다.
        vertexBuffer->Map(0, nullptr, &vData);
        memcpy(vData, vertices.data(), vertices.size() * sizeof(OBJVertex));
        vertexBuffer->Unmap(0, nullptr);
    }
}


void GameObject::BuildNormalBuffer(ComPtr<ID3D12Device> device)
{
	if (indices.empty() || vertices.empty()) return;

	std::vector<OBJVertex> normalVertices;
	std::vector<uint16_t> normalIndices;

	float normalLength = 0.5f; // 노멀(법선) 선의 길이 설정

	// 정점이 아닌 삼각형(인덱스 3개) 단위로 루프를 돕니다.
	for (size_t i = 0; i < indices.size(); i += 3)
	{
		// 삼각형을 이루는 세 정점 가져오기
		OBJVertex v0 = vertices[indices[i]];
		OBJVertex v1 = vertices[indices[i + 1]];
		OBJVertex v2 = vertices[indices[i + 2]];

		// 세 정점의 좌표 평균값 (면의 중심 위치)
		float centerX = (v0.x + v1.x + v2.x) / 3.0f;
		float centerY = (v0.y + v1.y + v2.y) / 3.0f;
		float centerZ = (v0.z + v1.z + v2.z) / 3.0f;

		// 세 정점의 법선 평균값 (면의 법선 방향, 필요시 정규화 추가 가능)
		float normalX = (v0.nx + v1.nx + v2.nx) / 3.0f;
		float normalY = (v0.ny + v1.ny + v2.ny) / 3.0f;
		float normalZ = (v0.nz + v1.nz + v2.nz) / 3.0f;

		// 시작점: 면의 중심, 색상은 주황색(R=1, G=0.5, B=0)
		OBJVertex startPoint;
		startPoint.x = centerX; startPoint.y = centerY; startPoint.z = centerZ;
		startPoint.r = 1.0f; startPoint.g = 0.5f; startPoint.b = 0.0f; startPoint.a = 1.0f;
		// 셰이더 등에 영향을 주지 않으려면 nx, ny, nz 등은 0으로 비워둬도 무방합니다.

		// 끝점: 면의 중심 + 면 법선 벡터 * 길이
		OBJVertex endPoint = startPoint;
		endPoint.x += normalX * normalLength;
		endPoint.y += normalY * normalLength;
		endPoint.z += normalZ * normalLength;

		// 버퍼 벡터에 정점 2개 추가
		normalVertices.push_back(startPoint);
		normalVertices.push_back(endPoint);

		// 해당 라인의 시작점, 끝점 인덱스 저장
		normalIndices.push_back(static_cast<uint16_t>(normalVertices.size() - 2));
		normalIndices.push_back(static_cast<uint16_t>(normalVertices.size() - 1));
	}

	normalIndexCount = static_cast<UINT>(normalIndices.size());

	// ---------------------------------
	// 법선 정점/인덱스 버퍼 생성
	UINT vbSize = static_cast<UINT>(normalVertices.size() * sizeof(OBJVertex));
	D3D12_HEAP_PROPERTIES heap = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_RESOURCE_DESC vbRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize, 1, 1, 1,
								  DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &vbRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalVertexBuffer));

	normalVbView.BufferLocation = normalVertexBuffer->GetGPUVirtualAddress();
	normalVbView.StrideInBytes = sizeof(OBJVertex);
	normalVbView.SizeInBytes = vbSize;

	void* vbData;
	normalVertexBuffer->Map(0, nullptr, &vbData);
	memcpy(vbData, normalVertices.data(), vbSize);
	normalVertexBuffer->Unmap(0, nullptr);

	UINT ibSize = static_cast<UINT>(normalIndices.size() * sizeof(uint16_t));
	D3D12_RESOURCE_DESC ibRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize, 1, 1, 1,
								  DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &ibRes, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalIndexBuffer));

	void* ibData;
	normalIndexBuffer->Map(0, nullptr, &ibData);
	memcpy(ibData, normalIndices.data(), ibSize);
	normalIndexBuffer->Unmap(0, nullptr);

	normalIbView.BufferLocation = normalIndexBuffer->GetGPUVirtualAddress();
	normalIbView.Format = DXGI_FORMAT_R16_UINT;
	normalIbView.SizeInBytes = ibSize;
}