#include "stdafx.h"
#include "TestScene.h"
#include "Cube.h"
#include "Plane.h"
#include "Gizumo.h"
#include "framework.h"
#include "Camera.h"
#include "Utiles.h"

TestScene::TestScene()
{

}

TestScene::~TestScene()
{
}

void TestScene::Initialize()
{
	// ============================================
	GameObject* gizumo = new Gizumo();
	gizumo->Initialize(Framework::GetDevice());
	mGameObjects.push_back(gizumo);

	for (int i = 0; i < 2; ++i)
	{
		GameObject* floorPlane = new Plane();
		floorPlane->Initialize(Framework::GetDevice());
		floorPlane->SetPosition(0, 15.0f * i, 0); // 각 평면을 y축으로 5 단위씩 떨어뜨려 배치
		if (i == 1) {
			floorPlane->SetRotation(-XM_PI, 0, 0); // 두 번째 평면은 x축으로 90도 회전
		}
		mGameObjects.push_back(floorPlane);

	}
	
	GameObject* testCube = new Cube();
	testCube->Initialize(Framework::GetDevice());
	testCube->SetPosition(0, 7.5f, 0);
	mGameObjects.push_back(testCube);
	//int gridSize = 15;       // 3x3x3 형태로 총 27개의 큐브 생성
	//float spacing = 5.0f;   // 큐브 사이의 간격

	//for (int x = 0; x < gridSize; ++x) {
	//	for (int y = 0; y < gridSize; ++y) {
	//		for (int z = 0; z < gridSize; ++z) {
	//			GameObject* cube = new Cube();
	//			cube->Initialize(Framework::GetDevice());
	//			cube->SetPosition(x * spacing, y * spacing, z * spacing);
	//			mGameObjects.push_back(cube);
	//		}
	//	}
	//}
}

void TestScene::Update(float dt)
{
	// 1. 모든 게임 오브젝트 업데이트 (중력 등 적용)
	for (const auto& obj : mGameObjects) {
		obj->Update(dt);
	}

	// 2. 충돌 감지 및 반응 (평면의 방정식 사용)
	for (auto& objA : mGameObjects) {
		Cube* cube = dynamic_cast<Cube*>(objA);
		if (!cube) continue; // 큐브가 아니면 패스

		for (auto& objB : mGameObjects) {
			Plane* plane = dynamic_cast<Plane*>(objB);
			if (!plane) continue; // 평면이 아니면 패스

			// --- 평면의 정보 추출 ---
			// 평면의 Normal은 기본적으로 (0, 1, 0)이며, 평면의 Rotation에 의해 변함
			XMMATRIX planeRot = XMMatrixRotationRollPitchYaw(plane->rotation.x, plane->rotation.y, plane->rotation.z);
			XMVECTOR planeNormal = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), planeRot);
			XMVECTOR planePos = XMLoadFloat3(&plane->position);
			XMVECTOR cubePos = XMLoadFloat3(&cube->position);

			// --- 점과 평면 사이의 거리 계산 (D = n · (Q - P0)) ---
			XMVECTOR vecToCube = XMVectorSubtract(cubePos, planePos);
			XMVECTOR distVec = XMVector3Dot(planeNormal, vecToCube);

			float distance = XMVectorGetX(distVec);

			// 큐브의 절반 크기 (바닥면까지의 거리)
			float halfHeight = cube->scale.y * 0.5f;

			// 충돌 조건: 거리가 큐브의 절반보다 작을 때
			if (distance < halfHeight)
			{
				// 충돌 반응: 평면의 법선 방향으로 큐브를 밀어냄 (Penetration Resolution)
				float penetrationDepth = halfHeight - distance;
				XMVECTOR correction = XMVectorScale(planeNormal, penetrationDepth);

				XMVECTOR correctedPos = XMVectorAdd(cubePos, correction);
				XMStoreFloat3(&cube->position, correctedPos);

			}
		}
	}
}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{

    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);


  
    for (const auto& obj : mGameObjects) {
        obj->Render(commandList, view, proj);
    }

    // 설정 UI 
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);

	ImGui::Separator();
	for (const auto& obj : mGameObjects) {
		Cube* cube = dynamic_cast<Cube*>(obj);
		if (cube) {
			// Render 함수 내 반복문 안에서 교체해서 테스트해보세요.
			// 1. 방향 구하기
			XMFLOAT3 toObj;
			XMVECTOR toObjVec;
			XMVECTOR camPosVec = XMLoadFloat3(&Camera::camPos);
			XMVECTOR objPosVec = XMLoadFloat3(&cube->position);

			toObjVec = XMVectorSubtract(objPosVec, camPosVec);

			// 2. 정규화 (Scalar 방식은 루트를 직접 계산해야 함)
			float length = XMVectorGetX(XMVector3Length(toObjVec));
			toObjVec = XMVectorScale(toObjVec, 1.0f / length);

			// 3. 내적 (Dot Product) 직접 계산
			float fDot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Camera::camForward), toObjVec));

			// 4. 외적 (Cross Product) 계산
			XMVECTOR cross = XMVector3Cross(XMLoadFloat3(&Camera::camForward), toObjVec);

			// 5. 스칼라 삼중적 마무리
			float tripleProduct = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Camera::camForward), cross));
		}
	}

	// 오브젝트 목록에서 Plane 타입 찾기
	for (const auto& obj : mGameObjects)
	{
		Plane* plane = dynamic_cast<Plane*>(obj);
		if (plane != nullptr)
		{
			XMFLOAT3 pos = plane->position;
		
			if (ImGui::DragFloat3("Plane Position", &pos.x, 0.1f))
			{
				// 위치와 월드 매트릭스를 함께 갱신해줌
				plane->SetPosition(pos.x, pos.y, pos.z);
			}
			break; 
		}
	}

	ImGui::End();

	ImGui::Begin("Object Control");

	// 1. 오브젝트 선택 리스트 생성
	int cubeDisplayCount = 0; 
	if (ImGui::BeginListBox("Cube List"))
	{
		for (int i = 0; i < mGameObjects.size(); ++i)
		{
			// 큐브 타입만 골라서 리스트에 표시
			Cube* cube = dynamic_cast<Cube*>(mGameObjects[i]);
			if (cube)
			{
				char label[32];
				sprintf_s(label, "Cube %d (Index: %d)", cubeDisplayCount++, i);

				// 리스트 아이템 클릭 시 mSelectedIndex 업데이트
				bool isSelected = (mSelectedIndex == i);
				if (ImGui::Selectable(label, isSelected))
				{
					mSelectedIndex = i;
				}

				// 포커스 설정 (선택된 항목으로 스크롤)
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndListBox();
	}

	ImGui::Separator();

	// 2. 선택된 큐브의 데이터 수정
	if (mSelectedIndex != -1)
	{
		GameObject* selectedObj = mGameObjects[mSelectedIndex];
		XMFLOAT3 pos = selectedObj->position; 
		XMFLOAT3 rot = selectedObj->rotation; 
		XMVECTOR forward = XMLoadFloat4(&selectedObj->forward_vector);

		ImGui::Text("Editing: Cube %d", mSelectedIndex);
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
		{
			selectedObj->SetPosition(pos.x, pos.y, pos.z);
		}
		ImGui::Separator();
		if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
		{
			selectedObj->SetRotation(rot.x, rot.y, rot.z);
		}
	}
	else
	{
		ImGui::Text("Select a cube from the list.");
	}

    ImGui::End();
}

void TestScene::Release()
{
    for (const auto& obj : mGameObjects) {
        delete obj;
    }
    mGameObjects.clear();
}