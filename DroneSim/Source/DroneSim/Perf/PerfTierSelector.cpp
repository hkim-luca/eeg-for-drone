#include "PerfTierSelector.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/CoreDelegates.h"
#include "RHI.h"
#include "RHIGlobals.h"
#include "RHIStrings.h"
#include "Scalability.h"
#include "Scenario/ScenarioLog.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
/**
 *  ESTIMATE, not measured on the target device: GPU perf indices below this select Low.
 *  42 is the engine's Low/Medium boundary (BaseScalability.ini PerfIndexThresholds); adjust
 *  after the target device's index is read from the log.
 */
constexpr float LowTierGpuIndexBelow = 42.0f;

/** Internal render resolution of the Low tier in percent: the engine's medium step (~970x545 at 1366x768) */
constexpr float LowTierResolutionQuality = 71.0f;

/**
 *  Scalability levels (0 low, 1 medium, 2 high, 3 epic) of the Low tier: every group at its lowest except
 *  textures and anti-aliasing, which stay at medium so the image does not turn to mush.
 *  Tune after on-device measurement.
 */
auto MakeLowTierLevels() -> Scalability::FQualityLevels
{
    Scalability::FQualityLevels Levels;
    Levels.SetFromSingleQualityLevel(0);
    Levels.ResolutionQuality = LowTierResolutionQuality;
    Levels.TextureQuality = 1;
    Levels.AntiAliasingQuality = 1;
    return Levels;
}

/** Low-tier CVars that no scalability group covers (measured on the D3D11/SM5 path) */
struct FLowTierCVar
{
    const TCHAR *Name;
    int32 Value;
};

constexpr FLowTierCVar LowTierCVars[] = {
    {TEXT("r.AntiAliasingMethod"), 1}, // FXAA instead of TSR: anti-aliasing stays on at a fraction of the cost
    {TEXT("r.VolumetricCloud"), 0},    // the cloud pass costs more than the whole base pass
};

/** Small always-on-top window shown while the benchmark blocks the game thread */
class FMeasureNotice
{
  public:
    FMeasureNotice()
    {
        if (!FSlateApplication::IsInitialized())
        {
            return;
        }

        Window = SNew(SWindow)
                     .Title(NSLOCTEXT("PerfTier", "NoticeTitle", "DroneSim"))
                     .ClientSize(FVector2D(560.0, 120.0))
                     .SizingRule(ESizingRule::FixedSize)
                     .AutoCenter(EAutoCenter::PrimaryWorkArea)
                     .SupportsMaximize(false)
                     .SupportsMinimize(false)
                     .IsTopmostWindow(true)[SNew(SBorder).HAlign(HAlign_Center).VAlign(VAlign_Center)
                                                [SNew(STextBlock)
                                                     .Text(NSLOCTEXT("PerfTier", "Measuring",
                                                                     "Measuring graphics performance (first run only)..."))
                                                     .Font(FCoreStyle::GetDefaultFontStyle("Bold", 18))]];
        FSlateApplication::Get().AddWindow(Window.ToSharedRef());
        // draw once now: the benchmark below does not return to the main loop
        FSlateApplication::Get().Tick();
    }

    ~FMeasureNotice()
    {
        if (Window.IsValid())
        {
            Window->RequestDestroyWindow();
        }
    }

  private:
    TSharedPtr<SWindow> Window;
};
} // namespace

FDelegateHandle FPerfTierSelector::PreLoadMapHandle;

void FPerfTierSelector::Register()
{
#if !WITH_EDITOR
    if (IsRunningCommandlet() || IsRunningDedicatedServer() || !FApp::CanEverRender())
    {
        return;
    }
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddStatic(&FPerfTierSelector::OnPreLoadMap);
#endif
}

void FPerfTierSelector::Unregister()
{
    if (PreLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
        PreLoadMapHandle.Reset();
    }
}

