#pragma once
#include "SceneManager.h"
#include "TestScene.h"


void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
    SceneManager::CreateScene<TestScene>(L"TestScene");
    SceneManager::LoadScene(L"TestScene");
}