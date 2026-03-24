#include "stdafx.h"
#include "TestScene.h"
#include "Cube.h"
#include "Plane.h"
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

	GameObject* floorPlane = new Plane();
	floorPlane->Initialize(Framework::GetDevice());
	floorPlane->SetPosition(0.0f, -5.0f, 0.0f); // 큐브 무리 아래쪽(-5.0f)에 배치
	mGameObjects.push_back(floorPlane);

    int gridSize = 3;       // 3x3x3 형태로 총 27개의 큐브 생성
    float spacing = 3.0f;   // 큐브 사이의 간격

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

                cube->SetPosition(posX, posY, posZ);
                mGameObjects.push_back(cube);
            }
        }
    }
}

void TestScene::Update(float dt)
{
   
    // 3. 오브젝트들 업데이트 (다형성 활용)
    for (auto obj : mGameObjects) {
        obj->Update(dt);
    }
}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
   
    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), Camera::camForward, XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

    // 활성화된 오브젝트들을 모두 드로우 (다형성 활용)
    for (auto obj : mGameObjects) {
        obj->Render(commandList, view, proj);
    }

    // 설정 UI 
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
    ImGui::End();
}

void TestScene::Release()
{
    for (auto obj : mGameObjects) {
        delete obj;
    }
    mGameObjects.clear();
}