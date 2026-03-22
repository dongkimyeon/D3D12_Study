#include "stdafx.h"
#include "TestScene.h"
#include "Cube.h" // <== 새로 만든 Cube 헤더 추가

TestScene::TestScene()
{

    camPos = { 0, 0, -10 };
    camYaw = 0.0f;
    camPitch = 0.0f;
    moveSpeed = 20.0f;
    lookSpeed = 0.006f;
    isRotating = false;
    prevMousePos = { 0, 0 };
}

TestScene::~TestScene()
{
}

void TestScene::Initialize()
{
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
    ImGuiIO& io = ImGui::GetIO();

    // 1. 카메라 마우스 회전 제어
    if (!io.WantCaptureMouse && Input::GetKey(eKeyCode::LButton))
    {
        POINT currMousePos;
        GetCursorPos(&currMousePos);

        if (!isRotating) { prevMousePos = currMousePos; isRotating = true; }

        camYaw += (currMousePos.x - prevMousePos.x) * lookSpeed;
        camPitch += (currMousePos.y - prevMousePos.y) * lookSpeed;
        const float limit = XM_PIDIV2 - 0.1f;
        if (camPitch > limit) camPitch = limit;
        if (camPitch < -limit) camPitch = -limit;

        prevMousePos = currMousePos;
    }
    else { isRotating = false; }

    // 2. 카메라 이동
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);
    XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1, 0, 0, 0), camRot);
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR pos = XMLoadFloat3(&camPos);

    if (Input::GetKey(eKeyCode::W)) pos += forward * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::S)) pos -= forward * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::D)) pos += right * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::A)) pos -= right * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::E)) pos += up * moveSpeed * dt;
    if (Input::GetKey(eKeyCode::Q)) pos -= up * moveSpeed * dt;

	if (Input::GetKeyDown(eKeyCode::SHIFT)) moveSpeed *= 2.0f;
    if (Input::GetKeyUp(eKeyCode::SHIFT)) moveSpeed /= 2.0f;

    XMStoreFloat3(&camPos, pos);

    // 3. 오브젝트들 업데이트 (다형성 활용)
    for (auto obj : mGameObjects) {
        obj->Update(dt);
    }
}

void TestScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
    // 뷰, 프로젝션 행렬 계산
    XMVECTOR currentPos = XMLoadFloat3(&camPos);
    XMMATRIX camRot = XMMatrixRotationRollPitchYaw(camPitch, camYaw, 0);
    XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0, 0, 1, 0), camRot);
    XMMATRIX view = XMMatrixLookToLH(currentPos, forward, XMVectorSet(0, 1, 0, 0));
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

    // 활성화된 오브젝트들을 모두 드로우 (다형성 활용)
    for (auto obj : mGameObjects) {
        obj->Render(commandList, view, proj);
    }

    // 설정 UI 
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Camera Position: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
    ImGui::End();
}

void TestScene::Release()
{
    for (auto obj : mGameObjects) {
        delete obj;
    }
    mGameObjects.clear();
}