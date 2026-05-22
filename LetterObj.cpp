#include "LetterObj.h"

LetterObj::LetterObj()
{
}

LetterObj::~LetterObj()
{
}

void LetterObj::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("START.obj", device);

	// 버텍스로부터 로컬 공간 AABB 계산
	XMFLOAT3 vmin = { FLT_MAX,  FLT_MAX,  FLT_MAX };
	XMFLOAT3 vmax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
	for (const auto& v : vertices)
	{
		vmin.x = std::min(vmin.x, v.x);  vmax.x = std::max(vmax.x, v.x);
		vmin.y = std::min(vmin.y, v.y);  vmax.y = std::max(vmax.y, v.y);
		vmin.z = std::min(vmin.z, v.z);  vmax.z = std::max(vmax.z, v.z);
	}
	XMFLOAT3 center = { (vmin.x + vmax.x) * 0.5f, (vmin.y + vmax.y) * 0.5f, (vmin.z + vmax.z) * 0.5f };
	XMFLOAT3 extents = { (vmax.x - vmin.x) * 0.5f, (vmax.y - vmin.y) * 0.5f, (vmax.z - vmin.z) * 0.5f };
	mLocalAABB = DirectX::BoundingBox(center, extents);
}

DirectX::BoundingBox LetterObj::GetWorldAABB() const
{
	DirectX::BoundingBox worldAABB;
	mLocalAABB.Transform(worldAABB, XMLoadFloat4x4(&worldMatrix));
	return worldAABB;
}

void LetterObj::Update(float dt)
{
	GameObject::Update(dt);
}

void LetterObj::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}