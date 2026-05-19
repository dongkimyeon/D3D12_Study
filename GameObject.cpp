#include "stdafx.h"
#include "GameObject.h"

GameObject::GameObject()
{
    position = { 0, 0, 0 };
	worldMatrix = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
	XMMATRIX temp = XMLoadFloat4x4(&worldMatrix);
	temp = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix, temp);

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
	// 1. 현재 프레임의 SRT(Scale, Rotation, Translation) 행렬을 생성합니다.
	XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX mRot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX mTrans = XMMatrixTranslation(position.x, position.y, position.z);

	// 2. 행렬 결합 (S -> R -> T 순서가 일반적입니다)
	// 기존 worldMatrix를 곱하지 않고 새로 계산하여 '대입'합니다.
	XMMATRIX world = mScale * mRot * mTrans;

	// 3. XMFLOAT4X4에 저장 (CPU 메모리 구조 그대로 저장)
	// 여기서 Transpose를 하지 마세요. 연산은 Row-major 상태로 유지하는 게 편합니다.
	XMStoreFloat4x4(&worldMatrix, world);
}


void GameObject::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (indices.empty()) return; // 로드된 메쉬가 없다면 그리지 않음

	// 업로드 힙 → 디폴트 힙 복사 (더티 플래그가 있을 때만 실행)
	UploadBufferIfDirty(commandList, vertexBuffer, vertexBufferUpload,
		mVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		vertices.size() * sizeof(OBJVertex), mVBDirty);
	UploadBufferIfDirty(commandList, indexBuffer, indexBufferUpload,
		mIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indices.size() * sizeof(uint16_t), mIBDirty);
	if (normalIndexCount > 0 && mNormalsDirty)
	{
		// VB와 IB를 같은 더티 조건으로 함께 업로드
		bool vbDirty = true, ibDirty = true;
		UploadBufferIfDirty(commandList, normalVertexBuffer, normalVertexBufferUpload,
			mNVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
			normalIndexCount * sizeof(OBJVertex), vbDirty);
		UploadBufferIfDirty(commandList, normalIndexBuffer, normalIndexBufferUpload,
			mNIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
			normalIndexCount * sizeof(uint16_t), ibDirty);
		mNormalsDirty = false;
	}

	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	XMMATRIX mvp =  world * view * proj;
	XMFLOAT4X4 mvpFloat;
	XMStoreFloat4x4(&mvpFloat, XMMatrixTranspose(mvp));

	// dummyColor slot: white (1,1,1,1) keeps vertex colors unchanged
	// (Cube::Render overrides this with a per-instance tint color)
	static const float white[4] = { 1.f, 1.f, 1.f, 1.f };
	commandList->SetGraphicsRoot32BitConstants(0, 4,  white,             0);
	commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpFloat.m[0][0], 4);

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);

	// ============================================
	// 법선 벡터 렌더링
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

    D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

    // ---------------------------------
    // 정점 버퍼 생성 (업로드 스테이징 + 디폴트 힙 GPU 버퍼)
    UINT vbSize = static_cast<UINT>(vertices.size() * sizeof(OBJVertex));
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize, 1, 1, 1,
                                 DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload));
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&vertexBuffer));

    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes = sizeof(OBJVertex);
    vbView.SizeInBytes = vbSize;
    mVBState = D3D12_RESOURCE_STATE_COPY_DEST;

    UpdateVertexBuffer(); // 업로드 힙에 복사 + 더티 플래그 설정

    // ---------------------------------
    // 인덱스 버퍼 생성 (업로드 스테이징 + 디폴트 힙 GPU 버퍼)
    UINT ibSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize, 1, 1, 1,
                                 DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

    device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBufferUpload));
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&indexBuffer));

    void* iData;
    indexBufferUpload->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), ibSize);
    indexBufferUpload->Unmap(0, nullptr);

    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = ibSize;
    mIBState = D3D12_RESOURCE_STATE_COPY_DEST;
    mIBDirty = true;
}


void GameObject::CreateBuffersFromData(ComPtr<ID3D12Device> device)
{
    D3D12_HEAP_PROPERTIES uploadHeap  = { D3D12_HEAP_TYPE_UPLOAD };
    D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

    UINT vbSize = static_cast<UINT>(vertices.size() * sizeof(OBJVertex));
    D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize, 1, 1, 1,
                                 DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&uploadHeap,  D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBufferUpload));
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vRes,
        D3D12_RESOURCE_STATE_COPY_DEST,    nullptr, IID_PPV_ARGS(&vertexBuffer));
    vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
    vbView.StrideInBytes  = sizeof(OBJVertex);
    vbView.SizeInBytes    = vbSize;
    mVBState = D3D12_RESOURCE_STATE_COPY_DEST;
    UpdateVertexBuffer();

    UINT ibSize = static_cast<UINT>(indices.size() * sizeof(uint16_t));
    D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize, 1, 1, 1,
                                 DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
    device->CreateCommittedResource(&uploadHeap,  D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBufferUpload));
    device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &iRes,
        D3D12_RESOURCE_STATE_COPY_DEST,    nullptr, IID_PPV_ARGS(&indexBuffer));
    void* iData;
    indexBufferUpload->Map(0, nullptr, &iData);
    memcpy(iData, indices.data(), ibSize);
    indexBufferUpload->Unmap(0, nullptr);
    ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
    ibView.Format         = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes    = ibSize;
    mIBState = D3D12_RESOURCE_STATE_COPY_DEST;
    mIBDirty  = true;
}

