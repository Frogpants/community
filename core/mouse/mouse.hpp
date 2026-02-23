#pragma once
#include <cstdint>

struct GLFWwindow;

namespace Mouse {

    void Init(GLFWwindow* window);
    void Update();

    double X();
    double Y();

    double DeltaX();
    double DeltaY();

    bool IsDown(int button);
    bool IsPressed(int button);
    bool IsReleased(int button);

    double ScrollX();
    double ScrollY();

    void Lock();
    void Unlock();
    bool IsLocked();
}
