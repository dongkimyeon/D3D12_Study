#include "stdafx.h"
#include "GameObject.h"
#include "framework.h"

bool                     GameObject::sShowAABB = false;
ComPtr<ID3D12Resource>   GameObject::sAABBVB;
ComPtr<ID3D12Resource>   GameObject::sAABBIB;
ComPtr<ID3D12Resource>   GameObject::sAABBInstBuf;
D3D12_VERTEX_BUFFER_VIEW GameObject::sAABBVbView = {};
D3D12_INDEX_BUFFER_VIEW  GameObject::sAABBIbView = {};
D3D12_VERTEX_BUFFER_VIEW GameObject::sAABBInstView = {};

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
	CreateInstanceBuffer(device, 1);
}

void GameObject::CreateInstanceBuffer(ComPtr<ID3D12Device> device, UINT count)
{
	D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

	UINT64 size = (UINT64)count * sizeof(XMFLOAT4X4);
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, size, 1, 1, 1,
		DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mInstanceBufferUpload));
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mInstanceBuffer));

	mInstanceBufView.BufferLocation = mInstanceBuffer->GetGPUVirtualAddress();
	mInstanceBufView.StrideInBytes = sizeof(XMFLOAT4X4);
	mInstanceBufView.SizeInBytes = (UINT)size;
	mInstState = D3D12_RESOURCE_STATE_COPY_DEST;
	mInstDirty = false;
	mInstanceCount = count;
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
		indices.size() * sizeof(uint32_t), mIBDirty);
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

	// 단일 인스턴스 오브젝트: 매 프레임 현재 worldMatrix를 인스턴스 버퍼에 반영
	if (mInstanceCount == 1 && mInstanceBufferUpload)
	{
		void* ptr;
		mInstanceBufferUpload->Map(0, nullptr, &ptr);
		memcpy(ptr, &worldMatrix, sizeof(XMFLOAT4X4)); // row-major 그대로 저장
		mInstanceBufferUpload->Unmap(0, nullptr);
		mInstDirty = true;
	}
	UploadBufferIfDirty(commandList, mInstanceBuffer, mInstanceBufferUpload,
		mInstState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		mInstanceCount * sizeof(XMFLOAT4X4), mInstDirty);

	XMFLOAT4X4 vpFloat;
	XMStoreFloat4x4(&vpFloat, XMMatrixTranspose(view * proj));
	commandList->SetGraphicsRoot32BitConstants(0, 16, &vpFloat.m[0][0], 0);

	commandList->IASetVertexBuffers(0, 1, &vbView);
	commandList->IASetVertexBuffers(1, 1, &mInstanceBufView);
	commandList->IASetIndexBuffer(&ibView);
	commandList->DrawIndexedInstanced(static_cast<UINT>(indices.size()), mInstanceCount, 0, 0, 0);

	// ============================================
	// 법선 벡터 렌더링
	if (normalIndexCount > 0)
	{
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
		commandList->IASetVertexBuffers(0, 1, &normalVbView);
		commandList->IASetIndexBuffer(&normalIbView);
		commandList->DrawIndexedInstanced(normalIndexCount, 1, 0, 0, 0);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	if (sShowAABB)
		RenderAABB(commandList, view, proj);
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
	UINT ibSize = static_cast<UINT>(indices.size() * sizeof(uint32_t));
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
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = ibSize;
	mIBState = D3D12_RESOURCE_STATE_COPY_DEST;
	mIBDirty = true;
}


