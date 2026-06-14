#include "stdafx.h"
#include "Level_2_Scene.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Input.h"
#include "framework.h"

extern bool debugMode;

void Level_2_Scene::Initialize()
{
	debugMode = false;

	auto device = Framework::GetDevice();

	mTerrain = std::make_unique<Terrain>();
	mTerrain->Initialize(device);

	size_t numTanks = 15;

	mTankBodyRenderer = std::make_unique<TankBody>();
	mTankLidRenderer = std::make_unique<TankLid>();
	mTankBarrelRenderer = std::make_unique<TankBarrel>();

	mTankBodyRenderer->Initialize(device, numTanks);
	mTankLidRenderer->Initialize(device, numTanks);
	mTankBarrelRenderer->Initialize(device, numTanks);

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<float> distX(-100.f, 100.f);
	std::uniform_real_distribution<float> distZ(40.f, 250.f);

	mTanks.reserve(numTanks);

	for (size_t i = 0; i < numTanks; ++i)
	{
		auto tank = std::make_unique<Tank>();
		float randX = distX(gen);
		float randZ = distZ(gen);
		float groundY = mTerrain->GetHeightAt(randX, randZ);
		tank->SetPosition(randX, groundY, randZ);
		mTanks.emplace_back(std::move(tank));
	}

	mPlayerBodyRenderer   = std::make_unique<TankBody>();
	mPlayerLidRenderer    = std::make_unique<TankLid>();
	mPlayerBarrelRenderer = std::make_unique<TankBarrel>();
	mPlayerBodyRenderer->Initialize(device, 1);
	mPlayerLidRenderer->Initialize(device, 1);
	mPlayerBarrelRenderer->Initialize(device, 1);

	mPlayerBodyRenderer->SetColor(0.53f, 0.81f, 0.98f);
	mPlayerLidRenderer->SetColor(0.53f, 0.81f, 0.98f);
	mPlayerBarrelRenderer->SetColor(0.45f, 0.72f, 0.90f);

	mPlayerTank = std::make_unique<Tank>();
	float spawnY = mTerrain->GetHeightAt(0.f, 0.f);
	mPlayerTank->SetPosition(0.f, spawnY, 0.f);
}

