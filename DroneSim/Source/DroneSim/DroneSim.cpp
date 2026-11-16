#include "DroneSim.h"
#include "Modules/ModuleManager.h"
#include "Perf/PerfTierSelector.h"

/** Primary game module: hooks the first-run performance tier selection */
class FDroneSimModule final : public FDefaultGameModuleImpl
{
  public:
    void StartupModule() override
    {
        FPerfTierSelector::Register();
    }

    void ShutdownModule() override
    {
        FPerfTierSelector::Unregister();
    }
};

IMPLEMENT_PRIMARY_GAME_MODULE( FDroneSimModule, DroneSim, "DroneSim" );

DEFINE_LOG_CATEGORY(LogDroneSim)