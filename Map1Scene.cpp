#include "Map1Scene.h"
#include "Cube.h"
#include "framework.h"
#include "Camera.h"

Map1Scene::Map1Scene() {}
Map1Scene::~Map1Scene() {}

void Map1Scene::Initialize()
{
    // ---- constants (change these to resize the maze) ----
    const int   cols    = 21;   // odd value recommended for DFS maze
    const int   rows    = 21;
    const float spacing = 3.0f;
    // ------------------------------------------------------

    // Build a cols x rows grid, all walls to start
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, 1));

    // DFS maze generation (Recursive Backtracker)
    std::mt19937 rng(std::random_device{}());

    const int dr[] = { -2,  2,  0,  0 };
    const int dc[] = {  0,  0, -2,  2 };

    struct Cell { int r, c; };
    std::vector<Cell> stk;

    grid[1][1] = 0;
    stk.push_back({ 1, 1 });

    while (!stk.empty())
    {
        Cell cur = stk.back();

        // Collect unvisited neighbors (2 steps away, still inside border)
        std::vector<int> dirs;
        for (int d = 0; d < 4; ++d)
        {
            int nr = cur.r + dr[d];
            int nc = cur.c + dc[d];
            if (nr > 0 && nr < rows - 1 && nc > 0 && nc < cols - 1 && grid[nr][nc] == 1)
                dirs.push_back(d);
        }

        if (!dirs.empty())
        {
            int d  = dirs[rng() % dirs.size()];
            int nr = cur.r + dr[d];
            int nc = cur.c + dc[d];
            grid[cur.r + dr[d] / 2][cur.c + dc[d] / 2] = 0; // carve wall between
            grid[nr][nc] = 0;
            stk.push_back({ nr, nc });
        }
        else
        {
            stk.pop_back();
        }
    }

    // Build GameObjects from the generated grid
    float startX = -(cols - 1) * spacing;
    float startZ = -(rows - 1) * spacing;

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            bool isWall = (grid[r][c] == 1);

            // Height varies by wall type for visual depth
            float scaleY;
            if (!isWall)
            {
                scaleY = 0.1f;
            }
            else if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1)
            {
                scaleY = 3.0f;   // outer border: tallest
            }
            else if ((r % 2 == 0) && (c % 2 == 0))
            {
                scaleY = 2.5f;   // inner pillar
            }
            else
            {
                scaleY = 2.0f;   // regular inner wall
            }

            Cube* cube = new Cube();
            cube->Initialize(Framework::GetDevice());
            cube->SetScale(spacing, scaleY, spacing);
            cube->SetPosition(
                startX + c * spacing,
                scaleY * 0.5f,          // bottom sits on y=0
                startZ + r * spacing
            );

            mGameObjects.push_back(cube);
            if (isWall)
                mWallCubes.push_back(cube);
        }
    }

}

void Map1Scene::Update(float dt)
{
    for (const auto& obj : mGameObjects)
        obj->Update(dt);

}

void Map1Scene::Render(ComPtr<ID3D12GraphicsCommandList>& commandList)
{
    XMMATRIX view = XMMatrixLookToLH(
        XMLoadFloat3(&Camera::camPos),
        XMVectorSet(Camera::camForward.x, Camera::camForward.y, Camera::camForward.z, 0),
        XMVectorSet(0, 1, 0, 0)
    );
    XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);

    for (const auto& obj : mGameObjects)
        obj->Render(commandList, view, proj);

    ImGui::Begin("Map1 - Maze");
    ImGui::Text("Camera: (%.2f, %.2f, %.2f)", Camera::camPos.x, Camera::camPos.y, Camera::camPos.z);
    ImGui::Text("Walls: %d / Total: %d", (int)mWallCubes.size(), (int)mGameObjects.size());
    ImGui::Separator();
    ImGui::Text("WASD: move,  LMB: rotate (debug mode)");
    ImGui::End();
}

void Map1Scene::Release()
{
    for (auto obj : mGameObjects)
        delete obj;
    mGameObjects.clear();
    mWallCubes.clear();
}
