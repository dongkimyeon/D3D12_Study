#pragma once
#include "stdafx.h"
#include "Scene.h"
#include <map>

class SceneManager
{
public:
	static void Initialize();
	static void Update();
	static void Render(ComPtr<ID3D12GraphicsCommandList>& commandList);
	static void Release();

	// 씬 생성 및 등록 템플릿
	template <typename T>
	static Scene* CreateScene(const std::wstring& name)
	{
		T* newScene = new T();
		newScene->SetName(name); // Entity에서 상속받은 이름 설정 함수
		newScene->Initialize();

		mScenes.insert({ name, newScene });
		return newScene;
	}

	static Scene* LoadScene(const std::wstring& name);

private:
	static std::map<std::wstring, Scene*> mScenes;
	static Scene* mActiveScene;
};