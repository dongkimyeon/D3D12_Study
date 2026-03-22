#include "stdafx.h"
#include "SceneManager.h"
#include "Time.h"
#include "Input.h"

std::map<std::wstring, Scene*> SceneManager::mScenes;
Scene* SceneManager::mActiveScene = nullptr;

void SceneManager::Initialize()
{
	Time::Initialize();
	Input::Initialize();
}

Scene* SceneManager::LoadScene(const std::wstring& name)
{
	auto iter = mScenes.find(name);

	if (iter != mScenes.end())
	{
		mActiveScene = iter->second;
		return mActiveScene;
	}

	return nullptr;
}

void SceneManager::Update()
{

	if (mActiveScene != nullptr)
	{
		// 프레임 전역 델타 타임을 넘겨줌
		mActiveScene->Update(Time::GetDeltaTime());

		if (Input::GetKeyDown(eKeyCode::ESC))
		{
			Release();
			exit(0);
		}
	}
}

void SceneManager::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	if (mActiveScene != nullptr)
	{
		mActiveScene->Render(commandList);
	}
}

void SceneManager::Release()
{
	for (auto& pair : mScenes)
	{
		pair.second->Release();
		delete pair.second;
	}
	mScenes.clear();
	mActiveScene = nullptr;
}