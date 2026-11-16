#pragma once

#include "CoreMinimal.h"

/** Graphics tier chosen by the first-run measurement */
enum class EPerfTier : uint8
{
    None, // not decided yet
    High,
    Low
};

auto PerfTierName(EPerfTier Tier) -> const TCHAR *;

/** config/perf_tier.json: measurement switch, re-measure request and the stored tier */
struct FPerfTierConfig
{
    /** false: never measure; a stored tier is still applied */
    bool bEnabled = true;

    /** true: measure again on the next run; reset to false after that run */
    bool bRemeasure = false;

    EPerfTier Tier = EPerfTier::None;

    static auto FilePath() -> FString;

    /** A missing or invalid file yields the defaults (measure on first run) */
    static auto Load() -> FPerfTierConfig;

    auto Save() const -> bool;
};
