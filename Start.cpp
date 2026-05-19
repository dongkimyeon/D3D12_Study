#include "Start.h"

Start::Start()
{
}

Start::~Start()
{
}

void Start::Initialize(ComPtr<ID3D12Device> device)
{

	GameObject::Initialize(device);

	LoadFromOBJ("START.obj", device);
}

void Start::Update(float dt)
{
	GameObject::Update(dt);
}

void Start::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
	GameObject::Render(commandList, view, proj);
}
