#include "stdafx.h"
#include "TestScene.h"
#include "Cube.h"
#include "Plane.h"
#include "Gizumo.h"
#include "framework.h"
#include "Camera.h"

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

    int gridSize = 3;       // 3x3x3 형태로 총 27개의 큐브 생성
    float spacing = 10.0f;   // 큐브 사이의 간격

    for (int x = 0; x < gridSize; ++x)
    {
        for (int y = 0; y < gridSize; ++y)
        {
            for (int z = 0; z < gridSize; ++z)
            {
                GameObject* cube = new Cube();
                cube->Initialize(Framework::GetDevice());

                // 전체 큐브 무리의 중심이 원점(0, 0, 0)이 되도록 좌표 오프셋 계산
                float posX = (x - (gridSize - 1) / 2.0f) * spacing;
                float posY = (y - (gridSize - 1) / 2.0f) * spacing;
                float posZ = (z - (gridSize - 1) / 2.0f) * spacing;

                cube->SetPosition(posX, posY + 30, posZ);
				cube->SetScale(2.0f, 2.0f, 2.0f); // 큐브 크기를 2배로 키움
                mGameObjects.push_back(cube);
            }
        }
    }
}

void TestScene::Update(float dt)
{
   
    for (const auto& obj : mGameObjects) {
        obj->Update(dt);
    }
}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
   
    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), Camera::camForward, XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

  
    for (const auto& obj : mGameObjects) {
        obj->Render(commandList, view, proj);
    }

    // 설정 UI 
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
	ImGui::Separator();
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);

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