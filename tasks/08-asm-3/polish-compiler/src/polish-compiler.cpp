#include <polish-compiler.hpp>

#include <unused.hpp>  // TODO: remove before flight.

struct PolishCompiler::Impl {
    void* Compile(std::span<const PolishOp> program, size_t nargs) {
        UNUSED(program, nargs);  // TODO: remove before flight.
        return nullptr;  // TODO: remove before flight.
        // TODO: your code here.
    }

    // TODO: your code here.
};

PolishCompiler::PolishCompiler() : impl_(std::make_unique<Impl>()) {
}

PolishCompiler::~PolishCompiler() = default;

void* PolishCompiler::Compile(std::span<const PolishOp> program, size_t args) {
    return impl_->Compile(program, args);
}
