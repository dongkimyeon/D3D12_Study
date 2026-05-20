#include "Item_HP.h"
#include "Player.h"

ItemMesh Item_HP::sMesh;

void Item_HP::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
    Item::LoadMesh(device, "HP.obj", sMesh);
}

void Item_HP::UnloadSharedMesh()
{
    Item::UnloadMesh(sMesh);
}

void Item_HP::Initialize(ComPtr<ID3D12Device> device)
{
    mColor     = { 0.55f, 1.0f, 0.55f, 1.0f };
    mLocalAABB = sMesh.localAABB;
}

void Item_HP::OnPickup(Player* player)
{
    player->AddHP(10);
    SetInactive();
}
