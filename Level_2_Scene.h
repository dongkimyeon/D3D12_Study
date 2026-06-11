#pragma once
#include "Scene.h"

class Level_2_Scene : public Scene
{
public:
    Level_2_Scene() = default;
    virtual ~Level_2_Scene() = default;

    virtual void Initialize() override;
    virtual void Update(float dt) override;
    virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList) override;
    virtual void Release() override;
};
