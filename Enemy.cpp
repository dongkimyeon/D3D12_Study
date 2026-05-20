#include "Enemy.h"
#include <algorithm>

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

DirectX::BoundingBox Enemy::ComputePhysicsAABB() const
{
	XMMATRIX world = XMMatrixScaling(scale.x, scale.y, scale.z)
	               * XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::BoundingBox out;
	sLocalAABB.Transform(out, world);
	return out;
}

void Enemy::ResolveWallCollision()
{
	DirectX::BoundingBox p = ComputePhysicsAABB();

	for (const auto& c : *mColliders) {
		float dx = (p.Extents.x + c.Extents.x) - fabsf(p.Center.x - c.Center.x);
		float dy = (p.Extents.y + c.Extents.y) - fabsf(p.Center.y - c.Center.y);
		float dz = (p.Extents.z + c.Extents.z) - fabsf(p.Center.z - c.Center.z);

		if (dx <= 0.0f || dy <= 0.0f || dz <= 0.0f) continue;

		if (dx <= dy && dx <= dz) {
			float push = (p.Center.x > c.Center.x) ? dx : -dx;
			position.x += push;
			p.Center.x += push;
		} else if (dy <= dx && dy <= dz) {
			float push = (p.Center.y > c.Center.y) ? dy : -dy;
			position.y += push;
			p.Center.y += push;
		} else {
			float push = (p.Center.z > c.Center.z) ? dz : -dz;
			position.z += push;
			p.Center.z += push;
		}
	}
}

void Enemy::SetFlowField(const int* field, int gridSize, float spacing, float offset)
{
	mFlowField = field;
	mGridSize  = gridSize;
	mSpacing   = spacing;
	mOffset    = offset;
}

// flow field에서 현재 셀 이웃 중 플레이어에 가장 가까운 셀의 월드 중심을 반환
XMFLOAT3 Enemy::GetNextWaypoint() const
{
	int eRow = std::clamp((int)roundf((position.z + mOffset) / mSpacing), 0, mGridSize - 1);
	int eCol = std::clamp((int)roundf((position.x + mOffset) / mSpacing), 0, mGridSize - 1);

	int myDist = mFlowField[eRow * mGridSize + eCol];
	if (myDist <= 0) {
		// 플레이어 셀에 도착했거나 flow field 미도달, 직접 타겟으로
		return *mTarget;
	}

	static const int DR[] = { 0, 0, -1, 1 };
	static const int DC[] = { -1, 1,  0, 0 };

	int bestRow = eRow, bestCol = eCol, bestDist = myDist;
	for (int d = 0; d < 4; d++) {
		int nr = eRow + DR[d], nc = eCol + DC[d];
		if (nr < 0 || nr >= mGridSize || nc < 0 || nc >= mGridSize) continue;
		int dist = mFlowField[nr * mGridSize + nc];
		if (dist >= 0 && dist < bestDist) {
			bestDist = dist;
			bestRow  = nr;
			bestCol  = nc;
		}
	}

	return { bestCol * mSpacing - mOffset, position.y, bestRow * mSpacing - mOffset };
}

void Enemy::SeparateFromEnemies()
{
	static constexpr float kSepRadius = 0.8f;
	XMVECTOR myPos = XMLoadFloat3(&position);

	for (auto* other : *mEnemies) {
		if (other == this || !other->IsAlive()) continue;
		XMFLOAT3 otherPosF = other->GetPosition();
		XMVECTOR otherPos  = XMLoadFloat3(&otherPosF);
		XMVECTOR diff = myPos - otherPos;
		float dist = XMVectorGetX(XMVector3LengthEst(diff));
		if (dist < kSepRadius && dist > 0.001f) {
			myPos += XMVector3Normalize(diff) * ((kSepRadius - dist) * 0.5f);
		}
	}
	XMStoreFloat3(&position, myPos);
}

void Enemy::Update(float dt)
{
	if (!mAlive) return;
	if (mTarget && mFlowField) {
		// 아직 추적 중이 아니면 감지 범위 체크
		if (!mIsChasing) {
			XMVECTOR ep = XMLoadFloat3(&position);
			XMVECTOR tp = XMLoadFloat3(mTarget);
			float dist = XMVectorGetX(XMVector3LengthEst(tp - ep));
			if (dist <= mDetectRange)
				mIsChasing = true;
		}

		if (!mIsChasing) {
			GameObject::Update(dt);
			return;
		}

		XMFLOAT3 waypoint = GetNextWaypoint();

		XMVECTOR pos  = XMLoadFloat3(&position);
		XMVECTOR wp   = XMLoadFloat3(&waypoint);
		XMVECTOR toWP = XMVectorSetY(wp - pos, 0.0f); // 수평 이동만 (공중 부유)
		float dist = XMVectorGetX(XMVector3LengthEst(toWP));

		if (dist > 0.05f) {
			pos += XMVector3Normalize(toWP) * mMoveSpeed * dt;
			XMStoreFloat3(&position, pos);

			XMFLOAT3 d; XMStoreFloat3(&d, toWP);
			rotation.y = atan2f(d.x, d.z);
		}

		if (mColliders) ResolveWallCollision();
		if (mEnemies)   SeparateFromEnemies();
	}

	GameObject::Update(dt);
}

void Enemy::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (!mAlive) return;
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
