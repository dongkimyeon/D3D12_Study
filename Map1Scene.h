#pragma once
#include "Scene.h"
#include "GameObject.h"



class Map1Scene : public Scene
{
public:
	Map1Scene();
	virtual ~Map1Scene();

	virtual void Initialize() override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
	virtual void Release() override;

private:
	std::vector<GameObject*> mGameObjects;

};