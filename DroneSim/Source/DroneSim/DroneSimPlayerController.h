#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Scenario/ScenarioConfig.h"

#include "DroneSimPlayerController.generated.h"

class ADroneSystemsActor;
class UDronePhysicsSettingsWidget;
class UEegHudWidget;
class UInputMappingContext;
class UUserWidget;
class UScenarioHudWidget;
class UScenarioMenuWidget;

/**
 *  Basic PlayerController class for a third person game.
 *  Owns the per-player concerns: input mappings, the scenario menu and HUD widgets,
 *  the physics settings panel, and the launch-time mode selection. The game systems
 *  themselves (EEG link + pawn driver, scenario playback, telemetry) live on the
 *  level's ADroneSystemsActor; this controller resolves that actor and drives it.
 *  The mode is selected at launch: by default EEG running mode starts immediately;
 *  the -recording command line option shows the scenario collection menu instead.
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

    /** Drone physics settings panel, toggled with the P key; built entirely in C++ */
    UPROPERTY()
    TObjectPtr<UDronePhysicsSettingsWidget> PhysicsSettingsWidget;

    /** Level actor hosting the drone session systems (EEG runner, scenario runner,
     *  telemetry); resolved in BeginPlay via ADroneSystemsActor::Get */
    UPROPERTY()
    TObjectPtr<ADroneSystemsActor> Systems;

    /** Creates (if needed) and shows the initial menu screen */
    void ShowMenu();

    /** Creates the action-label HUD and adds it to the viewport; false if the class is unset */
    auto CreateActionHud(const FString &InitialLabel) -> bool;

    /** Loads Content/Scenarios/Scenario.json and starts it when the Recording button is clicked */
    UFUNCTION()
    void HandleRecordingRequested();

    /** Starts EEG running mode; entered directly from BeginPlay by default
     *  (no menu is shown unless the game was launched with -recording).
     *  The mode runs until the game exits. */
    void StartEegRunningMode();

    /** Shows or hides the drone physics settings panel (P key) */
    void TogglePhysicsSettings();

    /** Hides the settings panel and returns input to the game */
    UFUNCTION()
    void ClosePhysicsSettings();

    /** SAVE button: applies the saved parameters to whichever runner is flying
     *  right now and publishes them to the EEG dashboard */
    UFUNCTION()
    void HandlePhysicsSettingsSaved();

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
