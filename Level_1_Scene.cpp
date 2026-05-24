#include "stdafx.h"
#include "framework.h"
#include "Level_1_Scene.h"
#include "Gizumo.h"
#include "Camera.h"
#include "Map.h"
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

	Camera::SetPosition(0.f, 5.f, -15.f);

	//GameObject* gizumo = new Gizumo();
	//gizumo->Initialize(Framework::GetDevice());
	//mGameObjects.push_back(gizumo);

	mHelicopter = std::make_unique<Helicopter>();
	mHelicopter->Initialize(Framework::GetDevice());

	mMap = new Map();
	mMap->Initialize(Framework::GetDevice());
	RebuildMapInstances();
	mGameObjects.push_back(mMap);



}

void Level_1_Scene::Update(float dt)
{
	if (Input::GetKeyDown(eKeyCode::ESC))
		SceneManager::LoadScene(L"MenuScene");

	mHelicopter->Update(dt);

	// 3인칭 카메라 팔로우
	XMFLOAT3 heliPos = mHelicopter->GetPosition();
	float heading    = mHelicopter->GetHeading();

	// 헬기 뒤쪽 + 위쪽 오프셋
	const float camDist   = 15.f;
	const float camHeight =  5.f;
	XMFLOAT3 targetCamPos = {
		heliPos.x - sinf(heading) * camDist,
		heliPos.y + camHeight,
		heliPos.z - cosf(heading) * camDist
	};

	// 부드러운 카메라 추적 (보간)
	const float camSpeed = 8.f;
	float t = 1.f - expf(-camSpeed * dt);
	XMVECTOR camPos    = XMLoadFloat3(&Camera::camPos);
	XMVECTOR targetPos = XMLoadFloat3(&targetCamPos);
	XMStoreFloat3(&Camera::camPos, XMVectorLerp(camPos, targetPos, t));

	// 헬기 중심 바라보기
	XMVECTOR lookDir = XMVector3Normalize(
		XMLoadFloat3(&heliPos) - XMLoadFloat3(&Camera::camPos));
	XMStoreFloat3(&Camera::camForward, lookDir);

	for (const auto& obj : mGameObjects)
		obj->Update(dt);
}

void Level_1_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);

	BoundingFrustum frustum(proj);
	BoundingFrustum worldFrustum;
	frustum.Transform(worldFrustum, XMMatrixInverse(nullptr, view));

	ApplyFrustumCulling(worldFrustum);

	for (const auto& obj : mGameObjects)
	{
		if (obj != mMap)
		{
			if (!worldFrustum.Intersects(obj->GetWorldAABB()))
				continue;
		}
		obj->Render(commandList, view, proj);
	}

	mHelicopter->Render(commandList, view, proj);

	ImGui::Begin("Settings");
	ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
	ImGui::Separator();

	ImGui::End();
}

void Level_1_Scene::RebuildMapInstances()
{
	constexpr int GRID = 5;
	float offsetX = (GRID - 1) * mMapSpacingX * 0.5f;
	float offsetZ = (GRID - 1) * mMapSpacingZ * 0.5f;

	mMap->ClearInstances();
	mAllTileMatrices.clear();
	for (int z = 0; z < GRID; z++)
		for (int x = 0; x < GRID; x++)
		{
			XMFLOAT3 pos = { x * mMapSpacingX - offsetX, 0.f, z * mMapSpacingZ - offsetZ };
			mMap->AddInstance(pos);

			XMFLOAT4X4 w;
			XMStoreFloat4x4(&w, XMMatrixTranslation(pos.x, pos.y, pos.z));
			mAllTileMatrices.push_back(w);
		}

	mMap->BuildInstanceBuffer(Framework::GetDevice());
}

void Level_1_Scene::ApplyFrustumCulling(const DirectX::BoundingFrustum& worldFrustum)
{
	float radius = sqrtf(mMapSpacingX * mMapSpacingX + mMapSpacingZ * mMapSpacingZ) * 0.5f;

	std::vector<XMFLOAT4X4> visible;
	visible.reserve(mAllTileMatrices.size());
	for (const auto& mat : mAllTileMatrices)
	{
		BoundingSphere sphere({ mat._41, mat._42, mat._43 }, radius);
		if (worldFrustum.Intersects(sphere))
			visible.push_back(mat);
	}

	mVisibleTileCount = (int)visible.size();
	mMap->UpdateInstancesForCulling(visible);
}

void Level_1_Scene::Release()
{
	debugMode = true;
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	mMap = nullptr;
	mHelicopter.reset();
	mAllTileMatrices.clear();
}
