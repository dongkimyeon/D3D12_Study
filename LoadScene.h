#pragma once
#include "SceneManager.h"
#include "Level_1_Scene.h"
#include "TitleScene.h"


void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
    SceneManager::CreateScene<Level_1_Scene>(L"Level_1");
	SceneManager::CreateScene<TitleScene>(L"TitleScene");
    SceneManager::LoadScene(L"TitleScene");
}