void Level_2_Scene::Update(float dt)
{
	if (Input::GetKeyDown(eKeyCode::ESC))
		SceneManager::LoadScene(L"MenuScene");

	constexpr float kMoveSpeed = 15.0f;
	constexpr float kTurnSpeed = 1.5f;

	XMFLOAT3 pos = mPlayerTank->GetPosition();
	float sinH = sinf(mPlayerTank->mHeading);
	float cosH = cosf(mPlayerTank->mHeading);

	if (Input::GetKey(eKeyCode::W)) { pos.x -= sinH * kMoveSpeed * dt; pos.z -= cosH * kMoveSpeed * dt; }
	if (Input::GetKey(eKeyCode::S)) { pos.x += sinH * kMoveSpeed * dt; pos.z += cosH * kMoveSpeed * dt; }

	if (Input::GetKey(eKeyCode::A)) mPlayerTank->mHeading += kTurnSpeed * dt;
	if (Input::GetKey(eKeyCode::D)) mPlayerTank->mHeading -= kTurnSpeed * dt;

	pos.y = mTerrain->GetHeightAt(pos.x, pos.z);
	mPlayerTank->SetPosition(pos);

	if (Input::GetKey(eKeyCode::LButton))
	{
		POINT curr;
		GetCursorPos(&curr);
		if (!mMouseRotating) { mPrevMousePos = curr; mMouseRotating = true; }

		constexpr float kSensH = 0.3f;
		constexpr float kSensV = 0.3f;
		float dx = static_cast<float>(curr.x - mPrevMousePos.x);
		float dy = static_cast<float>(curr.y - mPrevMousePos.y);

		mPlayerTank->mLidRotation.y -= dx * kSensH;

		mPlayerTank->mBarrelRotation.x -= dy * kSensV;
		mPlayerTank->mBarrelRotation.x  = std::clamp(mPlayerTank->mBarrelRotation.x, -10.0f, 20.0f);

		mPrevMousePos = curr;
	}
	else { mMouseRotating = false; }

	{
		constexpr float kStep   = 1.0f;
		constexpr float kSmooth = 8.0f;

		float sh = sinf(mPlayerTank->mHeading);
		float ch = cosf(mPlayerTank->mHeading);

		float sFwd = (mTerrain->GetHeightAt(pos.x - sh * kStep, pos.z - ch * kStep)
		            - mTerrain->GetHeightAt(pos.x + sh * kStep, pos.z + ch * kStep))
		           / (2.0f * kStep);

		float sLat = (mTerrain->GetHeightAt(pos.x - ch * kStep, pos.z + sh * kStep)
		            - mTerrain->GetHeightAt(pos.x + ch * kStep, pos.z - sh * kStep))
		           / (2.0f * kStep);

		float targetPitch = atan2f(sFwd, 1.0f);
		float targetRoll  = atan2f(sLat, 1.0f);

		mPlayerTank->mPitch += (targetPitch - mPlayerTank->mPitch) * kSmooth * dt;
		mPlayerTank->mRoll  += (targetRoll  - mPlayerTank->mRoll)  * kSmooth * dt;
	}

	mPlayerTank->Update(dt);
	mPlayerBodyRenderer->SetWorldMatrix(mPlayerTank->mBodyMatrix);
	mPlayerLidRenderer->SetWorldMatrix(mPlayerTank->mLidMatrix);
	mPlayerBarrelRenderer->SetWorldMatrix(mPlayerTank->mBarrelMatrix);

	constexpr float kCamDist   = 12.0f;
	constexpr float kCamHeight = 5.0f;

	float turretWorldYaw = mPlayerTank->mHeading + XMConvertToRadians(mPlayerTank->mLidRotation.y);
	float sinTY = sinf(turretWorldYaw);
	float cosTY = cosf(turretWorldYaw);

	XMVECTOR tankPosV = XMLoadFloat3(&pos);
	XMVECTOR worldUp  = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	XMVECTOR camPosV  = tankPosV
		+ XMVectorSet(sinTY, 0.f, cosTY, 0.f) * kCamDist
		+ worldUp * kCamHeight;
	XMStoreFloat3(&Camera::camPos, camPosV);

	XMVECTOR lookTarget = tankPosV + worldUp * 1.0f;
	XMVECTOR fwdV   = XMVector3Normalize(lookTarget - camPosV);
	XMVECTOR rightV = XMVector3Normalize(XMVector3Cross(worldUp, fwdV));
	XMVECTOR upV    = XMVector3Cross(fwdV, rightV);
	XMStoreFloat3(&Camera::camForward, fwdV);
	XMStoreFloat3(&Camera::camRight, rightV);
	XMStoreFloat3(&Camera::camUp, upV);

	std::vector<XMFLOAT4X4> bodyMats, lidMats, barrelMats;
	bodyMats.reserve(15); lidMats.reserve(15); barrelMats.reserve(15);

	for (auto& tank : mTanks)
	{
		tank->mLidOffset      = mTanks[0]->mLidOffset;
		tank->mLidRotation    = mTanks[0]->mLidRotation;
		tank->mBarrelOffset   = mTanks[0]->mBarrelOffset;
		tank->mBarrelPivot    = mTanks[0]->mBarrelPivot;
		tank->mBarrelRotation = mTanks[0]->mBarrelRotation;
		tank->Update(dt);

		if (tank->IsAlive())
		{
			bodyMats.push_back(tank->mBodyMatrix);
			lidMats.push_back(tank->mLidMatrix);
			barrelMats.push_back(tank->mBarrelMatrix);
		}
	}

	mTankBodyRenderer->UpdateInstances(bodyMats);
	mTankLidRenderer->UpdateInstances(lidMats);
	mTankBarrelRenderer->UpdateInstances(barrelMats);
}

void Level_2_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&Camera::camPos),
		XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
		XMVectorSet(Camera::camUp.x, Camera::camUp.y, Camera::camUp.z, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		XM_PIDIV4, (float)Framework::GetWidth() / (float)Framework::GetHeight(),
		0.1f, 1000.0f);

	for (const auto& obj : mGameObjects)
		obj->Render(commandList, view, proj);

	mTerrain->Render(commandList, view, proj);

	mTankBodyRenderer->Render(commandList, view, proj);
	mTankLidRenderer->Render(commandList, view, proj);
	mTankBarrelRenderer->Render(commandList, view, proj);
	mTankBodyRenderer->RenderOBBs(commandList, view, proj);

	mPlayerBodyRenderer->Render(commandList, view, proj);
	mPlayerLidRenderer->Render(commandList, view, proj);
	mPlayerBarrelRenderer->Render(commandList, view, proj);

}

void Level_2_Scene::Release()
{
}
