#include "stdafx.h"
#include "Level_2_Scene.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Input.h"
#include "framework.h"

void Level_2_Scene::Initialize()
{
	Camera::SetPosition(0.f, 10.f, -20.f);
	Camera::camForward = { 0.f, 0.f, 1.f };
	Camera::camUp = { 0.f, 1.f, 0.f };

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

		// 무작위 X, Z 좌표 추출
		float randX = distX(gen);
		float randZ = distZ(gen);

		// 해당 난수 좌표의 지형 높이를 계산
		float groundY = mTerrain->GetHeightAt(randX, randZ);

		tank->SetPosition(randX, groundY, randZ);
		mTanks.emplace_back(std::move(tank));
	}
}

void Level_2_Scene::Update(float dt)
{
    if (Input::GetKeyDown(eKeyCode::ESC))
        SceneManager::LoadScene(L"MenuScene");

	// 탱크 업데이트 & 인스턴스 행렬 수집
	std::vector<XMFLOAT4X4> bodyMats, lidMats, barrelMats;
	bodyMats.reserve(15); lidMats.reserve(15); barrelMats.reserve(15);

	for (auto& tank : mTanks)
	{
		tank->mLidOffset = mTanks[0]->mLidOffset;
		tank->mLidRotation = mTanks[0]->mLidRotation;
		tank->mBarrelOffset = mTanks[0]->mBarrelOffset;
		tank->mBarrelPivot = mTanks[0]->mBarrelPivot;
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


    ImGui::Begin("Level 2");
    ImGui::TextColored({1,1,0,1}, "Level 2 - Tank Game");
    ImGui::Text("(To be implemented)");
    ImGui::Separator();
    ImGui::Text("ESC: Return to menu");
    ImGui::End();
}

void Level_2_Scene::Release()
{
}
