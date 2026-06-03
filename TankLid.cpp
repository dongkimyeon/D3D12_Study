#include "TankLid.h"

TankLid::TankLid() {}
TankLid::~TankLid() {}

void TankLid::Initialize(ComPtr<ID3D12Device> device, UINT count)
{
    LoadFromOBJ("Tank_Lid.OBJ", device);
    SetColor(1.f, 0.6f, 0.6f);
    CreateInstanceBuffer(device, count);
}

void TankLid::UpdateInstances(const std::vector<XMFLOAT4X4>& matrices)
{
    if (matrices.empty() || !mInstanceBufferUpload) return;

    mInstanceCount = (UINT)matrices.size();

    void* ptr;
    mInstanceBufferUpload->Map(0, nullptr, &ptr);
    memcpy(ptr, matrices.data(), matrices.size() * sizeof(XMFLOAT4X4));
    mInstanceBufferUpload->Unmap(0, nullptr);
    mInstDirty = true;
}

void TankLid::Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj)
{
    if (mInstanceCount == 0) return;
    GameObject::Render(commandList, view, proj);
}
