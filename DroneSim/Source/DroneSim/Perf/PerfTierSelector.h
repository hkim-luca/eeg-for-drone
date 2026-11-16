#pragma once

#include "CoreMinimal.h"
#include "PerfTierConfig.h"

/**
 *  First-run graphics tier selection. Runs once, right before the default map loads:
 *  when the config asks for a measurement (first run or re-measure request) it logs the
 *  GPU/RHI/atomics report and stores the tier in config/perf_tier.json: Low when SM6 is not
 *  available (D3D11/SM5 fallback), otherwise the result of the engine's built-in benchmark.
 *  Every run then applies the stored tier. A failed measurement selects Low.
 *  Game builds only; the editor is never touched.
 */
class FPerfTierSelector
{
public:
    /** Hooks the pre-map-load delegate; call from module startup */
    static void Register();
    static void Unregister();

    /** Applies the tier's scalability settings for this run (High keeps the project defaults) */
    static void Apply(EPerfTier Tier);

private:
    static void OnPreLoadMap(const FString &MapName);
    static auto Measure() -> EPerfTier;
    static void LogGpuReport();

    static FDelegateHandle PreLoadMapHandle;
};
