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


	Tutorial = new LetterObj();
	Tutorial->Initialize(Framework::GetDevice(), "SM_TUTORIAL.obj");
	Tutorial->SetPosition(0, 10.0f, 0);

	Level_1 = new LetterObj();
	Level_1->Initialize(Framework::GetDevice(), "SM_LEVEL1.obj");
	Level_1->SetPosition(0, 5.0f, 0);

	Level_2 = new LetterObj();
	Level_2->Initialize(Framework::GetDevice(), "SM_LEVEL2.obj");
	Level_2->SetPosition(0, 0.0f, 0);
	Level_3 = new LetterObj();
	Level_3->Initialize(Framework::GetDevice(), "SM_LEVEL3.obj");
	Level_3->SetPosition(0, -5.0f, 0);

	Start = new LetterObj();
	Start->Initialize(Framework::GetDevice(), "SM_START.obj");
	Start->SetPosition(0, -10.0f, 0);

	End = new LetterObj();
	End->Initialize(Framework::GetDevice(), "SM_END.obj");
	End->SetPosition(0, -15.0f, 0);


	mGameObjects.push_back(Tutorial);
	mGameObjects.push_back(Level_1);
	mGameObjects.push_back(Level_2);
	mGameObjects.push_back(Level_3);
	mGameObjects.push_back(Start);
	mGameObjects.push_back(End);


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
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

	for (auto obj : mGameObjects)
	{
		obj->Render(commandList, view, proj);
	}
}

void MenuScene::Release()
{

}
