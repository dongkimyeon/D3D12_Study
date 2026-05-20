#pragma once
#include "Item.h"

class Item_HP : public Item
{
public:
    virtual void Initialize(ComPtr<ID3D12Device> device) override;
    virtual void OnPickup(Player* player) override;

    static void LoadSharedMesh(ComPtr<ID3D12Device> device);
    static void UnloadSharedMesh();

protected:
    virtual ItemMesh& GetMesh() override { return sMesh; }

private:
    static ItemMesh sMesh;
};
