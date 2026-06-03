#pragma once
#include "stdafx.h"

struct SceneLights
{
    // Directional light
    static XMFLOAT3 dirLightDir;
    static XMFLOAT3 dirLightColor;
    static float    dirLightIntensity;

    // Ambient
    static float    ambientIntensity;

    // Point light (heli front)
    static XMFLOAT3 pointPos;
    static float    pointRange;
    static XMFLOAT3 pointColor;
    static float    pointIntensity;
};
