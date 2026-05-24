#pragma once
#include "GameObject.h"
#include <DirectXCollision.h>

class Enemy : public GameObject
{
public:
	Enemy();
	virtual ~Enemy();

	virtual void Initialize(ComPtr<ID3D12Device> device) override;
	virtual void Update(float dt) override;
	virtual void Render(ComPtr<ID3D12GraphicsCommandList>& commandList, XMMATRIX view, XMMATRIX proj) override;
	virtual DirectX::BoundingBox GetWorldAABB() const override;

	// 씬 Initialize/Release에서 명시적으로 호출
	static void LoadSharedMesh(ComPtr<ID3D12Device> device);
	static void UnloadSharedMesh();

	// 인스턴싱: 씬 초기화 시 최대 용량 할당, 매 프레임 RenderBatch 호출
	static void BuildInstanceBuffer(ComPtr<ID3D12Device> device, UINT maxCount);
	static void RenderBatch(ComPtr<ID3D12GraphicsCommandList>& commandList,
	                        const std::vector<Enemy*>& enemies);

	void SetTarget(const XMFLOAT3* pos)                              { mTarget    = pos; }
	void SetColliders(const std::vector<DirectX::BoundingBox>* c)   { mColliders = c;   }
	void SetEnemyList(const std::vector<Enemy*>* list)              { mEnemies   = list; }
	void SetFlowField(const int* field, int gridSize, float spacing, float offset);
	void SetDetectRange(float r) { mDetectRange = r; }

	void TakeDamage(int dmg) { mHp -= dmg; if (mHp <= 0) { mHp = 0; mAlive = false; } }
	bool IsAlive()    const  { return mAlive; }
	int  GetHp()      const  { return mHp; }

private:
	DirectX::BoundingBox ComputePhysicsAABB() const;
	void ResolveWallCollision();
	void SeparateFromEnemies();
	XMFLOAT3 GetNextWaypoint() const;

	const XMFLOAT3* mTarget    = nullptr;
	const std::vector<DirectX::BoundingBox>* mColliders = nullptr;
	const std::vector<Enemy*>*               mEnemies   = nullptr;

	// Flow field 경로 추적
	const int* mFlowField = nullptr;
	int   mGridSize  = 0;
	float mSpacing   = 0.0f;
	float mOffset    = 0.0f;

	float mMoveSpeed   = 3.0f;
	float mDetectRange = 10.0f;  // 이 거리 안에 들어오면 추적 시작
	bool  mIsChasing   = false;  // 한 번 감지되면 true로 유지

	DirectX::BoundingBox mLocalAABB;

	static ComPtr<ID3D12Resource>   sVB;
	static ComPtr<ID3D12Resource>   sIB;
	static ComPtr<ID3D12Resource>   sVBUpload;
	static ComPtr<ID3D12Resource>   sIBUpload;
	static D3D12_VERTEX_BUFFER_VIEW sVbView;
	static D3D12_INDEX_BUFFER_VIEW  sIbView;
	static UINT                     sIndexCount;
	static D3D12_RESOURCE_STATES    sVBState;
	static D3D12_RESOURCE_STATES    sIBState;
	static bool                     sVBDirty;
	static bool                     sIBDirty;
	static DirectX::BoundingBox     sLocalAABB;

	static ComPtr<ID3D12Resource>   sInstanceBuffer;
	static D3D12_VERTEX_BUFFER_VIEW sInstanceView;
	static UINT                     sMaxInstances;

	int  mHp    = 100;
	bool mAlive = true;
};
