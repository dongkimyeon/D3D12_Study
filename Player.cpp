#include "Player.h"
#include "Camera.h"
#include "framework.h"

extern bool debugMode;

Player::Player()
{
}

Player::~Player()
{
}

void Player::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);
	LoadFromOBJ("Pacman.obj", device);
	BakeScale(0.0025f, 0.0025f, 0.0025f);
	BakeRotation(0, 180, 0);
	for (auto& v : vertices) {
		v.r = 1.0f; v.g = 1.0f; v.b = 0.0f; v.a = 1.0f;
	}

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
	UpdateVertexBuffer();
}

void Player::Update(float dt)
{
	if (!debugMode) {
		// 마우스 룩: 창 중앙 기준 delta 계산 후 재센터링
		HWND hwnd = Framework::GetHwnd();
		RECT rc;
		GetClientRect(hwnd, &rc);
		POINT center = { rc.right / 2, rc.bottom / 2 };
		ClientToScreen(hwnd, &center);

		POINT cur;
		GetCursorPos(&cur);
		if (!mFirstMouse) {
			mYaw += (cur.x - center.x) * 0.002f;
		}
		mFirstMouse = false;
		SetCursorPos(center.x, center.y);

		// WASD 이동 (XZ 평면)
		float speed = mMoveSpeed;
		if (Input::GetKey(eKeyCode::SHIFT)) speed *= 2.0f;

		XMMATRIX rotM = XMMatrixRotationY(mYaw);
		XMVECTOR fwd  = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotM);
		XMVECTOR rgt  = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotM);
		XMVECTOR pos  = XMLoadFloat3(&position);

		if (Input::GetKey(eKeyCode::W)) pos = pos + fwd * (speed * dt);
		if (Input::GetKey(eKeyCode::S)) pos = pos - fwd * (speed * dt);
		if (Input::GetKey(eKeyCode::D)) pos = pos + rgt * (speed * dt);
		if (Input::GetKey(eKeyCode::A)) pos = pos - rgt * (speed * dt);
		XMStoreFloat3(&position, pos);
		rotation.y = mYaw;

		Camera::SetFollowTarget(position, mYaw);
	} else {
		mFirstMouse = true; // 플레이어 모드 재진입 시 마우스 점프 방지
	}

	GameObject::Update(dt);
}

void Player::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}

DirectX::BoundingBox Player::GetWorldAABB() const
{
	DirectX::BoundingBox worldAABB;
	mLocalAABB.Transform(worldAABB, XMLoadFloat4x4(&worldMatrix));
	return worldAABB;
}
