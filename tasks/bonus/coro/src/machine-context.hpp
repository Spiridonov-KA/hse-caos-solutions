#pragma once

#include <runnable.hpp>

struct MachineContext {
    // TODO: your code here.
};

extern "C" void SwitchMachineContext(MachineContext* from,
                                     const MachineContext* to);

// Sets up MachineContext so that on the next switch it runs `body(payload)`
extern "C" void SetupMachineContext(MachineContext* ctx, void* stack_top,
                                    void (*body)(void*), void* payload);

inline void SetupMachineContext(MachineContext* ctx, void* stack_top,
                                IRunnable* runnable) {
    SetupMachineContext(
        ctx, stack_top,
        [](void* raw) {
            ((IRunnable*)raw)->Run();
        },
        runnable);
}