void GameObject::UpdateVertexBuffer()
{
    if (vertexBufferUpload && !vertices.empty())
    {
        void* vData;
        vertexBufferUpload->Map(0, nullptr, &vData);
        memcpy(vData, vertices.data(), vertices.size() * sizeof(OBJVertex));
        vertexBufferUpload->Unmap(0, nullptr);
        mVBDirty = true;
    }
}

void GameObject::UploadBufferIfDirty(
    ComPtr<ID3D12GraphicsCommandList>& cmdList,
    ComPtr<ID3D12Resource>& gpuBuf,
    ComPtr<ID3D12Resource>& uploadBuf,
    D3D12_RESOURCE_STATES& currentState,
    D3D12_RESOURCE_STATES targetState,
    UINT64 byteSize,
    bool& dirty)
{
    if (!dirty || !gpuBuf || !uploadBuf) return;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = gpuBuf.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // 이미 다른 상태라면 먼저 COPY_DEST로 전환
    if (currentState != D3D12_RESOURCE_STATE_COPY_DEST)
    {
        barrier.Transition.StateBefore = currentState;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        cmdList->ResourceBarrier(1, &barrier);
    }

    cmdList->CopyBufferRegion(gpuBuf.Get(), 0, uploadBuf.Get(), 0, byteSize);

    // 복사 후 렌더링에 사용할 상태로 전환
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = targetState;
    cmdList->ResourceBarrier(1, &barrier);

    currentState = targetState;
    dirty = false;
}


void GameObject::SetAlpha(float alpha)
{
	for (auto& v : vertices)
		v.a = alpha;
	UpdateVertexBuffer();
}

void GameObject::BakeRotationX(float angleDeg)
{
	float rad = XMConvertToRadians(angleDeg);
	float cosA = cosf(rad);
	float sinA = sinf(rad);
	for (auto& v : vertices) {
		float y = v.y, z = v.z;
		v.y = cosA * y - sinA * z;
		v.z = sinA * y + cosA * z;
		float ny = v.ny, nz = v.nz;
		v.ny = cosA * ny - sinA * nz;
		v.nz = sinA * ny + cosA * nz;
	}
	UpdateVertexBuffer();
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
	// 법선 정점/인덱스 버퍼 생성 (업로드 스테이징 + 디폴트 힙 GPU 버퍼)
	D3D12_HEAP_PROPERTIES uploadHeap  = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

	UINT nvbSize = static_cast<UINT>(normalVertices.size() * sizeof(OBJVertex));
	D3D12_RESOURCE_DESC vbRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, nvbSize, 1, 1, 1,
								  DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	device->CreateCommittedResource(&uploadHeap,  D3D12_HEAP_FLAG_NONE, &vbRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalVertexBufferUpload));
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vbRes,
		D3D12_RESOURCE_STATE_COPY_DEST,    nullptr, IID_PPV_ARGS(&normalVertexBuffer));

	void* vbData;
	normalVertexBufferUpload->Map(0, nullptr, &vbData);
	memcpy(vbData, normalVertices.data(), nvbSize);
	normalVertexBufferUpload->Unmap(0, nullptr);

	normalVbView.BufferLocation = normalVertexBuffer->GetGPUVirtualAddress();
	normalVbView.StrideInBytes = sizeof(OBJVertex);
	normalVbView.SizeInBytes = nvbSize;
	mNVBState = D3D12_RESOURCE_STATE_COPY_DEST;

	UINT nibSize = static_cast<UINT>(normalIndices.size() * sizeof(uint16_t));
	D3D12_RESOURCE_DESC ibRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, nibSize, 1, 1, 1,
								  DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	device->CreateCommittedResource(&uploadHeap,  D3D12_HEAP_FLAG_NONE, &ibRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalIndexBufferUpload));
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &ibRes,
		D3D12_RESOURCE_STATE_COPY_DEST,    nullptr, IID_PPV_ARGS(&normalIndexBuffer));

	void* ibData;
	normalIndexBufferUpload->Map(0, nullptr, &ibData);
	memcpy(ibData, normalIndices.data(), nibSize);
	normalIndexBufferUpload->Unmap(0, nullptr);

	normalIbView.BufferLocation = normalIndexBuffer->GetGPUVirtualAddress();
	normalIbView.Format = DXGI_FORMAT_R16_UINT;
	normalIbView.SizeInBytes = nibSize;
	mNIBState = D3D12_RESOURCE_STATE_COPY_DEST;
	mNormalsDirty = true;
}