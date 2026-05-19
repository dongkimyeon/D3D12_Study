#include "Gun.h"
#include "Player.h"

Gun::Gun()
{
}

Gun::~Gun()
{
}

void Gun::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("AK-74.obj", device);
	BakeScale(0.0015f, 0.0015f, 0.0015f);
	BakeRotation(0.0f, 270.0f, 0.0f);
	for (auto& v : vertices) {
		v.r = 0.0f; v.g = 1.0f; v.b = 0.0f; v.a = 1.0f;
	}
	UpdateVertexBuffer();
}

void Gun::Update(float dt)
{
	if (mOwner) {
		float yaw           = mOwner->GetYaw();
		XMFLOAT3 playerPos  = mOwner->GetPosition();

		XMMATRIX rotY    = XMMatrixRotationY(yaw);
		XMVECTOR right   = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotY);
		XMVECTOR up      = XMVectorSet(0, 1, 0, 0);
		XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotY);

		XMVECTOR gunPos = XMLoadFloat3(&playerPos)
			+ right   * kOffsetRight
			+ up      * kOffsetUp
			+ forward * kOffsetForward;

		XMStoreFloat3(&position, gunPos);
		rotation.y = yaw;
	}

	GameObject::Update(dt);
}

XMFLOAT3 Gun::GetMuzzlePosition() const
{
	if (!mOwner) return position;
	float    yaw     = mOwner->GetYaw();
	XMMATRIX rotY    = XMMatrixRotationY(yaw);
	XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotY);
	XMVECTOR muzzle  = XMLoadFloat3(&position) + forward * 0.5f;
	XMFLOAT3 result;
	XMStoreFloat3(&result, muzzle);
	return result;
}

void Gun::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
