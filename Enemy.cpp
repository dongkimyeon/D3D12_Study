#include "Enemy.h"

ComPtr<ID3D12Resource>   Enemy::sVB;
ComPtr<ID3D12Resource>   Enemy::sIB;
ComPtr<ID3D12Resource>   Enemy::sVBUpload;
ComPtr<ID3D12Resource>   Enemy::sIBUpload;
D3D12_VERTEX_BUFFER_VIEW Enemy::sVbView  = {};
D3D12_INDEX_BUFFER_VIEW  Enemy::sIbView  = {};
UINT                     Enemy::sIndexCount = 0;
D3D12_RESOURCE_STATES    Enemy::sVBState = D3D12_RESOURCE_STATE_COPY_DEST;
D3D12_RESOURCE_STATES    Enemy::sIBState = D3D12_RESOURCE_STATE_COPY_DEST;
bool                     Enemy::sVBDirty = false;
bool                     Enemy::sIBDirty = false;
DirectX::BoundingBox     Enemy::sLocalAABB;

void Enemy::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
	std::vector<OBJVertex> verts;
	std::vector<uint16_t>  inds;
	OBJLoader::Load("Enemy.obj", verts, inds);


	// 회전 베이킹 
	XMMATRIX rot = XMMatrixRotationX(XMConvertToRadians(-90.0f))
	* XMMatrixRotationY(XMConvertToRadians(90.0f));  // 원하는 각도로

	for (auto& v : verts) {
		XMVECTOR p = XMVector3TransformCoord(XMVectorSet(v.x, v.y, v.z, 1), rot);
		XMVECTOR n = XMVector3TransformNormal(XMVectorSet(v.nx, v.ny, v.nz, 0), rot);
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&v.x), p);
		XMStoreFloat3(reinterpret_cast<XMFLOAT3*>(&v.nx), n);
	}

	for (auto& v : verts) { v.r = 1.f; v.g = 0.f; v.b = 0.f; v.a = 1.f; }

	// AABB 한 번만 계산
	XMFLOAT3 vmin = {  FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& v : verts) {
		vmin.x = std::min(vmin.x, v.x); vmax.x = std::max(vmax.x, v.x);
		vmin.y = std::min(vmin.y, v.y); vmax.y = std::max(vmax.y, v.y);
		vmin.z = std::min(vmin.z, v.z); vmax.z = std::max(vmax.z, v.z);
	}
	sLocalAABB = DirectX::BoundingBox(
		{ (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f },
		{ (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f });

	D3D12_HEAP_PROPERTIES upload = { D3D12_HEAP_TYPE_UPLOAD };
	D3D12_HEAP_PROPERTIES def    = { D3D12_HEAP_TYPE_DEFAULT };

	UINT vbSize = (UINT)(verts.size() * sizeof(OBJVertex));
	D3D12_RESOURCE_DESC vRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, vbSize,
		1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &vRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sVBUpload));
	device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &vRes,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sVB));
	void* mapped;
	sVBUpload->Map(0, nullptr, &mapped);
	memcpy(mapped, verts.data(), vbSize);
	sVBUpload->Unmap(0, nullptr);
	sVbView.BufferLocation = sVB->GetGPUVirtualAddress();
	sVbView.StrideInBytes  = sizeof(OBJVertex);
	sVbView.SizeInBytes    = vbSize;
	sVBState = D3D12_RESOURCE_STATE_COPY_DEST;
	sVBDirty = true;

	UINT ibSize = (UINT)(inds.size() * sizeof(uint16_t));
	D3D12_RESOURCE_DESC iRes = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, ibSize,
		1, 1, 1, DXGI_FORMAT_UNKNOWN, {1, 0}, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	device->CreateCommittedResource(&upload, D3D12_HEAP_FLAG_NONE, &iRes,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&sIBUpload));
	device->CreateCommittedResource(&def, D3D12_HEAP_FLAG_NONE, &iRes,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&sIB));
	sIBUpload->Map(0, nullptr, &mapped);
	memcpy(mapped, inds.data(), ibSize);
	sIBUpload->Unmap(0, nullptr);
	sIbView.BufferLocation = sIB->GetGPUVirtualAddress();
	sIbView.Format         = DXGI_FORMAT_R16_UINT;
	sIbView.SizeInBytes    = ibSize;
	sIBState = D3D12_RESOURCE_STATE_COPY_DEST;
	sIBDirty = true;

	sIndexCount = (UINT)inds.size();
}

void Enemy::UnloadSharedMesh()
{
	sVB.Reset();  sVBUpload.Reset();
	sIB.Reset();  sIBUpload.Reset();
	sIndexCount = 0;
	sVBDirty = sIBDirty = false;
}

Enemy::Enemy()
{
}

Enemy::~Enemy()
{
}

void Enemy::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	
	mLocalAABB = sLocalAABB;
}

void Enemy::Update(float dt)
{
	GameObject::Update(dt);
}

void Enemy::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (sIndexCount == 0) return;

	auto flush = [&](ComPtr<ID3D12Resource>& gpu, ComPtr<ID3D12Resource>& staging,
		D3D12_RESOURCE_STATES& state, D3D12_RESOURCE_STATES target,
		UINT64 size, bool& dirty)
	{
		if (!dirty) return;
		if (state != D3D12_RESOURCE_STATE_COPY_DEST) {
			D3D12_RESOURCE_BARRIER b = {};
			b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			b.Transition.pResource   = gpu.Get();
			b.Transition.StateBefore = state;
			b.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
			b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			commandList->ResourceBarrier(1, &b);
		}
		commandList->CopyBufferRegion(gpu.Get(), 0, staging.Get(), 0, size);
		D3D12_RESOURCE_BARRIER b = {};
		b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		b.Transition.pResource   = gpu.Get();
		b.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		b.Transition.StateAfter  = target;
		b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &b);
		state = target;
		dirty = false;
	};

	flush(sVB, sVBUpload, sVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, sVbView.SizeInBytes, sVBDirty);
	flush(sIB, sIBUpload, sIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,               sIbView.SizeInBytes, sIBDirty);

	XMMATRIX mvp = XMLoadFloat4x4(&worldMatrix) * view * proj;
	XMFLOAT4X4 mvpT;
	XMStoreFloat4x4(&mvpT, XMMatrixTranspose(mvp));

	static const float white[4] = { 1.f, 1.f, 1.f, 1.f };
	commandList->SetGraphicsRoot32BitConstants(0, 4,  white,             0);
	commandList->SetGraphicsRoot32BitConstants(0, 16, &mvpT.m[0][0],    4);
	commandList->IASetVertexBuffers(0, 1, &sVbView);
	commandList->IASetIndexBuffer(&sIbView);
	commandList->DrawIndexedInstanced(sIndexCount, 1, 0, 0, 0);

	if (sShowAABB)
		RenderAABB(commandList, view, proj);
}

DirectX::BoundingBox Enemy::GetWorldAABB() const
{
	DirectX::BoundingBox worldAABB;
	mLocalAABB.Transform(worldAABB, XMLoadFloat4x4(&worldMatrix));
	return worldAABB;
}
