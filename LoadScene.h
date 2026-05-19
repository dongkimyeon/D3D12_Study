#pragma once
#include "SceneManager.h"
#include "Map1Scene.h"
#include "Map2Scene.h"
#include "TitleScene.h"
#include "MapSelectScene.h"

void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
	SceneManager::CreateScene<TitleScene>(L"TitleScene");
	SceneManager::CreateScene<MapSelectScene>(L"MapSelectScene");
	SceneManager::CreateScene<Map1Scene>(L"Map1Scene");
	SceneManager::CreateScene<Map2Scene>(L"Map2Scene");
	

    SceneManager::LoadScene(L"TitleScene");
}