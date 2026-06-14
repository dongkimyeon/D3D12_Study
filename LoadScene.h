#pragma once
#include "SceneManager.h"
#include "Level_1_Scene.h"
#include "Level_2_Scene.h"
#include "TitleScene.h"
#include "MenuScene.h"

void LoadScenes()
{
    SceneManager::CreateScene<Level_1_Scene>(L"Level_1");
    SceneManager::CreateScene<Level_2_Scene>(L"Level_2");
    SceneManager::CreateScene<TitleScene>(L"TitleScene");
    SceneManager::CreateScene<MenuScene>(L"MenuScene");

    SceneManager::LoadScene(L"TitleScene");
}
