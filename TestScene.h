#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Framework.h"

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
    std::default_random_engine rng;

    // 카메라 관련 
    XMFLOAT3 camPos;
    float camYaw;
    float camPitch;
    float moveSpeed;
    float lookSpeed;
    bool isRotating;
    POINT prevMousePos;
};