#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include <unused.hpp>  // TODO: remove before flight.

template <class F>
std::variant<std::pair<std::string, std::string>, int>
CaptureOutput(F&& f, std::string_view input) {
    UNUSED(f, input);  // TODO: remove before flight.
    // TODO: your code here.
    throw "TODO";
}
