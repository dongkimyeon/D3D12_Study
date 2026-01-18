#include "Time.h"


LARGE_INTEGER Time::CpuFrequency = {};
LARGE_INTEGER Time::PrevFrequency = {};
LARGE_INTEGER Time::CurrentFrequency = {};
float Time::DeltaTimeValue = 0.0f;
bool Time::TimeStop;
void Time::Initialize()
{
    QueryPerformanceFrequency(&CpuFrequency);

    QueryPerformanceCounter(&PrevFrequency);
}

void Time::Update()
{

    QueryPerformanceCounter(&CurrentFrequency);

    float differenceFrequency
        = static_cast<float>(CurrentFrequency.QuadPart - PrevFrequency.QuadPart);
    DeltaTimeValue = differenceFrequency / static_cast<float>(CpuFrequency.QuadPart);
    PrevFrequency.QuadPart = CurrentFrequency.QuadPart;


}

void Time::SetTimeStop(bool a)
{
    TimeStop = a;
}
float Time::DeltaTime() { return TimeStop ? 0.0f : DeltaTimeValue; }