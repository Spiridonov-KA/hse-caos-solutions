#include "check-mt.hpp"

#include "catch2/catch_test_macros.hpp"

#include <atomic>

struct CriticalSectionState {
    struct State {
        uint32_t readers;
        uint32_t writers;
    };

    struct GuardBase {
        CriticalSectionState* state_{nullptr};

        GuardBase() = default;

        GuardBase(CriticalSectionState* state) : state_(state) {
        }

        GuardBase(const GuardBase&) = delete;
        GuardBase& operator=(const GuardBase&) = delete;

        GuardBase(GuardBase&& other) : GuardBase() {
            std::swap(state_, other.state_);
        }

        GuardBase& operator=(GuardBase&& other) {
            std::swap(state_, other.state_);
            return *this;
        }
    };

    State ReaderStart() {
        return ToState(state_.fetch_add(kSingleReader) + kSingleReader);
    }

    State ReaderEnd() {
        return ToState(state_.fetch_sub(kSingleReader) - kSingleReader);
    }

    State WriterStart() {
        return ToState(state_.fetch_add(kSingleWriter) + kSingleWriter);
    }

    State WriterEnd() {
        return ToState(state_.fetch_sub(kSingleWriter) - kSingleWriter);
    }

    struct ReaderGuard : private GuardBase {
        using GuardBase::GuardBase;
        ReaderGuard(ReaderGuard&&) = default;

        ~ReaderGuard() {
            if (state_ != nullptr) {
                state_->ReaderEnd();
            }
        }
    };

    std::tuple<ReaderGuard, State> Reader() {
        return {ReaderGuard(this), ReaderStart()};
    }

    struct WriterGuard : private GuardBase {
        using GuardBase::GuardBase;
        WriterGuard(WriterGuard&&) = default;

        ~WriterGuard() {
            if (state_ != nullptr) {
                state_->WriterEnd();
            }
        }
    };

    [[nodiscard]] std::tuple<WriterGuard, State> Writer() {
        return {WriterGuard{this}, WriterStart()};
    }

  private:
    static constexpr uint64_t kSingleReader = 1;
    static constexpr uint64_t kSingleWriter = uint64_t{1} << 32;

    static void CheckState(State s) {
        REQUIRE_MT(s.writers <= 1);
        if (s.writers > 0) {
            REQUIRE_MT(s.readers == 0);
        }
    }

    static State ToState(uint64_t state) {
        auto s = State{
            .readers = static_cast<uint32_t>(state & (kSingleWriter - 1)),
            .writers = static_cast<uint32_t>(state / kSingleWriter),
        };
        CheckState(s);
        return s;
    }

    std::atomic<uint64_t> state_{0};
};
