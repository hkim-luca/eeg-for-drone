#include "DroneSystemsActor.h"
#include "Eeg/EegRunnerComponent.h"
#include "EngineUtils.h"
#include "Scenario/ScenarioRunnerComponent.h"
#include "Telemetry/DroneTelemetryComponent.h"

ADroneSystemsActor::ADroneSystemsActor()
{
    EegRunner = CreateDefaultSubobject<UEegRunnerComponent>(TEXT("EegRunner"));
    ScenarioRunner = CreateDefaultSubobject<UScenarioRunnerComponent>(TEXT("ScenarioRunner"));
    DroneTelemetry = CreateDefaultSubobject<UDroneTelemetryComponent>(TEXT("DroneTelemetry"));
}

auto ADroneSystemsActor::Get(UWorld *World) -> ADroneSystemsActor *
{
    if (World == nullptr)
    {
        return nullptr;
    }

    // the actor must be placed in the level in the editor; returns the placed instance
    for (TActorIterator<ADroneSystemsActor> It(World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

auto ADroneSystemsActor::GetEegRunner() const -> UEegRunnerComponent *
{
    return EegRunner;
}

auto ADroneSystemsActor::GetScenarioRunner() const -> UScenarioRunnerComponent *
{
    return ScenarioRunner;
}

auto ADroneSystemsActor::GetTelemetry() const -> UDroneTelemetryComponent *
{
    return DroneTelemetry;
}
