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

	GameObject* floorPlane = new Plane();
	floorPlane->Initialize(Framework::GetDevice());
	mGameObjects.push_back(floorPlane);

	GameObject* testCube = new Cube();
	testCube->Initialize(Framework::GetDevice());
	testCube->SetPosition(0, 0, 0);
	mGameObjects.push_back(testCube);
    //int gridSize = 2;       // 3x3x3 형태로 총 27개의 큐브 생성
    //float spacing = 5.0f;   // 큐브 사이의 간격

    //for (int x = 0; x < gridSize; ++x)
    //{
    //    for (int y = 0; y < gridSize; ++y)
    //    {
    //        for (int z = 0; z < gridSize; ++z)
    //        {
    //            GameObject* cube = new Cube();
    //            cube->Initialize(Framework::GetDevice());

    //            // 전체 큐브 무리의 중심이 원점(0, 0, 0)이 되도록 좌표 오프셋 계산
    //            float posX = (x - (gridSize - 1) / 2.0f) * spacing;
    //            float posY = (y - (gridSize - 1) / 2.0f) * spacing;
    //            float posZ = (z - (gridSize - 1) / 2.0f) * spacing;

    //            cube->SetPosition(posX, posY + 30, posZ);
				//cube->SetScale(2.0f, 2.0f, 2.0f); // 큐브 크기를 2배로 키움
    //            mGameObjects.push_back(cube);
    //        }
    //    }
    //}
}

void TestScene::Update(float dt)
{
	

   
    for (const auto& obj : mGameObjects) {
        obj->Update(dt);
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
			// 1. "목표 좌표 - 내 좌표" (교수님 강조: 나를 원점으로 만들어 상대적 위치 파악)
			DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&cube->position);
			DirectX::XMVECTOR myPos = DirectX::XMLoadFloat3(&Camera::camPos);
			// 방향 벡터 추출 (정규화는 내적/삼중적 결과의 '순수 부호'를 보기 위해 수행)
			DirectX::XMVECTOR toObjVec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(targetPos, myPos));

			// 2. 카메라의 로컬 기저 벡터 로드 (회전이 이미 반영된 상태)
			DirectX::XMVECTOR forward = DirectX::XMLoadFloat3(&Camera::camForward);
			DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&Camera::camUp);

			// ---------------------------------------------------------
			// 3. 앞/뒤 판별 (내적: Dot Product)
			// ---------------------------------------------------------
			DirectX::XMVECTOR dotFront = DirectX::XMVector3Dot(forward, toObjVec);
			float fDot;
			DirectX::XMStoreFloat(&fDot, dotFront);

			// ---------------------------------------------------------
			// 4. 좌/우 판별 (스칼라 삼중적: Scalar Triple Product)
			// 공식: Up · (Forward × toObj) -> Up이 회전축 역할
			// ---------------------------------------------------------
			DirectX::XMVECTOR crossVec = DirectX::XMVector3Cross(forward, toObjVec);
			DirectX::XMVECTOR dotSide = DirectX::XMVector3Dot(up, crossVec);
			float tripleProduct;
			DirectX::XMStoreFloat(&tripleProduct, dotSide);

			// ---------------------------------------------------------
			// 5. ImGui 출력 및 판별 로직
			// ---------------------------------------------------------
			const char* vertical = (fDot > 0) ? "Front" : "Back";
			const char* horizontal = (tripleProduct > 0) ? "Right" : "Left";

			// 만약 값이 거의 0이라면 "Center"라고 표시할 수도 있습니다 (Deadzone 처리)
			if (fabs(fDot) < 0.01f) vertical = "Side-aligned";
			if (fabs(tripleProduct) < 0.01f) horizontal = "Center";

			ImGui::Text("Object Position Relative to Camera:");
			ImGui::Text("Vertical: %s (Value: %.2f)", vertical, fDot);
			ImGui::Text("Horizontal: %s (Value: %.2f)", horizontal, tripleProduct);
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

	//ImGui::Begin("Object Control");

	//// 1. 오브젝트 선택 리스트 생성
	//int cubeDisplayCount = 0; 
	//if (ImGui::BeginListBox("Cube List"))
	//{
	//	for (int i = 0; i < mGameObjects.size(); ++i)
	//	{
	//		// 큐브 타입만 골라서 리스트에 표시
	//		Cube* cube = dynamic_cast<Cube*>(mGameObjects[i]);
	//		if (cube)
	//		{
	//			char label[32];
	//			sprintf_s(label, "Cube %d (Index: %d)", cubeDisplayCount++, i);

	//			// 리스트 아이템 클릭 시 mSelectedIndex 업데이트
	//			bool isSelected = (mSelectedIndex == i);
	//			if (ImGui::Selectable(label, isSelected))
	//			{
	//				mSelectedIndex = i;
	//			}

	//			// 포커스 설정 (선택된 항목으로 스크롤)
	//			if (isSelected)
	//				ImGui::SetItemDefaultFocus();
	//		}
	//	}
	//	ImGui::EndListBox();
	//}

	//ImGui::Separator();

	//// 2. 선택된 큐브의 데이터 수정
	//if (mSelectedIndex != -1)
	//{
	//	GameObject* selectedObj = mGameObjects[mSelectedIndex];
	//	XMFLOAT3 pos = selectedObj->position; 
	//	XMFLOAT3 rot = selectedObj->rotation; 
	//	XMVECTOR forward = XMLoadFloat4(&selectedObj->forward_vector);

	//	ImGui::Text("Editing: Cube %d", mSelectedIndex);
	//	if (ImGui::DragFloat3("Position", &pos.x, 0.1f))
	//	{
	//		selectedObj->SetPosition(pos.x, pos.y, pos.z);
	//	}
	//	ImGui::Separator();
	//	if (ImGui::DragFloat3("Rotation", &rot.x, 0.1f))
	//	{
	//		selectedObj->SetRotation(rot.x, rot.y, rot.z);
	//	}
	//}
	//else
	//{
	//	ImGui::Text("Select a cube from the list.");
	//}

 //   ImGui::End();
}

void TestScene::Release()
{
    for (const auto& obj : mGameObjects) {
        delete obj;
    }
    mGameObjects.clear();
}