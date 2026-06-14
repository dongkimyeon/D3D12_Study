#include "stdafx.h"
#include "ExplosionSystem.h"
#include "framework.h"

void ExplosionSystem::Initialize(ComPtr<ID3D12Device> device)
{
    mRng.seed(std::random_device{}());

    struct FaceDef { XMFLOAT3 normal; float r, g, b; };
    static const FaceDef kFaces[6] = {
        {{ 0.f, 0.f, 1.f}, 1.0f, 0.60f, 0.00f},
        {{ 0.f, 0.f,-1.f}, 0.9f, 0.20f, 0.00f},
        {{ 1.f, 0.f, 0.f}, 1.0f, 0.80f, 0.00f},
        {{-1.f, 0.f, 0.f}, 0.8f, 0.30f, 0.00f},
        {{ 0.f, 1.f, 0.f}, 1.0f, 0.90f, 0.10f},
        {{ 0.f,-1.f, 0.f}, 0.7f, 0.10f, 0.00f},
    };
    static const XMFLOAT3 kFaceVerts[6][4] = {
        {{-0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f}},
        {{ 0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f}},
        {{ 0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f, 0.5f}},
        {{-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},{-0.5f, 0.5f,-0.5f}},
        {{-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f}},
        {{-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f}},
    };

    vertices.reserve(24);
    indices.reserve(36);
    for (int f = 0; f < 6; ++f)
    {
        auto b = (uint32_t)(f * 4);
        for (int v = 0; v < 4; ++v)
        {
            OBJVertex ov = {};
            ov.x  = kFaceVerts[f][v].x; ov.y = kFaceVerts[f][v].y; ov.z = kFaceVerts[f][v].z;
            ov.nx = kFaces[f].normal.x;  ov.ny = kFaces[f].normal.y; ov.nz = kFaces[f].normal.z;
            ov.r  = kFaces[f].r; ov.g = kFaces[f].g; ov.b = kFaces[f].b; ov.a = 1.f;
            vertices.push_back(ov);
        }
        indices.insert(indices.end(), {b, b+1, b+2, b, b+2, b+3});
    }
    mLocalAABB = DirectX::BoundingBox({0.f, 0.f, 0.f}, {0.5f, 0.5f, 0.5f});

    CreateBuffersFromData(device);
    CreateInstanceBuffer(device, kMaxParticles);
    mInstanceMats.reserve(kMaxParticles);
}

void ExplosionSystem::Spawn(XMFLOAT3 center, int count)
{
    std::uniform_real_distribution<float> distDir(-1.f, 1.f);
    std::uniform_real_distribution<float> distSpeed(5.f, 15.f);
    std::uniform_real_distribution<float> distScale(0.5f, 1.5f);
    std::uniform_real_distribution<float> distLife(1.0f, 2.0f);
    std::uniform_real_distribution<float> distRotSpd(2.f, 8.f);

    int spawned = 0;
    for (int i = 0; i < kMaxParticles && spawned < count; ++i)
    {
        if (mParticles[i].active) continue;
        Particle& p = mParticles[i];

        XMVECTOR dir = XMVector3Normalize(XMVectorSet(
            distDir(mRng), fabsf(distDir(mRng)) + 0.3f, distDir(mRng), 0.f));
        float speed = distSpeed(mRng);
        XMFLOAT3 dirF;
        XMStoreFloat3(&dirF, dir);

        p.pos      = center;
        p.vel      = {dirF.x * speed, dirF.y * speed, dirF.z * speed};
        p.rotAngle = 0.f;
        p.rotSpeed = distRotSpd(mRng);
        p.life = p.totalLife = distLife(mRng);
        p.scale  = distScale(mRng);
        p.active = true;

        XMVECTOR axis = XMVector3Normalize(XMVectorSet(distDir(mRng), distDir(mRng), distDir(mRng), 0.f));
        XMStoreFloat3(&p.rotAxis, axis);

        ++spawned;
    }
}

void ExplosionSystem::Update(float dt)
{
    constexpr float kGravity = -12.f;
    mInstanceMats.clear();

    for (auto& p : mParticles)
    {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.f) { p.active = false; continue; }

        p.vel.y    += kGravity * dt;
        p.pos.x    += p.vel.x * dt;
        p.pos.y    += p.vel.y * dt;
        p.pos.z    += p.vel.z * dt;
        p.rotAngle += p.rotSpeed * dt;

        float lifeRatio = p.life / p.totalLife;
        float s = p.scale * lifeRatio;

        XMMATRIX S = XMMatrixScaling(s, s, s);
        XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&p.rotAxis), p.rotAngle);
        XMMATRIX T = XMMatrixTranslation(p.pos.x, p.pos.y, p.pos.z);
        XMFLOAT4X4 mat;
        XMStoreFloat4x4(&mat, S * R * T);
        mInstanceMats.push_back(mat);
    }

    mInstanceCount = (UINT)mInstanceMats.size();
    if (mInstanceCount > 0 && mInstanceBufferUpload)
    {
        void* ptr;
        mInstanceBufferUpload->Map(0, nullptr, &ptr);
        memcpy(ptr, mInstanceMats.data(), mInstanceCount * sizeof(XMFLOAT4X4));
        mInstanceBufferUpload->Unmap(0, nullptr);
        mInstDirty = true;
    }
}

void ExplosionSystem::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (mInstanceCount == 0 || indices.empty()) return;

    UploadBufferIfDirty(commandList, vertexBuffer, vertexBufferUpload,
        mVBState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        vertices.size() * sizeof(OBJVertex), mVBDirty);
    UploadBufferIfDirty(commandList, indexBuffer, indexBufferUpload,
        mIBState, D3D12_RESOURCE_STATE_INDEX_BUFFER,
        indices.size() * sizeof(uint32_t), mIBDirty);
    UploadBufferIfDirty(commandList, mInstanceBuffer, mInstanceBufferUpload,
        mInstState, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        mInstanceCount * sizeof(XMFLOAT4X4), mInstDirty);

    XMFLOAT4X4 vpFloat;
    XMStoreFloat4x4(&vpFloat, XMMatrixTranspose(view * proj));
    commandList->SetGraphicsRoot32BitConstants(0, 16, &vpFloat.m[0][0], 0);

    commandList->IASetVertexBuffers(0, 1, &vbView);
    commandList->IASetVertexBuffers(1, 1, &mInstanceBufView);
    commandList->IASetIndexBuffer(&ibView);
    commandList->DrawIndexedInstanced((UINT)indices.size(), mInstanceCount, 0, 0, 0);
}
