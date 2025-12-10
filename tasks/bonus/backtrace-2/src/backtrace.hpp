#pragma once

#include <span>
#include <string_view>

struct Function {
    std::string_view name;
    void* begin;
    void* end;
};

void SetupHandler(std::span<const Function> functions);
