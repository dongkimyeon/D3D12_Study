#include "stdafx.h"
#include "SceneManager.h"
#include "Time.h"
#include "Input.h"

std::map<std::wstring, Scene*> SceneManager::mScenes;
Scene* SceneManager::mActiveScene = nullptr;
std::wstring SceneManager::mPendingSceneName;



void SceneManager::Initialize()
{
	Time::Initialize();
	
	Input::Initialize();
}

Scene* SceneManager::LoadScene(const std::wstring& name)
{
	mPendingSceneName = name;
	return nullptr;
}

void SceneManager::ApplyPendingScene()
{
	if (mPendingSceneName.empty()) return;

	auto iter = mScenes.find(mPendingSceneName);
	mPendingSceneName.clear();

	if (iter == mScenes.end()) return;

	if (mActiveScene != nullptr)
		mActiveScene->Release();

	mActiveScene = iter->second;
	mActiveScene->Initialize();
}

void SceneManager::Update()
{

	if (mActiveScene != nullptr)
	{
		
		mActiveScene->Update(Time::GetDeltaTime());
		
	

		
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