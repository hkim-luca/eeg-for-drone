#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Scenario/ScenarioConfig.h"

#include "DroneSimPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UScenarioHudWidget;
class UScenarioMenuWidget;
class UScenarioRunnerComponent;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings, the scenario menu and scenario playback
 */
UCLASS(abstract)
class ADroneSimPlayerController : public APlayerController
{
    GENERATED_BODY()

  public:
    /** Constructor */
    ADroneSimPlayerController();

  protected:
    /** Input Mapping Contexts */
    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<UInputMappingContext *> DefaultMappingContexts;

    /** Input Mapping Contexts */
    UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
    TArray<UInputMappingContext *> MobileExcludedMappingContexts;

    /** Mobile controls widget to spawn */
    UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
    TSubclassOf<UUserWidget> MobileControlsWidgetClass;

    /** Pointer to the mobile controls widget */
    UPROPERTY()
    TObjectPtr<UUserWidget> MobileControlsWidget;

    /** If true, the player will use UMG touch controls even if not playing on mobile platforms */
    UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
    bool bForceTouchControls = false;

    /** Gameplay initialization */
    void BeginPlay() override;

    /** Input mapping context setup */
    void SetupInputComponent() override;

    /** Returns true if the player should use UMG touch controls */
    auto ShouldUseTouchControls() const -> bool;

  protected:
    /** Position recording resolution (interval between two CSV samples) */
    UPROPERTY(EditAnywhere, Category = "Scenario", meta = (ClampMin = "0.01", ForceUnits = "s"))
    float RecordingResolution = 0.1f;

    /** Max movement speed applied to the pawn during playback; 0 keeps the pawn's own speed */
    UPROPERTY(EditAnywhere, Category = "Scenario", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
    float ScenarioMoveSpeed = 1500.0f;

    /** Widget class for the initial menu screen; point this to a Widget Blueprint to design it in the editor */
    UPROPERTY(EditAnywhere, Category = "Scenario|UI")
    TSubclassOf<UScenarioMenuWidget> MenuWidgetClass;

    /** Widget class for the playback HUD; point this to a Widget Blueprint to design it in the editor */
    UPROPERTY(EditAnywhere, Category = "Scenario|UI")
    TSubclassOf<UScenarioHudWidget> HudWidgetClass;

    /** Pointer to the initial menu screen widget */
    UPROPERTY()
    TObjectPtr<UScenarioMenuWidget> MenuWidget;

    /** Pointer to the playback HUD widget showing the current action */
    UPROPERTY()
    TObjectPtr<UScenarioHudWidget> HudWidget;

    /** Component that plays scenarios and records positions */
    UPROPERTY(VisibleAnywhere, Category = "Scenario")
    TObjectPtr<UScenarioRunnerComponent> ScenarioRunner;

    /** Creates (if needed) and shows the initial menu screen */
    void ShowMenu();

    /** Loads the scenario selected in Scenarios.ini and starts it when the Recording button is clicked */
    UFUNCTION()
    void HandleRecordingRequested();

    /** Saves the CSV and returns to the initial screen when the scenario ends */
    UFUNCTION()
    void HandleScenarioFinished();

    /** Reloads the level to show the initial screen again */
    void ReturnToInitialScreen();

    /** Scenario loaded by the Recording button; kept for the CSV file name */
    FLoadedScenario LoadedScenario;

    /** Timer delaying the level reload so the save result stays readable */
    FTimerHandle ReloadTimerHandle;
};
