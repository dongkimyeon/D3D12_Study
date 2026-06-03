#include "SceneLights.h"

XMFLOAT3 SceneLights::dirLightDir       = { 1.f, 2.f, -1.f };
XMFLOAT3 SceneLights::dirLightColor     = { 1.f, 0.92f, 0.75f };
float    SceneLights::dirLightIntensity = 1.0f;

float    SceneLights::ambientIntensity  = 0.12f;

XMFLOAT3 SceneLights::pointPos          = { 0.f, 0.f, 0.f };
float    SceneLights::pointRange        = 40.f;
XMFLOAT3 SceneLights::pointColor        = { 1.f, 0.85f, 0.5f };
float    SceneLights::pointIntensity    = 3.f;
