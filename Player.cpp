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
		// --- 마우스 룩 ---
		HWND hwnd = Framework::GetHwnd();
		RECT rc;
		GetClientRect(hwnd, &rc);
		POINT center = { rc.right / 2, rc.bottom / 2 };
		ClientToScreen(hwnd, &center);
		POINT cur; GetCursorPos(&cur);
		if (!mFirstMouse) {
			mPitch += (cur.y - center.y) * 0.002f;
			mYaw += (cur.x - center.x) * 0.002f;
		}

		mFirstMouse = false;
		SetCursorPos(center.x, center.y);

		// --- WASD 수평 속도 ---
		float speed = mMoveSpeed;
		if (Input::GetKey(eKeyCode::SHIFT)) speed *= 2.0f;

		XMMATRIX rotM = XMMatrixRotationY(mYaw);
		XMFLOAT3 fwdF, rgtF;
		XMStoreFloat3(&fwdF, XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), rotM));
		XMStoreFloat3(&rgtF, XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), rotM));

		float velX = 0.0f, velZ = 0.0f;
		if (Input::GetKey(eKeyCode::W)) { velX += fwdF.x * speed; velZ += fwdF.z * speed; }
		if (Input::GetKey(eKeyCode::S)) { velX -= fwdF.x * speed; velZ -= fwdF.z * speed; }
		if (Input::GetKey(eKeyCode::D)) { velX += rgtF.x * speed; velZ += rgtF.z * speed; }
		if (Input::GetKey(eKeyCode::A)) { velX -= rgtF.x * speed; velZ -= rgtF.z * speed; }

		// --- 점프 ---
		if (mOnGround && Input::GetKeyDown(eKeyCode::SPACE)) {
			mVelocityY = kJumpSpeed;
			mOnGround  = false;
		}

		// --- 중력 ---
		mVelocityY += kGravity * dt;
		if (mVelocityY < -30.0f) mVelocityY = -30.0f;

		// --- 수평 이동 + 벽 충돌 ---
		position.x += velX * dt;
		position.z += velZ * dt;
		if (mColliders) ResolveHorizontal(*mColliders);

		// --- 수직 이동 + 바닥/천장 충돌 ---
		position.y += mVelocityY * dt;
		mOnGround = false;
		if (mColliders) ResolveVertical(*mColliders);

		rotation.x = mPitch;
		rotation.y = mYaw;
	
		Camera::SetFollowTarget(position, mPitch, mYaw);
	} else {
		mFirstMouse = true;
	}

	GameObject::Update(dt);
}

DirectX::BoundingBox Player::ComputePhysicsAABB() const
{
	// BakeScale 적용 후 scale={1,1,1} → worldMatrix = RotY(mYaw) * Trans(position)
	XMMATRIX world = XMMatrixRotationY(mYaw)
	               * XMMatrixTranslation(position.x, position.y, position.z);
	DirectX::BoundingBox out;
	mLocalAABB.Transform(out, world);
	return out;
}

void Player::ResolveHorizontal(const std::vector<DirectX::BoundingBox>& cubes)
{
	DirectX::BoundingBox p = ComputePhysicsAABB();

	for (const auto& c : cubes) {
		float dx = (p.Extents.x + c.Extents.x) - fabsf(p.Center.x - c.Center.x);
		float dy = (p.Extents.y + c.Extents.y) - fabsf(p.Center.y - c.Center.y);
		float dz = (p.Extents.z + c.Extents.z) - fabsf(p.Center.z - c.Center.z);

		if (dx <= 0.0f || dy <= 0.0f || dz <= 0.0f) continue;
		if (dy <= std::min(dx, dz)) continue; // 수직 접촉 → 수평 패스 스킵

		if (dx < dz) {
			float push = (p.Center.x > c.Center.x) ? dx : -dx;
			position.x   += push;
			p.Center.x   += push;
		} else {
			float push = (p.Center.z > c.Center.z) ? dz : -dz;
			position.z   += push;
			p.Center.z   += push;
		}
	}
}

void Player::ResolveVertical(const std::vector<DirectX::BoundingBox>& cubes)
{
	DirectX::BoundingBox p = ComputePhysicsAABB();

	for (const auto& c : cubes) {
		float dx = (p.Extents.x + c.Extents.x) - fabsf(p.Center.x - c.Center.x);
		float dy = (p.Extents.y + c.Extents.y) - fabsf(p.Center.y - c.Center.y);
		float dz = (p.Extents.z + c.Extents.z) - fabsf(p.Center.z - c.Center.z);

		if (dx <= 0.0f || dy <= 0.0f || dz <= 0.0f) continue;
		if (dy > std::min(dx, dz)) continue; // 수평 접촉 → 수직 패스 스킵

		if (p.Center.y > c.Center.y) {   // 플레이어가 위 → 바닥 착지
			position.y   += dy;
			p.Center.y   += dy;
			if (mVelocityY < 0.0f) { mVelocityY = 0.0f; mOnGround = true; }
		} else {                          // 플레이어가 아래 → 천장 충돌
			position.y   -= dy;
			p.Center.y   -= dy;
			if (mVelocityY > 0.0f) mVelocityY = 0.0f;
		}
	}
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
