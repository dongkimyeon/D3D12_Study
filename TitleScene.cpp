#include "TitleScene.h"
#include "Camera.h"
#include "stdafx.h"
#include "framework.h"
#include "SceneManager.h"
#include "PickingUtils.h"

TitleScene::TitleScene()
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize()
{

	Camera::camPitch = 0.0f;
	Camera::SetPosition(0, 0, -30.0f);


	mTitleText1 = new LetterObj();
	mTitleText1->Initialize(Framework::GetDevice(), "SM_Title1.obj");
	mTitleText1->SetPosition(0, 5.0f, 0);	

	mTitleText2 = new LetterObj();
	mTitleText2->Initialize(Framework::GetDevice(), "SM_Title2.obj");

	mName = new LetterObj();
	mName->Initialize(Framework::GetDevice(), "SM_NAME.obj");
	mName->SetPosition(0, -5.0f, 0);

	mGameObjects.push_back(mTitleText1);
	mGameObjects.push_back(mTitleText2);
	mGameObjects.push_back(mName);
}

void TitleScene::Update(float dt)
{
	float rotSpeed = 1.0f;
	mTitleText1->SetRotation({ mTitleText1->GetRotation().x, mTitleText1->GetRotation().y + rotSpeed * dt, mTitleText1->GetRotation().z });
	mTitleText2->SetRotation({ mTitleText2->GetRotation().x, mTitleText2->GetRotation().y + rotSpeed * dt, mTitleText2->GetRotation().z });

	if (Input::GetKeyDown(eKeyCode::LButton))
	{
		if (PickAABB(mName->GetWorldAABB()))
			SceneManager::LoadScene(L"MenuScene");
	}

	for(auto obj : mGameObjects)
	{
		obj->Update(dt);
	}
}

void TitleScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);

	for(auto obj : mGameObjects)
	{
		obj->Render(commandList, view, proj);
	}
}

void TitleScene::Release()
{
}
