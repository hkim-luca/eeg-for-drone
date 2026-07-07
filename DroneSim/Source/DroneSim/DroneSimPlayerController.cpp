#include "DroneSimPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DroneSim.h"
#include "DroneSimCharacter.h"
#include "DroneSystemsActor.h"
#include "Eeg/DronePhysicsSettingsWidget.h"
#include "Eeg/EegHudWidget.h"
#include "Eeg/EegRunnerComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Scenario/DronePhysicsConfig.h"
#include "Scenario/ScenarioHudWidget.h"
#include "Scenario/ScenarioLog.h"
#include "Scenario/ScenarioMenuWidget.h"
#include "Scenario/ScenarioRunnerComponent.h"
#include "Scenario/ScenarioTime.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

ADroneSimPlayerController::ADroneSimPlayerController() = default;

void ADroneSimPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // only spawn touch controls on local player controllers
    if (ShouldUseTouchControls() && IsLocalPlayerController())
    {
        // spawn the mobile controls widget
        MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

        if (MobileControlsWidget)
        {
            // add the controls to the player screen
            MobileControlsWidget->AddToPlayerScreen(0);
        }
        else
        {

            UE_LOG(LogDroneSim, Error, TEXT("Could not spawn mobile controls widget."));
        }
    }

    // show the initial scenario menu on local player controllers
    if (IsLocalPlayerController())
    {
        FScenarioLog::LogSessionHeader(GetWorld());
        FScenarioLog::Info(FString::Printf(TEXT("Controller=%s pawn=%s | resolution=%.3fs moveSpeed=%.0f"),
                                           *GetClass()->GetName(),
                                           GetPawn() != nullptr ? *GetPawn()->GetClass()->GetName() : TEXT("NULL"),
                                           RecordingResolution, ScenarioMoveSpeed));

        // the game systems live on a level actor, not this controller; resolve it once
        Systems = ADroneSystemsActor::Get(GetWorld());
        if (Systems == nullptr)
        {
            FScenarioLog::Error(TEXT("No ADroneSystemsActor in this level; EEG/scenario modes are disabled. "
                                     "Place a 'Drone Systems Actor' in the level (Place Actors panel) and save."));
            return;
        }

        Systems->GetScenarioRunner()->OnScenarioFinished.AddDynamic(this,
                                                                    &ADroneSimPlayerController::HandleScenarioFinished);

        // EEG running mode is the default; scenario collection is the secondary
        // function, selected with the -recording launch option
        if (FParse::Param(FCommandLine::Get(), TEXT("recording")))
        {
            ShowMenu();
        }
        else
        {
            StartEegRunningMode();
        }

        // build timestamp on screen to make stale editor binaries obvious
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(
                -1, 5.0f, FColor::Cyan,
                FString::Printf(TEXT("DroneSim Scenario ready (built %hs %hs)"), __DATE__, __TIME__));
        }
    }
}

void ADroneSimPlayerController::ShowMenu()
{
    if (MenuWidget == nullptr)
    {
        MenuWidget = CreateWidget<UScenarioMenuWidget>(this, MenuWidgetClass);

        if (MenuWidget == nullptr)
        {
            FScenarioLog::Error(TEXT("Could not spawn scenario menu widget. Set MenuWidgetClass to a Widget Blueprint "
                                     "on the player controller."));
            return;
        }

        MenuWidget->OnRecordingRequested.AddDynamic(this, &ADroneSimPlayerController::HandleRecordingRequested);
    }

    MenuWidget->AddToViewport(1);
    FScenarioLog::Info(TEXT("Scenario menu shown"));

    // clear the previous run's save result now that the Space bar can start a new one
    if (HudWidget != nullptr)
    {
        HudWidget->HideSaveResult();
    }

    // recording starts via the Space bar only; no mouse input is used on this screen.
    // The cursor is hidden but must still be locked to the viewport: otherwise moving the
    // mouse lets the (invisible) cursor leave the game window, which drops OS input focus
    // and stops keyboard events from reaching this widget entirely.
    SetInputMode(FInputModeUIOnly().SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways));
    SetShowMouseCursor(false);

    // SetWidgetToFocus on the input mode struct can silently fail to grant focus when the
    // mode is set outside of a Slate input event (as here, from BeginPlay); set it directly
    // so the widget's NativeOnKeyDown receives the Space bar shortcut
    MenuWidget->SetKeyboardFocus();
}

