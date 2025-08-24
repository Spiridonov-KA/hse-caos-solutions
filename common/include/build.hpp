#pragma once

enum class BuildType {
    Release,
    Debug,
    ASan,
    TSan,
};

constexpr inline BuildType kBuildType =
#if defined(RELEASE)
    BuildType::Release
#elif defined(DEBUG)
    BuildType::Debug
#elif defined(ASAN)
    BuildType::ASan
#elif defined(TSAN)
    BuildType::TSan
#else
#error "Unable to get build type"
#endif
    ;
