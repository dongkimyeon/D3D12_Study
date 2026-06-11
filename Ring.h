#pragma once
#include "GameObject.h"

class Ring : public GameObject
{
public:
    void Initialize(ComPtr<ID3D12Device> device);
    bool CheckCollision(const XMFLOAT3& heliPos) const;

    static constexpr float kTriggerRadius = 28.f;
};
