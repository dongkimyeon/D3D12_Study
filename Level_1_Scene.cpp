#include "stdafx.h"
#include "framework.h"
#include "Level_1_Scene.h"
#include "Camera.h"
#include "SceneManager.h"

extern bool debugMode;

Level_1_Scene::Level_1_Scene()
{
}

Level_1_Scene::~Level_1_Scene()
{
}

void Level_1_Scene::Initialize()
{
    debugMode = false;

    Input::LockCursor(Framework::GetHwnd());

    mHelicopter = std::make_unique<Helicopter>();
    mHelicopter->Initialize(Framework::GetDevice());
    mHelicopter->SetPosition(0.f, 5.f, 0.f);

    Camera::SetPosition(0.f, 10.f, -15.f);

	mTerrain = std::make_unique<Terrain>();
	mTerrain->Initialize(Framework::GetDevice());
	mTerrain->SetScale(3.0f, 3.0f, 3.0f);


    static const float kTankXZ[15][2] = {
        {   0.f,  40.f }, {  30.f,  40.f }, { -30.f,  40.f },
        {  60.f,  80.f }, { -60.f,  80.f }, {   0.f,  80.f },
        {  90.f, 120.f }, { -90.f, 120.f }, {  30.f, 120.f },
        { -30.f, 120.f }, {  60.f, 160.f }, { -60.f, 160.f },
        {   0.f, 160.f }, {  90.f, 200.f }, { -90.f, 200.f },
    };

 

    mRing = new Ring();
    mRing->Initialize(Framework::GetDevice());
    mRing->SetPosition(0.f, 55.f, 250.f);
    mGameObjects.push_back(mRing);

    for (int i = 0; i < kMissilePoolSize; i++)
    {
        mMissilePool[i] = new Missile();
        mMissilePool[i]->Initialize(Framework::GetDevice());
    }
}

void Level_1_Scene::Update(float dt)
{
    if (Input::GetKeyDown(eKeyCode::ESC))
        SceneManager::LoadScene(L"MenuScene");

    if (Input::GetKeyDown(eKeyCode::N))
        SceneManager::LoadScene(L"Level_2");

    if (Input::GetKeyDown(eKeyCode::V))
        mFirstPerson = !mFirstPerson;

    if (Input::GetKeyDown(eKeyCode::F5))
    {
        if (Input::IsCursorLocked())
            Input::UnlockCursor();
        else
            Input::LockCursor(Framework::GetHwnd());
    }

    mHelicopter->Update(dt);
    mTerrain->Update(dt);

    XMFLOAT3 heliPos = mHelicopter->GetPosition();
    float groundY    = mTerrain->GetHeightAt(heliPos.x, heliPos.z);
    if (heliPos.y < groundY + 2.0f)
    {
        mHelicopter->SetPosition(heliPos.x, groundY + 2.0f, heliPos.z);
        heliPos.y = groundY + 2.0f;
    }
    float heading = mHelicopter->GetHeading();

    if (mFirstPerson)
    {
        XMMATRIX R = XMMatrixRotationRollPitchYaw(
            mHelicopter->GetTiltPitch(), heading, mHelicopter->GetTiltRoll());
        XMVECTOR worldOffset = XMVector3TransformNormal(XMLoadFloat3(&mFpvOffset), R);
        XMStoreFloat3(&Camera::camPos, XMLoadFloat3(&heliPos) + worldOffset);

        XMVECTOR fwd = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.f, 0.f, 1.f, 0.f), R));
        XMVECTOR up  = XMVector3Normalize(XMVector3TransformNormal(XMVectorSet(0.f, 1.f, 0.f, 0.f), R));
        XMStoreFloat3(&Camera::camForward, fwd);
        XMStoreFloat3(&Camera::camUp, up);
    }
    else
    {
        const float camDist   = 15.f;
        const float camHeight =  5.f;
        XMFLOAT3 targetCamPos = {
            heliPos.x - sinf(heading) * camDist,
            heliPos.y + camHeight,
            heliPos.z - cosf(heading) * camDist
        };

        const float camSpeed = 8.f;
        float t = 1.f - expf(-camSpeed * dt);
        XMVECTOR camPos    = XMLoadFloat3(&Camera::camPos);
        XMVECTOR targetPos = XMLoadFloat3(&targetCamPos);
        XMStoreFloat3(&Camera::camPos, XMVectorLerp(camPos, targetPos, t));

        XMVECTOR lookDir = XMVector3Normalize(
            XMLoadFloat3(&heliPos) - XMLoadFloat3(&Camera::camPos));
        XMStoreFloat3(&Camera::camForward, lookDir);
        Camera::camUp = { 0.f, 1.f, 0.f };
    }

 


    for (const auto& obj : mGameObjects)
        obj->Update(dt);

    // 미사일 발사 (마우스 왼쪽 클릭)
    if (Input::GetKeyDown(eKeyCode::LButton))
    {
        XMFLOAT3 pos1, pos2, dir;
        mHelicopter->GetFireData(pos1, pos2, dir);
        XMFLOAT3 spawnPos = mFireFromLeft ? pos1 : pos2;
        mFireFromLeft = !mFireFromLeft;

        for (int i = 0; i < kMissilePoolSize; i++)
        {
            if (mMissilePool[i]->IsDead())
            {
                mMissilePool[i]->Spawn(spawnPos, dir);
                break;
            }
        }
    }

    for (int i = 0; i < kMissilePoolSize; i++)
        mMissilePool[i]->Update(dt);

    // Ring 충돌 -> Level 2 전환
    if (mRing && mRing->CheckCollision(heliPos))
        SceneManager::LoadScene(L"Level_2");
}

