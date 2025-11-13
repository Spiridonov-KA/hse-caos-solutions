#pragma once

struct IRunnable {
    [[noreturn]] virtual void Run() = 0;
};
