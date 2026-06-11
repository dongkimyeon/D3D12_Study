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
    Camera::camUp      = { 0.f, 1.f, 0.f };

	mTerrain = std::make_unique<Terrain>();
	mTerrain->Initialize(Framework::GetDevice());
	
	// 탱크 렌더러: 메쉬 1회 로드, 인스턴스 버퍼 15개 분 할당
	mTankBodyRenderer = std::make_unique<TankBody>();
	mTankLidRenderer = std::make_unique<TankLid>();
	mTankBarrelRenderer = std::make_unique<TankBarrel>();
	mTankBodyRenderer->Initialize(Framework::GetDevice(), 15);
	mTankLidRenderer->Initialize(Framework::GetDevice(), 15);
	mTankBarrelRenderer->Initialize(Framework::GetDevice(), 15);

	static const float kTankXZ[15][2] = {
	   {   0.f,  40.f }, {  30.f,  40.f }, { -30.f,  40.f },
	   {  60.f,  80.f }, { -60.f,  80.f }, {   0.f,  80.f },
	   {  90.f, 120.f }, { -90.f, 120.f }, {  30.f, 120.f },
	   { -30.f, 120.f }, {  60.f, 160.f }, { -60.f, 160.f },
	   {   0.f, 160.f }, {  90.f, 200.f }, { -90.f, 200.f },
	};

	mTanks.resize(15);
	for (int i = 0; i < 15; i++)
	{
		mTanks[i] = std::make_unique<Tank>();
		float groundY = mTerrain->GetHeightAt(kTankXZ[i][0], kTankXZ[i][1]);
		mTanks[i]->SetPosition(kTankXZ[i][0], groundY, kTankXZ[i][1]);
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