void ADroneSimPlayerController::HandleRecordingRequested()
{
    FScenarioLog::Info(TEXT("Recording requested"));

    if (!FScenarioConfig::LoadConfiguredScenario(LoadedScenario))
    {
        FScenarioLog::Error(TEXT("Cannot start recording: no valid scenario steps."));
        MenuWidget->ShowWarning(NSLOCTEXT("ScenarioMenu", "LoadFail", "Scenario load failed. See Output Log."));
        return;
    }

    MenuWidget->RemoveFromParent();
    SetInputMode(FInputModeGameOnly());
    SetShowMouseCursor(false);

    // show the current action at the top center of the screen while playing
    if (CreateActionHud(TEXT("WAIT")))
    {
        Systems->GetScenarioRunner()->OnActionChanged.AddDynamic(HudWidget.Get(), &UScenarioHudWidget::SetActionLabel);
    }

    // file name: recording start time in KST (yyyymmdd_hhmmss); the CSV
    // is created now and appended to as the scenario plays, not written all at once at the end
    const FString RecordingFileName = ScenarioTime::ToFileStampKst(ScenarioTime::NowKst());

    FScenarioLog::Info(FString::Printf(TEXT("Starting scenario: %s"), *LoadedScenario.FilePath));
    Systems->GetScenarioRunner()->Start(LoadedScenario.Steps, RecordingResolution, ScenarioMoveSpeed,
                                        RecordingFileName);
}

auto ADroneSimPlayerController::CreateActionHud(const FString &InitialLabel) -> bool
{
    HudWidget = CreateWidget<UScenarioHudWidget>(this, HudWidgetClass);
    if (HudWidget == nullptr)
    {
        FScenarioLog::Error(TEXT("Could not spawn scenario HUD widget. Set HudWidgetClass to a Widget Blueprint "
                                 "on the player controller; the run continues without the action label."));
        return false;
    }

    HudWidget->AddToViewport(10);
    HudWidget->SetActionLabel(InitialLabel);
    return true;
}

void ADroneSimPlayerController::StartEegRunningMode()
{
    FScenarioLog::Info(TEXT("EEG running mode start (default mode)"));

    SetInputMode(FInputModeGameOnly());
    SetShowMouseCursor(false);

    // EEG overlay: electrode graphs top-right, connection state, flight status bottom-left,
    // flight-trail minimap bottom-right
    EegHudWidget = CreateWidget<UEegHudWidget>(this, EegHudWidgetClass);
    if (EegHudWidget != nullptr)
    {
        EegHudWidget->AddToViewport(11);
        EegHudWidget->SetRunner(Systems->GetEegRunner());
        EegHudWidget->SetTelemetry(Systems->GetTelemetry());
    }
    else
    {
        FScenarioLog::Error(TEXT("Could not spawn EEG HUD widget. Set EegHudWidgetClass to a Widget Blueprint "
                                 "on the player controller; running mode continues without graphs."));
    }

    Systems->GetEegRunner()->Start(ScenarioMoveSpeed);
}

void ADroneSimPlayerController::HandleScenarioFinished()
{
    const FString SavedPath = Systems->GetScenarioRunner()->GetRecordingPath();

    // show the result on the HUD, separate from the action label; on-screen debug messages do
    // not render in Shipping builds
    if (HudWidget != nullptr)
    {
        HudWidget->ShowSaveResult(
            SavedPath.IsEmpty()
                ? FText::FromString(TEXT("SAVE FAILED - check Saved/ScenarioLogs"))
                : FText::FromString(FString::Printf(TEXT("SAVED: %s"), *FPaths::GetCleanFilename(SavedPath))));
    }

    // reload the current level after a short delay so the result stays readable
    FScenarioLog::Info(TEXT("Returning to the initial screen in 3 seconds"));
    GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &ADroneSimPlayerController::ReturnToInitialScreen, 3.0f,
                                    false);
}

void ADroneSimPlayerController::ReturnToInitialScreen()
{
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}

void ADroneSimPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // only add IMCs for local player controllers
    if (IsLocalPlayerController())
    {
        // physics settings panel toggle; a legacy key binding so no IMC asset is needed
        InputComponent->BindKey(EKeys::P, IE_Pressed, this, &ADroneSimPlayerController::TogglePhysicsSettings);

        // black-background option toggle (same legacy binding pattern as the P key)
        InputComponent->BindKey(EKeys::B, IE_Pressed, this, &ADroneSimPlayerController::ToggleEnvironmentBlackout);

        // Add Input Mapping Contexts
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            for (UInputMappingContext *CurrentContext : DefaultMappingContexts)
            {
                Subsystem->AddMappingContext(CurrentContext, 0);
            }

            // only add these IMCs if we're not using mobile touch input
            if (!ShouldUseTouchControls())
            {
                for (UInputMappingContext *CurrentContext : MobileExcludedMappingContexts)
                {
                    Subsystem->AddMappingContext(CurrentContext, 0);
                }
            }
        }
    }
}

void ADroneSimPlayerController::ToggleEnvironmentBlackout()
{
    if (EnvironmentBlackout.IsActive())
    {
        EnvironmentBlackout.Restore();
        FScenarioLog::Info(TEXT("Environment blackout off: scenery restored"));
        return;
    }

    const int32 NumHidden = EnvironmentBlackout.Apply(*GetWorld(), GetPawn());
    FScenarioLog::Info(FString::Printf(TEXT("Environment blackout on: %d actors hidden"), NumHidden));
}

void ADroneSimPlayerController::TogglePhysicsSettings()
{
    if (PhysicsSettingsWidget != nullptr && PhysicsSettingsWidget->IsInViewport())
    {
        ClosePhysicsSettings();
        return;
    }

    if (PhysicsSettingsWidget == nullptr)
    {
        // the widget builds its whole layout in C++, so its class is used directly
        PhysicsSettingsWidget =
            CreateWidget<UDronePhysicsSettingsWidget>(this, UDronePhysicsSettingsWidget::StaticClass());
        if (PhysicsSettingsWidget == nullptr)
        {
            FScenarioLog::Error(TEXT("Could not create the drone physics settings widget"));
            return;
        }
        PhysicsSettingsWidget->OnSettingsSaved.AddDynamic(this, &ADroneSimPlayerController::HandlePhysicsSettingsSaved);
        PhysicsSettingsWidget->OnCloseRequested.AddDynamic(this, &ADroneSimPlayerController::ClosePhysicsSettings);
    }

    PhysicsSettingsWidget->RefreshFromViewModel();
    PhysicsSettingsWidget->AddToViewport(20);

    // UI-only input while editing; the game keeps simulating behind the panel
    SetInputMode(FInputModeUIOnly().SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
    SetShowMouseCursor(true);
    PhysicsSettingsWidget->SetKeyboardFocus();
    FScenarioLog::Info(TEXT("Drone physics settings opened"));
}

void ADroneSimPlayerController::ClosePhysicsSettings()
{
    if (PhysicsSettingsWidget == nullptr || !PhysicsSettingsWidget->IsInViewport())
    {
        return;
    }

    PhysicsSettingsWidget->RemoveFromParent();
    SetInputMode(FInputModeGameOnly());
    SetShowMouseCursor(false);
    FScenarioLog::Info(TEXT("Drone physics settings closed"));
}

void ADroneSimPlayerController::HandlePhysicsSettingsSaved()
{
    if (Systems != nullptr)
    {
        Systems->GetEegRunner()->NotifySettingsSaved();
        Systems->GetScenarioRunner()->NotifySettingsChanged();
    }

    // the runners forward settings only while their physics is active; the yaw/camera
    // coupling must switch on the pawn immediately, flying or not
    if (ADroneSimCharacter *DroneCharacter = Cast<ADroneSimCharacter>(GetPawn()))
    {
        DroneCharacter->ApplyYawControlMode(UDronePhysicsConfig::Get()->Settings.bMouseYawControl);
    }
}

auto ADroneSimPlayerController::ShouldUseTouchControls() const -> bool
{
    // are we on a mobile platform? Should we force touch?
    return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
