#include "stdafx.h"
#include "GameObject.h"
#include "framework.h"

bool                     GameObject::sShowAABB = false;
ComPtr<ID3D12Resource>   GameObject::sAABBVB;
ComPtr<ID3D12Resource>   GameObject::sAABBIB;
D3D12_VERTEX_BUFFER_VIEW GameObject::sAABBVbView = {};
D3D12_INDEX_BUFFER_VIEW  GameObject::sAABBIbView = {};

GameObject::GameObject()
{
	position = { 0, 0, 0 };
	rotation = { 0, 0, 0 };
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

	XMMATRIX mScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	XMMATRIX mRot = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
	XMMATRIX mTrans = XMMatrixTranslation(position.x, position.y, position.z);

	XMMATRIX world = mScale * mRot * mTrans;

	XMStoreFloat4x4(&worldMatrix, world);
}

void GameObject::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (indices.empty()) return;

	UploadBufferIfDirty(commandList, vertexBuffer, vertexBufferUpload,
		mVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		vertices.size() * sizeof(OBJVertex), mVBDirty);
	UploadBufferIfDirty(commandList, indexBuffer, indexBufferUpload,
		mIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
		indices.size() * sizeof(uint32_t), mIBDirty);

	if (mInstanceCount == 1 && mInstanceBufferUpload)
	{
		void* ptr;
		mInstanceBufferUpload->Map(0, nullptr, &ptr);
		memcpy(ptr, &worldMatrix, sizeof(XMFLOAT4X4));
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

if (sShowAABB)
		RenderAABB(commandList, view, proj);
}

void GameObject::LoadFromOBJ(const std::string& filename, ComPtr<ID3D12Device> device)
{
	OBJLoader::Load(filename, vertices, indices);

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

	if (!vertices.empty()) {
		XMFLOAT3 vmin = { FLT_MAX, FLT_MAX, FLT_MAX }, vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const auto& v : vertices) {
			vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
			vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
			vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
		}
		mLocalAABB = DirectX::BoundingBox(
			{ (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
			{ (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });
	}
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

	if (currentState != D3D12_RESOURCE_STATE_COPY_DEST)
	{
		barrier.Transition.StateBefore = currentState;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
		cmdList->ResourceBarrier(1, &barrier);
	}

	cmdList->CopyBufferRegion(gpuBuf.Get(), 0, uploadBuf.Get(), 0, byteSize);

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
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
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

}

DirectX::BoundingBox GameObject::GetWorldAABB() const
{
	DirectX::BoundingBox world;
	mLocalAABB.Transform(world, XMLoadFloat4x4(&worldMatrix));
	return world;
}

DirectX::BoundingOrientedBox GameObject::GetWorldOBB() const
{
	DirectX::BoundingOrientedBox localOBB;
	DirectX::BoundingOrientedBox::CreateFromBoundingBox(localOBB, mLocalAABB);
	DirectX::BoundingOrientedBox worldOBB;
	localOBB.Transform(worldOBB, XMLoadFloat4x4(&worldMatrix));
	return worldOBB;
}

void GameObject::RenderAABB(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	EnsureAABBMesh();

	XMMATRIX world;
	if (UseOBB())
	{
		DirectX::BoundingOrientedBox obb = GetWorldOBB();
		if (obb.Extents.x == 0 && obb.Extents.y == 0 && obb.Extents.z == 0) return;
		world = XMMatrixScaling(obb.Extents.x * 2.f, obb.Extents.y * 2.f, obb.Extents.z * 2.f)
			* XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation))
			* XMMatrixTranslation(obb.Center.x, obb.Center.y, obb.Center.z);
	}
	else
	{
		DirectX::BoundingBox aabb = GetWorldAABB();
		if (aabb.Extents.x == 0 && aabb.Extents.y == 0 && aabb.Extents.z == 0) return;
		world = XMMatrixScaling(aabb.Extents.x * 2.f, aabb.Extents.y * 2.f, aabb.Extents.z * 2.f)
			* XMMatrixTranslation(aabb.Center.x, aabb.Center.y, aabb.Center.z);
	}

	XMFLOAT4X4 worldData, vpFloat;
	XMStoreFloat4x4(&worldData, world);
	XMStoreFloat4x4(&vpFloat, XMMatrixTranspose(view * proj));

	if (!mAABBInstBuf)
	{
		D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
		UINT instSize = sizeof(XMFLOAT4X4);
		D3D12_RESOURCE_DESC instRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, instSize,
			1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
		Framework::GetDevice()->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &instRes,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mAABBInstBuf));
		mAABBInstView.BufferLocation = mAABBInstBuf->GetGPUVirtualAddress();
		mAABBInstView.StrideInBytes  = sizeof(XMFLOAT4X4);
		mAABBInstView.SizeInBytes    = instSize;
	}

	void* ptr;
	mAABBInstBuf->Map(0, nullptr, &ptr);
	memcpy(ptr, &worldData, sizeof(XMFLOAT4X4));
	mAABBInstBuf->Unmap(0, nullptr);

	commandList->SetGraphicsRoot32BitConstants(0, 16, &vpFloat.m[0][0], 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	commandList->IASetVertexBuffers(0, 1, &sAABBVbView);
	commandList->IASetVertexBuffers(1, 1, &mAABBInstView);
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

void GameObject::SetColor(float r, float g, float b)
{
	for (auto& v : vertices) {
		v.r = r; v.g = g; v.b = b;
	}
	UpdateVertexBuffer();
}

void GameObject::RecomputeLocalAABB()
{
	if (vertices.empty()) return;
	XMFLOAT3 vmin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& v : vertices) {
		vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
		vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
		vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
	}
	mLocalAABB = DirectX::BoundingBox(
		{ (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
		{ (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });
}

void GameObject::BakeScale(float sx, float sy, float sz)
{
	for (auto& v : vertices) {
		v.x *= sx; v.y *= sy; v.z *= sz;
	}
	UpdateVertexBuffer();
	RecomputeLocalAABB();
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
	RecomputeLocalAABB();
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
	RecomputeLocalAABB();
}

