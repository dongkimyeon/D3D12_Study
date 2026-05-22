#include "LetterObj.h"

LetterObj::LetterObj()
{
}

LetterObj::~LetterObj()
{
}

void LetterObj::Initialize(ComPtr<ID3D12Device> device, const std::string& text)
{
	GameObject::Initialize(device);

	LoadFromOBJ(text, device);
	BakeScale(0.05f, 0.05f, 0.05f); // OBJ 파일이 너무 크므로 스케일 다운
	BakeRotation(0.0f, 90.0f, 0.0f);
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