#include "stdafx.h"
#include "Level_2_Scene.h"
#include "SceneManager.h"
#include "Camera.h"
#include "Input.h"

void Level_2_Scene::Initialize()
{
    Camera::SetPosition(0.f, 10.f, -20.f);
    Camera::camForward = { 0.f, 0.f, 1.f };
    Camera::camUp      = { 0.f, 1.f, 0.f };
}

void Level_2_Scene::Update(float dt)
{
    if (Input::GetKeyDown(eKeyCode::ESC))
        SceneManager::LoadScene(L"MenuScene");
}

void Level_2_Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
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