void FPerfTierSelector::OnPreLoadMap(const FString & /*MapName*/)
{
    // the first map load is the default map; later loads must not measure again
    Unregister();

    FPerfTierConfig Config = FPerfTierConfig::Load();
    const bool bNeedMeasure = Config.bEnabled && (Config.Tier == EPerfTier::None || Config.bRemeasure);
    if (bNeedMeasure)
    {
        Config.Tier = Measure();
        Config.bRemeasure = false;
        if (Config.Save())
        {
            FScenarioLog::Info(FString::Printf(TEXT("Perf tier stored: %s"), PerfTierName(Config.Tier)));
        }
    }

    if (Config.Tier != EPerfTier::None)
    {
        Apply(Config.Tier);
    }
}

auto FPerfTierSelector::Measure() -> EPerfTier
{
    LogGpuReport();

    // without SM6 the engine runs the D3D11/SM5 path (no Nanite, Lumen or VSM): Low whatever the benchmark says
    if (GMaxRHIFeatureLevel < ERHIFeatureLevel::SM6)
    {
        FScenarioLog::Info(TEXT("SM6 is not available (D3D11/SM5 fallback); starting with the Low tier, no benchmark"));
        return EPerfTier::Low;
    }

    const FMeasureNotice Notice;
    const double StartSeconds = FPlatformTime::Seconds();
    const Scalability::FQualityLevels Result = Scalability::BenchmarkQualityLevels();
    const double ElapsedSeconds = FPlatformTime::Seconds() - StartSeconds;

    const float GpuIndex = Result.GPUBenchmarkResults;
    FScenarioLog::Info(FString::Printf(TEXT("Benchmark: gpuIndex=%.1f cpuIndex=%.1f elapsed=%.2fs"), GpuIndex,
                                       Result.CPUBenchmarkResults, ElapsedSeconds));

    if (!FMath::IsFinite(GpuIndex) || GpuIndex <= 0.0f)
    {
        FScenarioLog::Error(TEXT("Benchmark failed (no valid GPU index); starting with the Low tier"));
        return EPerfTier::Low;
    }
    return GpuIndex < LowTierGpuIndexBelow ? EPerfTier::Low : EPerfTier::High;
}

void FPerfTierSelector::LogGpuReport()
{
    FString FeatureLevel;
    GetFeatureLevelName(GMaxRHIFeatureLevel, FeatureLevel);

    FScenarioLog::Info(FString::Printf(TEXT("GPU: %s (vendor 0x%X device 0x%X) driver %s (%s)"), *GRHIAdapterName,
                                       GRHIVendorId, GRHIDeviceId, *GRHIAdapterUserDriverVersion,
                                       *GRHIAdapterDriverDate));
    FScenarioLog::Info(FString::Printf(TEXT("RHI: %s, max feature level %s"),
                                       GDynamicRHI != nullptr ? GDynamicRHI->GetName() : TEXT("none"), *FeatureLevel));
    FScenarioLog::Info(FString::Printf(TEXT("Atomics: 64-bit atomics=%s, wave ops=%s"),
                                       GRHISupportsAtomicUInt64 ? TEXT("yes") : TEXT("no"),
                                       GRHISupportsWaveOperations ? TEXT("yes") : TEXT("no")));
}

void FPerfTierSelector::Apply(EPerfTier Tier)
{
    if (Tier != EPerfTier::Low)
    {
        return; // High keeps the project defaults
    }

    // not saved to GameUserSettings: the tier file stays the single source on every run
    Scalability::SetQualityLevels(MakeLowTierLevels(), true);
    for (const FLowTierCVar &Entry : LowTierCVars)
    {
        if (IConsoleVariable *CVar = IConsoleManager::Get().FindConsoleVariable(Entry.Name))
        {
            CVar->Set(Entry.Value, ECVF_SetByCode);
        }
        else
        {
            FScenarioLog::Error(FString::Printf(TEXT("Low tier: console variable not found: %s"), Entry.Name));
        }
    }
    FScenarioLog::Info(TEXT("Low tier applied"));
}
