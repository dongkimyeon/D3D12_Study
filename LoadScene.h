#pragma once
#include "SceneManager.h"
#include "Level_1_Scene.h"
#include "TitleScene.h"
#include "MenuScene.h"	


void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
    SceneManager::CreateScene<Level_1_Scene>(L"Level_1");
	SceneManager::CreateScene<TitleScene>(L"TitleScene");
	SceneManager::CreateScene<MenuScene>(L"MenuScene");

    SceneManager::LoadScene(L"TitleScene");
}