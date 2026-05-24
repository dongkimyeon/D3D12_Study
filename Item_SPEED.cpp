#include "Item_SPEED.h"
#include "Player.h"

ItemMesh Item_SPEED::sMesh;

void Item_SPEED::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
    Item::LoadMesh(device, "SPEED.obj", sMesh);
}

void Item_SPEED::UnloadSharedMesh()
{
    Item::UnloadMesh(sMesh);
}

void Item_SPEED::Initialize(ComPtr<ID3D12Device> device)
{
    mColor     = { 0.55f, 0.75f, 1.0f, 1.0f };
    mLocalAABB = sMesh.localAABB;
}

void Item_SPEED::OnPickup(Player* player)
{
    player->AddMoveSpeed(1.0f);
    SetInactive();
}

void Item_SPEED::RenderBatch(ComPtr<ID3D12GraphicsCommandList>& commandList,
                             const std::vector<Item*>& items)
{
    Item::RenderBatch(commandList, sMesh, items);
}