void Level_1_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
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

    mHelicopter->Render(commandList, view, proj);
	mTerrain->Render(commandList, view, proj);
    for (int i = 0; i < kMissilePoolSize; i++)
        mMissilePool[i]->Render(commandList, view, proj);

    ImGui::Begin("Terrain");
    {
        XMFLOAT3 ts = mTerrain->GetScale();
        XMFLOAT3 tp = mTerrain->GetPosition();
        bool changed = false;
        changed |= ImGui::DragFloat ("Height Scale (Y)", &ts.y,  0.05f, 0.01f, 20.f);
        changed |= ImGui::DragFloat("X Scale",         &ts.x,  0.05f, 0.01f, 20.f);
        changed |= ImGui::DragFloat("Z Scale",         &ts.z,  0.05f, 0.01f, 20.f);
        changed |= ImGui::DragFloat ("Y Offset",         &tp.y,  0.5f, -50.f,  50.f);
        if (changed)
        {
            mTerrain->SetScale(ts);
            mTerrain->SetPosition(tp);
        }
    }
    ImGui::End();

    ImGui::Begin("Level 1");
    ImGui::Text("Camera: %s (V to toggle)", mFirstPerson ? "1인칭" : "3인칭");
    ImGui::Text("Pos: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
    ImGui::Text("Heli: (%.1f, %.1f, %.1f)",
        mHelicopter->GetPosition().x,
        mHelicopter->GetPosition().y,
        mHelicopter->GetPosition().z);
    ImGui::Text("Ring Pos: (%.1f, %.1f, %.1f)",
        mRing->GetPosition().x, mRing->GetPosition().y, mRing->GetPosition().z);
    ImGui::Text("N: Level2  |  ESC: Menu");
    if (mFirstPerson)
        ImGui::DragFloat3("FPV Offset", &mFpvOffset.x, 0.01f);
    ImGui::Separator();
    ImGui::DragFloat3("Missile L", &mHelicopter->mMissileOffset1.x, 0.05f);
    ImGui::DragFloat3("Missile R", &mHelicopter->mMissileOffset2.x, 0.05f);
    ImGui::Separator();
    ImGui::End();
}

void Level_1_Scene::Release()
{
    Input::UnlockCursor();
    ClipCursor(nullptr);
    debugMode = true;
    for (auto obj : mGameObjects)
        delete obj;
    mGameObjects.clear();
    mRing  = nullptr;
    mPlane = nullptr;
    mHelicopter.reset();
 
    for (int i = 0; i < kMissilePoolSize; i++)
    {
        delete mMissilePool[i];
        mMissilePool[i] = nullptr;
    }
}