void GameObject::CreateBuffersFromData(ComPtr<ID3D12Device> device)
{
	D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

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
	UpdateVertexBuffer();

	UINT ibSize = static_cast<UINT>(indices.size() * sizeof(uint32_t));
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
	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = ibSize;
	mIBState = D3D12_RESOURCE_STATE_COPY_DEST;
	mIBDirty = true;
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


void GameObject::EnsureAABBMesh()
{
	if (sAABBVB != nullptr) return;

	auto device = Framework::GetDevice();

	// ±0.5 단위 박스 (8정점) — 노멀을 조명 방향으로 설정해 최대 밝기 보장
	static const float c = 0.5774f;
	OBJVertex verts[8] = {
		{-0.5f,-0.5f,-0.5f, c,c,-c, 0,1,0,1},
		{ 0.5f,-0.5f,-0.5f, c,c,-c, 0,1,0,1},
		{ 0.5f,-0.5f, 0.5f, c,c,-c, 0,1,0,1},
		{-0.5f,-0.5f, 0.5f, c,c,-c, 0,1,0,1},
		{-0.5f, 0.5f,-0.5f, c,c,-c, 0,1,0,1},
		{ 0.5f, 0.5f,-0.5f, c,c,-c, 0,1,0,1},
		{ 0.5f, 0.5f, 0.5f, c,c,-c, 0,1,0,1},
		{-0.5f, 0.5f, 0.5f, c,c,-c, 0,1,0,1},
	};
	uint16_t inds[24] = {
		0,1, 1,2, 2,3, 3,0,  // 아랫면
		4,5, 5,6, 6,7, 7,4,  // 윗면
		0,4, 1,5, 2,6, 3,7   // 기둥
	};

	D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };

	UINT vbSize = sizeof(verts);
	D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize,
		1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &vRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sAABBVB));
	void* mapped;
	sAABBVB->Map(0, nullptr, &mapped);
	memcpy(mapped, verts, vbSize);
	sAABBVB->Unmap(0, nullptr);
	sAABBVbView.BufferLocation = sAABBVB->GetGPUVirtualAddress();
	sAABBVbView.SizeInBytes = vbSize;
	sAABBVbView.StrideInBytes = sizeof(OBJVertex);

	UINT ibSize = sizeof(inds);
	D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize,
		1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &iRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sAABBIB));
	sAABBIB->Map(0, nullptr, &mapped);
	memcpy(mapped, inds, ibSize);
	sAABBIB->Unmap(0, nullptr);
	sAABBIbView.BufferLocation = sAABBIB->GetGPUVirtualAddress();
	sAABBIbView.Format = DXGI_FORMAT_R16_UINT;
	sAABBIbView.SizeInBytes = ibSize;

	// AABB용 1-인스턴스 버퍼 (업로드 힙, 매 드로우마다 갱신)
	UINT instSize = sizeof(XMFLOAT4X4);
	D3D12_RESOURCE_DESC instRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, instSize,
		1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &instRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sAABBInstBuf));
	sAABBInstView.BufferLocation = sAABBInstBuf->GetGPUVirtualAddress();
	sAABBInstView.StrideInBytes = sizeof(XMFLOAT4X4);
	sAABBInstView.SizeInBytes = instSize;
}

DirectX::BoundingBox GameObject::GetWorldAABB() const
{
	if (vertices.empty()) return {};
	XMFLOAT3 vmin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& v : vertices) {
		vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
		vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
		vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
	}
	DirectX::BoundingBox local(
		{ (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
		{ (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });
	DirectX::BoundingBox world;
	local.Transform(world, XMLoadFloat4x4(&worldMatrix));
	return world;
}

void GameObject::RenderAABB(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	EnsureAABBMesh();

	DirectX::BoundingBox aabb = GetWorldAABB();
	if (aabb.Extents.x == 0 && aabb.Extents.y == 0 && aabb.Extents.z == 0) return;

	XMMATRIX world = XMMatrixScaling(aabb.Extents.x * 2.f, aabb.Extents.y * 2.f, aabb.Extents.z * 2.f)
		* XMMatrixTranslation(aabb.Center.x, aabb.Center.y, aabb.Center.z);

	XMFLOAT4X4 worldData, vpFloat;
	XMStoreFloat4x4(&worldData, world);
	XMStoreFloat4x4(&vpFloat, XMMatrixTranspose(view * proj));

	void* ptr;
	sAABBInstBuf->Map(0, nullptr, &ptr);
	memcpy(ptr, &worldData, sizeof(XMFLOAT4X4));
	sAABBInstBuf->Unmap(0, nullptr);

	commandList->SetGraphicsRoot32BitConstants(0, 16, &vpFloat.m[0][0], 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->IASetVertexBuffers(0, 1, &sAABBVbView);
	commandList->IASetVertexBuffers(1, 1, &sAABBInstView);
	commandList->IASetIndexBuffer(&sAABBIbView);
	commandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void GameObject::SetAlpha(float alpha)
{
	for (auto& v : vertices)
		v.a = alpha;
	UpdateVertexBuffer();
}

void GameObject::BakeScale(float sx, float sy, float sz)
{
	for (auto& v : vertices) {
		v.x *= sx; v.y *= sy; v.z *= sz;
	}
	UpdateVertexBuffer();
}

void GameObject::BakeRotation(float pitch, float yaw, float roll)
{
	XMMATRIX rot = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(pitch),
		XMConvertToRadians(yaw),
		XMConvertToRadians(roll));
	for (auto& v : vertices) {
		XMVECTOR pos = XMVector3TransformCoord(XMVectorSet(v.x, v.y, v.z, 1.f), rot);
		XMVECTOR nor = XMVector3TransformNormal(XMVectorSet(v.nx, v.ny, v.nz, 0.f), rot);
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&v.x), pos);
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&v.nx), nor);
	}
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
	D3D12_HEAP_PROPERTIES uploadHeap = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES defaultHeap = { D3D12_HEAP_TYPE_DEFAULT };

	UINT nvbSize = static_cast<UINT>(normalVertices.size() * sizeof(OBJVertex));
	D3D12_RESOURCE_DESC vbRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, nvbSize, 1, 1, 1,
								  DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };

	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &vbRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalVertexBufferUpload));
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &vbRes,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&normalVertexBuffer));

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

	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &ibRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&normalIndexBufferUpload));
	device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &ibRes,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&normalIndexBuffer));

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