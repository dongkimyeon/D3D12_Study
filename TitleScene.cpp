#include "TitleScene.h"
#include "SceneManager.h"
#include "Title.h"
#include "Start.h"
#include "Camera.h"
#include "framework.h"
TitleScene::TitleScene()
{
}

TitleScene::~TitleScene()
{
}

void TitleScene::Initialize()
{
	Camera::camPos = { 0, 0, -25};
	Camera::camPitch = 0;


	Title* title = new Title();
	title->Initialize(Framework::GetDevice());
	title->SetPosition(0, 3.0f, 0);
	title->SetScale(0.05f, 0.05f, 0.05f);
	title->SetRotation(0, 90.0f, 0);
	mGameObjects.push_back(title);

	Start* start = new Start();
	start->Initialize(Framework::GetDevice());
	start->SetPosition(0, -5.0f, 0);
	start->SetScale(0.05f, 0.05f, 0.05f);
	start->SetRotation(0, 90.0f, 0);
	mGameObjects.push_back(start);

}

void TitleScene::Update(float dt)
{
	for (const auto& obj : mGameObjects) {
		obj->Update(dt);
	}

	if(Input::GetKeyDown(eKeyCode::SPACE)) // 다음 씬으로 넘어가는 조건
	{
		SceneManager::LoadScene(L"TestScene");
	}
}

void TitleScene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&Camera::camPos), XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 100.0f);



	for (const auto& obj : mGameObjects) {
		obj->Render(commandList, view, proj);
	}
}

void TitleScene::Release()
{
}
