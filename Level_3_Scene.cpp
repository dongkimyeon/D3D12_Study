#include "stdafx.h"
#include "Level_3_Scene.h"
#include "framework.h"
#include "Camera.h"
#include "SceneManager.h"

extern bool debugMode;

Level_3_Scene::Level_3_Scene() {}
Level_3_Scene::~Level_3_Scene() {}

void Level_3_Scene::Initialize()
{
    debugMode = false;

    Input::LockCursor(Framework::GetHwnd());

    auto device = Framework::GetDevice();

    mTerrain = std::make_unique<Terrain>();
    mTerrain->Initialize(device);
    mTerrain->SetScale(3.0f, 3.0f, 3.0f);

    mHelicopter = std::make_unique<Helicopter>();
    mHelicopter->Initialize(device);
    mHelicopter->SetPosition(0.f, 5.f, 0.f);

    Camera::SetPosition(0.f, 10.f, -15.f);

    for (int i = 0; i < kMissilePoolSize; ++i)
    {
        mMissilePool[i] = new Missile();
        mMissilePool[i]->Initialize(device);
    }

    // 지형 위 탱크 배치
    constexpr size_t numTanks = 15;

    mTankBodyRenderer   = std::make_unique<TankBody>();
    mTankLidRenderer    = std::make_unique<TankLid>();
    mTankBarrelRenderer = std::make_unique<TankBarrel>();
    mTankBodyRenderer->Initialize(device, numTanks);
    mTankLidRenderer->Initialize(device, numTanks);
    mTankBarrelRenderer->Initialize(device, numTanks);

    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<float> distX(-100.f, 100.f);
    std::uniform_real_distribution<float> distZ(40.f, 250.f);

    mTanks.reserve(numTanks);
    for (size_t i = 0; i < numTanks; ++i)
    {
        float rx = distX(gen);
        float rz = distZ(gen);
        float ry = mTerrain->GetHeightAt(rx, rz);

        auto tank = std::make_unique<Tank>();
        tank->SetPosition(rx, ry, rz);
        tank->Update(0.f);
        mTanks.emplace_back(std::move(tank));
    }
}

void Level_3_Scene::Update(float dt)
{
    if (Input::GetKeyDown(eKeyCode::ESC))
        SceneManager::LoadScene(L"MenuScene");

    if (Input::GetKeyDown(eKeyCode::V))
        mFirstPerson = !mFirstPerson;

    if (Input::GetKeyDown(eKeyCode::F5))
    {
        if (Input::IsCursorLocked()) Input::UnlockCursor();
        else                         Input::LockCursor(Framework::GetHwnd());
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
        const float camDist = 15.f, camHeight = 5.f;
        XMFLOAT3 targetCamPos = {
            heliPos.x - sinf(heading) * camDist,
            heliPos.y + camHeight,
            heliPos.z - cosf(heading) * camDist
        };
        float t = 1.f - expf(-8.f * dt);
        XMStoreFloat3(&Camera::camPos,
            XMVectorLerp(XMLoadFloat3(&Camera::camPos), XMLoadFloat3(&targetCamPos), t));
        XMStoreFloat3(&Camera::camForward,
            XMVector3Normalize(XMLoadFloat3(&heliPos) - XMLoadFloat3(&Camera::camPos)));
        Camera::camUp = { 0.f, 1.f, 0.f };
    }

    for (const auto& obj : mGameObjects)
        obj->Update(dt);

    if (Input::GetKeyDown(eKeyCode::LButton))
    {
        XMFLOAT3 pos1, pos2, dir;
        mHelicopter->GetFireData(pos1, pos2, dir);
        XMFLOAT3 spawnPos = mFireFromLeft ? pos1 : pos2;
        mFireFromLeft = !mFireFromLeft;

        for (int i = 0; i < kMissilePoolSize; ++i)
        {
            if (mMissilePool[i]->IsDead())
            {
                mMissilePool[i]->Spawn(spawnPos, dir);
                break;
            }
        }
    }

    for (int i = 0; i < kMissilePoolSize; ++i)
        mMissilePool[i]->Update(dt);

    // 탱크 인스턴스 매트릭스 업데이트
    mBodyMats.clear(); mLidMats.clear(); mBarrelMats.clear();
    for (auto& tank : mTanks)
    {
        if (!tank->IsAlive()) continue;
        mBodyMats.push_back(tank->mBodyMatrix);
        mLidMats.push_back(tank->mLidMatrix);
        mBarrelMats.push_back(tank->mBarrelMatrix);
    }
    mTankBodyRenderer->UpdateInstances(mBodyMats);
    mTankLidRenderer->UpdateInstances(mLidMats);
    mTankBarrelRenderer->UpdateInstances(mBarrelMats);

}

void Level_3_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
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

    mTankBodyRenderer->Render(commandList, view, proj);
    mTankLidRenderer->Render(commandList, view, proj);
    mTankBarrelRenderer->Render(commandList, view, proj);

    for (int i = 0; i < kMissilePoolSize; ++i)
        mMissilePool[i]->Render(commandList, view, proj);
}

void Level_3_Scene::Release()
{
    Input::UnlockCursor();
    ClipCursor(nullptr);
    debugMode = true;

    for (auto* obj : mGameObjects)
        delete obj;
    mGameObjects.clear();

    for (int i = 0; i < kMissilePoolSize; ++i)
    {
        delete mMissilePool[i];
        mMissilePool[i] = nullptr;
    }
}
