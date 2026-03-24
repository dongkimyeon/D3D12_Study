#pragma once
#include "Scene.h"
#include "GameObject.h"

class TestScene : public Scene
{
public:
    TestScene();
    virtual ~TestScene();

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;

private:
    std::vector<GameObject*> mGameObjects;

 
};