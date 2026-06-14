#include "MenuScene.h"
#include "Camera.h"
#include "stdafx.h"
#include "framework.h"

#include "SceneManager.h"
#include "PickingUtils.h"

MenuScene::MenuScene()
{
}

MenuScene::~MenuScene()
{
}

void MenuScene::Initialize()
{

	Camera::camPitch = 0.0f;
	Camera::SetPosition(0, -2.5f, -45.0f);

	auto makeLabel = [&](const char* file, float y) {
		auto* obj = new LetterObj();
		obj->Initialize(Framework::GetDevice(), file);
		obj->SetPosition(0, y, 0);
		mGameObjects.push_back(obj);
		return obj;
	};

	makeLabel("SM_TUTORIAL.obj",  10.0f);
	makeLabel("SM_LEVEL1.obj",     5.0f);
	makeLabel("SM_LEVEL2.obj",     0.0f);
	makeLabel("SM_LEVEL3.obj",    -5.0f);
	Start = makeLabel("SM_START.obj", -10.0f);
	End   = makeLabel("SM_END.obj",   -15.0f);

}

void MenuScene::Update(float dt)
{
	if (Input::GetKeyDown(eKeyCode::LButton))
	{
		if (PickAABB(Start->GetWorldAABB()))
			SceneManager::LoadScene(L"Level_1");
		else if (PickAABB(End->GetWorldAABB()))
		{
			SceneManager::Release();
			exit(0);
		}
	}

	for (auto obj : mGameObjects)
	{
		obj->Update(dt);
	}
}

void MenuScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, (float)Framework::GetWidth() / (float)Framework::GetHeight(), 0.1f, 100.0f);

	for (auto obj : mGameObjects)
	{
		obj->Render(commandList, view, proj);
	}
}

void MenuScene::Release()
{
	for (auto obj : mGameObjects)
		delete obj;
	mGameObjects.clear();
	Start = End = nullptr;
}
