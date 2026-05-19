#include "Bullet.h"

Bullet::Bullet()
{
}

Bullet::~Bullet()
{
}

void Bullet::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("sphere.obj", device);
	BakeScale(0.2f, 0.2f, 0.2f);
	for (auto& v : vertices) {
		v.r = 1.0f; v.g = 1.0f; v.b = 0.0f; v.a = 1.0f;
	}

	XMFLOAT3 vmin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& v : vertices)
	{
		vmin.x = std::min(vmin.x, v.x);  vmax.x = std::max(vmax.x, v.x);
		vmin.y = std::min(vmin.y, v.y);  vmax.y = std::max(vmax.y, v.y);
		vmin.z = std::min(vmin.z, v.z);  vmax.z = std::max(vmax.z, v.z);
	}
	XMFLOAT3 center  = { (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f };
	XMFLOAT3 extents = { (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f };
	mLocalAABB = DirectX::BoundingBox(center, extents);
	UpdateVertexBuffer();
}

void Bullet::Fire(XMFLOAT3 spawnPos, XMFLOAT3 dir, float speed)
{
	position = spawnPos;
	XMVECTOR d = XMVector3Normalize(XMLoadFloat3(&dir));
	XMStoreFloat3(&mDir, d);
	mSpeed  = speed;
	mLife   = 3.0f;
	mActive = true;
}

void Bullet::Update(float dt)
{
	if (!mActive) return;

	mLife -= dt;
	if (mLife <= 0.0f) { mActive = false; return; }

	position.x += mDir.x * mSpeed * dt;
	position.y += mDir.y * mSpeed * dt;
	position.z += mDir.z * mSpeed * dt;

	GameObject::Update(dt);

	if (mColliders) {
		DirectX::BoundingBox worldAABB = GetWorldAABB();
		for (const auto& c : *mColliders) {
			if (worldAABB.Intersects(c)) {
				mActive = false;
				break;
			}
		}
	}
}

void Bullet::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	if (!mActive) return;
	GameObject::Render(commandList, view, proj);
}

DirectX::BoundingBox Bullet::GetWorldAABB() const
{
	DirectX::BoundingBox worldAABB;
	mLocalAABB.Transform(worldAABB, XMLoadFloat4x4(&worldMatrix));
	return worldAABB;
}
