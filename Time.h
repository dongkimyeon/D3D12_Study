#pragma once
#include <windows.h>
#include <string>
#include <cstdio>

class Time
{
public:
    static void Initialize();
    static void Update();
    static void SetTimeStop(bool a);
    static float DeltaTime();

private:
    static LARGE_INTEGER CpuFrequency;
    static LARGE_INTEGER PrevFrequency;
    static LARGE_INTEGER CurrentFrequency;
    static float DeltaTimeValue;
    static bool TimeStop;
};