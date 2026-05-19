#include "stdafx.h"
#include "SceneManager.h"
#include "Time.h"
#include "Input.h"
#include "GameObject.h"
#include "Camera.h"

extern bool debugMode;

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
		iter->second->Initialize();
		return mActiveScene;
	}

	return nullptr;
}

void SceneManager::Update()
{

	if (mActiveScene != nullptr)
	{
		
		mActiveScene->Update(Time::GetDeltaTime());
		
		if (Input::GetKeyDown(eKeyCode::F1))
			GameObject::sShowAABB = !GameObject::sShowAABB;

		if (Input::GetKeyDown(eKeyCode::F6)) {
			debugMode = !debugMode;
			ShowCursor(debugMode ? TRUE : FALSE);
		}

		if (!debugMode && Input::GetKeyDown(eKeyCode::F5)) {
			Camera::sMode = (Camera::sMode == eCameraMode::FirstPerson)
				? eCameraMode::ThirdPerson
				: eCameraMode::FirstPerson;
		}

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