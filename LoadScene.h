#pragma once
#include "SceneManager.h"
#include "TestScene.h"
#include "TitleScene.h"
#include "MapSelectScene.h"

void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
	SceneManager::CreateScene<TitleScene>(L"TitleScene");
    SceneManager::CreateScene<TestScene>(L"TestScene");
	SceneManager::CreateScene<MapSelectScene>(L"MapSelectScene");

    SceneManager::LoadScene(L"TitleScene");
}