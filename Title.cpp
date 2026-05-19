#include "Title.h"

Title::Title()
{
}

Title::~Title()
{
}

void Title::Initialize(ComPtr<ID3D12Device> device)
{
	GameObject::Initialize(device);

	LoadFromOBJ("TITLE.obj", device);

}

void Title::Update(float dt)
{

	GameObject::Update(dt);
}

void Title::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{

	GameObject::Render(commandList, view, proj);
}
