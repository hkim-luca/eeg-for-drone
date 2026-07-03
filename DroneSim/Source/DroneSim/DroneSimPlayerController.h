#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Scenario/ScenarioConfig.h"

#include "DroneSimPlayerController.generated.h"

class UEegHudWidget;
class UEegRunnerComponent;
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

    /** Widget class for the EEG running-mode overlay (electrode graphs, status line);
     *  point this to a Widget Blueprint to design it in the editor */
    UPROPERTY(EditAnywhere, Category = "Scenario|UI")
    TSubclassOf<UEegHudWidget> EegHudWidgetClass;

    /** Pointer to the initial menu screen widget */
    UPROPERTY()
    TObjectPtr<UScenarioMenuWidget> MenuWidget;

    /** Pointer to the playback HUD widget showing the current action */
    UPROPERTY()
    TObjectPtr<UScenarioHudWidget> HudWidget;

    /** Pointer to the EEG running-mode overlay widget */
    UPROPERTY()
    TObjectPtr<UEegHudWidget> EegHudWidget;

    /** Component that plays scenarios and records positions */
    UPROPERTY(VisibleAnywhere, Category = "Scenario")
    TObjectPtr<UScenarioRunnerComponent> ScenarioRunner;

    /** Component that drives the pawn from EEG server inferences in running mode */
    UPROPERTY(VisibleAnywhere, Category = "Scenario")
    TObjectPtr<UEegRunnerComponent> EegRunner;

    /** Creates (if needed) and shows the initial menu screen */
    void ShowMenu();

    /** Creates the action-label HUD and adds it to the viewport; false if the class is unset */
    auto CreateActionHud(const FString &InitialLabel) -> bool;

    /** Loads Content/Scenarios/Scenario.json and starts it when the Recording button is clicked */
    UFUNCTION()
    void HandleRecordingRequested();

    /** Starts EEG running mode when R is pressed on the initial screen */
    UFUNCTION()
    void HandleRunningRequested();

    /** Stops EEG running mode and returns to the initial screen */
    UFUNCTION()
    void HandleEegStopRequested();

    /** Saves the CSV and returns to the initial screen when the scenario ends */
    UFUNCTION()
    void HandleScenarioFinished();

    /** Reloads the level to show the initial screen again */
    void ReturnToInitialScreen();

    /** Scenario loaded by the Recording button */
    FLoadedScenario LoadedScenario;

    /** Timer delaying the level reload so the save result stays readable */
    FTimerHandle ReloadTimerHandle;
};
