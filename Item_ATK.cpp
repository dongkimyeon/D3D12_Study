#include "Item_ATK.h"
#include "Player.h"

ItemMesh Item_ATK::sMesh;

void Item_ATK::LoadSharedMesh(ComPtr<ID3D12Device> device)
{
    Item::LoadMesh(device, "ATK.obj", sMesh);
}

void Item_ATK::UnloadSharedMesh()
{
    Item::UnloadMesh(sMesh);
}

void Item_ATK::Initialize(ComPtr<ID3D12Device> device)
{
    mColor     = { 1.0f, 0.55f, 0.55f, 1.0f };
    mLocalAABB = sMesh.localAABB;
}

void Item_ATK::OnPickup(Player* player)
{
    player->AddATK(5);
    SetInactive();
}
