#pragma once
#include "SceneManager.h"
#include "TestScene.h"
#include "TitleScene.h"


void LoadScenes()
{
    // 씬을 생성하고 바로 활성화
	SceneManager::CreateScene<TitleScene>(L"TitleScene");
    SceneManager::CreateScene<TestScene>(L"TestScene");
	
    SceneManager::LoadScene(L"TitleScene");
}