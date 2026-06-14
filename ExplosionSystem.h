#pragma once
#include "GameObject.h"

class ExplosionSystem : public GameObject
{
public:
    void Initialize(ComPtr<ID3D12Device> device);
    void Spawn(XMFLOAT3 center, int count = 20);
    void Update(float dt);
    void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;

    DirectX::BoundingBox GetWorldAABB() const override { return {}; }

private:
    static constexpr int kMaxParticles = 200;

    struct Particle {
        XMFLOAT3 pos       = {};
        XMFLOAT3 vel       = {};
        XMFLOAT3 rotAxis   = {0.f, 1.f, 0.f};
        float    rotAngle  = 0.f;
        float    rotSpeed  = 0.f;
        float    life      = 0.f;
        float    totalLife = 1.f;
        float    scale     = 0.5f;
        bool     active    = false;
    };

    Particle                 mParticles[kMaxParticles] = {};
    std::vector<XMFLOAT4X4> mInstanceMats;
    std::mt19937             mRng;
};
