#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Helicopter.h"

class Level_1_Scene : public Scene
{
public:
    Level_1_Scene();
    virtual ~Level_1_Scene();

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;

private:
    std::vector<GameObject*> mGameObjects;
	std::unique_ptr<Helicopter> mHelicopter;
};