#include "Helicopter.h"
#include "HeliBody.h"
#include "HeliBlade.h"
#include "HeliTale.h"
#include "Input.h"

Helicopter::Helicopter()
    : mBody(std::make_unique<HeliBody>())
    , mBlade(std::make_unique<HeliBlade>())
    , mTail(std::make_unique<HeliTale>())
{
}

Helicopter::~Helicopter()
{
}

void Helicopter::Initialize(ComPtr<ID3D12Device> device)
{
    mBody->Initialize(device);
	mBody->BakeRotation(-90.f, 0.0f, 0.0f);
	mBody->BakeRotation(0.0f, -90.f, 0.0f);

    mBlade->Initialize(device);
	mBlade->BakeRotation(-90.f, 0.0f, 0.0f);
	mBlade->BakeRotation(0.0f, -90.f, 0.0f);

    mTail->Initialize(device);
	mTail->BakeRotation(-90.f, 0.0f, 0.0f);
	mTail->BakeRotation(0.0f, -90.f, 0.0f);
}

void Helicopter::Update(float dt)
{
	// --- 마우스로 기체 회전 ---
	POINT curMousePos;
	GetCursorPos(&curMousePos);
	if (mFirstMouse) { mPrevMousePos = curMousePos; mFirstMouse = false; }

	float mouseDeltaX = static_cast<float>(curMousePos.x - mPrevMousePos.x);
	mHeading += mouseDeltaX * mMouseSensitivity;
	mPrevMousePos = curMousePos;

	// --- WASD 이동 ---
	const float maxTilt = XMConvertToRadians(18.f);
	const float tiltSpeed = 6.f;

	float targetPitch = 0.f;
	float targetRoll = 0.f;

	XMFLOAT3 bodyPosF = mBody->GetPosition();
	XMVECTOR pos = XMLoadFloat3(&bodyPosF);

	XMVECTOR fwd   = XMVectorSet(sinf(mHeading), 0.f, cosf(mHeading), 0.f);
	XMVECTOR right = XMVectorSet(cosf(mHeading), 0.f, -sinf(mHeading), 0.f);
	XMVECTOR up    = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMVECTOR targetVel = XMVectorZero();
	if (Input::GetKey(eKeyCode::W)) { targetVel += fwd   * mMoveSpeed; targetPitch =  maxTilt; }
	if (Input::GetKey(eKeyCode::S)) { targetVel -= fwd   * mMoveSpeed; targetPitch = -maxTilt; }
	if (Input::GetKey(eKeyCode::A)) { targetVel -= right * mMoveSpeed; targetRoll  =  maxTilt; }
	if (Input::GetKey(eKeyCode::D)) { targetVel += right * mMoveSpeed; targetRoll  = -maxTilt; }
	if (Input::GetKey(eKeyCode::Q)) { targetVel -= up    * mMoveSpeed; }
	if (Input::GetKey(eKeyCode::E)) { targetVel += up    * mMoveSpeed; }

	float accelT = 1.f - expf(-mAcceleration * dt);
	XMVECTOR vel = XMLoadFloat3(&mVelocity);
	vel = XMVectorLerp(vel, targetVel, accelT);
	XMStoreFloat3(&mVelocity, vel);

	pos += vel * dt;

	// 기울기 보간 (지수 감쇠 lerp)
	float t = 1.f - expf(-tiltSpeed * dt);
	mTiltPitch += (targetPitch - mTiltPitch) * t;
	mTiltRoll += (targetRoll - mTiltRoll) * t;

	XMStoreFloat3(&bodyPosF, pos);
	mBody->SetPosition(bodyPosF);
	mBody->SetRotation(XMFLOAT3{ mTiltPitch, mHeading, mTiltRoll });

	// --- 블레이드 / 꼬리 애니메이션 ---
	mBladeAngle += 1000.0f * dt;
	mTailAngle  += 800.0f * dt;

	// 업데이트
	mBody->Update(dt);
	mBlade->Update(dt);
	mTail->Update(dt);

	// --- 로터/꼬리 worldMatrix 직접 계산 (바디 로컬 공간 기준 자전) ---
	// S * R_spin * T_local * R_body * T_body 순서로 합성하면
	// R_spin이 바디 로컬 축 기준으로 동작하면서 pitch/roll이 올바르게 반영됨
	XMFLOAT3 bodyPos = mBody->GetPosition();
	XMMATRIX R_body  = XMMatrixRotationRollPitchYaw(mTiltPitch, mHeading, mTiltRoll);
	XMMATRIX T_body  = XMMatrixTranslation(bodyPos.x, bodyPos.y, bodyPos.z);
	XMMATRIX S       = XMMatrixScaling(0.01f, 0.01f, 0.01f);

	// [메인 로터] 바디 로컬 Y축(상방향) 기준 자전
	{
		XMMATRIX R_spin  = XMMatrixRotationY(XMConvertToRadians(mBladeAngle));
		XMMATRIX T_local = XMMatrixTranslation(0.0f, 0.8f, 2.6f);
		XMFLOAT4X4 mat;
		XMStoreFloat4x4(&mat, S * R_spin * T_local * R_body * T_body);
		mBlade->SetWorldMatrix(mat);
	}

	// [꼬리 로터] 바디 로컬 X축(좌우 방향) 기준 자전
	{
		XMMATRIX R_spin  = XMMatrixRotationX(XMConvertToRadians(mTailAngle));
		XMMATRIX T_local = XMMatrixTranslation(0.3f, 0.7f, -6.4f);
		XMFLOAT4X4 mat;
		XMStoreFloat4x4(&mat, S * R_spin * T_local * R_body * T_body);
		mTail->SetWorldMatrix(mat);
	}
}
void Helicopter::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    mBody->Render(commandList, view, proj);
    mBlade->Render(commandList, view, proj);
    mTail->Render(commandList, view, proj);
}

XMFLOAT3 Helicopter::GetPosition() const
{
    return mBody->GetPosition();
}

XMFLOAT3 Helicopter::GetRotation() const
{
    return mBody->GetRotation();
}

void Helicopter::SetPosition(XMFLOAT3 pos)
{
    mBody->SetPosition(pos);
}

void Helicopter::SetPosition(float x, float y, float z)
{
    mBody->SetPosition(x, y, z);
}

void Helicopter::SetRotation(XMFLOAT3 rot)
{
    mBody->SetRotation(rot);
}

void Helicopter::SetRotation(float pitch, float yaw, float roll)
{
    mBody->SetRotation(pitch, yaw, roll);
}
