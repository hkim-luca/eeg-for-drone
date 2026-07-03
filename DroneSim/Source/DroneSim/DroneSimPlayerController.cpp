#include "DroneSimPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "DroneSim.h"
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
#include "Scenario/ScenarioHudWidget.h"
#include "Scenario/ScenarioLog.h"
#include "Scenario/ScenarioMenuWidget.h"
#include "Scenario/ScenarioRunnerComponent.h"
#include "Scenario/ScenarioTime.h"
#include "Telemetry/DroneTelemetryComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

ADroneSimPlayerController::ADroneSimPlayerController()
{
    ScenarioRunner = CreateDefaultSubobject<UScenarioRunnerComponent>(TEXT("ScenarioRunner"));
    EegRunner = CreateDefaultSubobject<UEegRunnerComponent>(TEXT("EegRunner"));
    DroneTelemetry = CreateDefaultSubobject<UDroneTelemetryComponent>(TEXT("DroneTelemetry"));
}

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

        ScenarioRunner->OnScenarioFinished.AddDynamic(this, &ADroneSimPlayerController::HandleScenarioFinished);

        // EEG running mode is the default; scenario collection is the secondary
        // function, selected with the -ScenarioRecording launch option
        if (FParse::Param(FCommandLine::Get(), TEXT("ScenarioRecording")))
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
        ScenarioRunner->OnActionChanged.AddDynamic(HudWidget.Get(), &UScenarioHudWidget::SetActionLabel);
    }

    // file name: recording start time in KST (yyyymmdd_hhmmss); the CSV
    // is created now and appended to as the scenario plays, not written all at once at the end
    const FString RecordingFileName = ScenarioTime::ToFileStampKst(ScenarioTime::NowKst());

    FScenarioLog::Info(FString::Printf(TEXT("Starting scenario: %s"), *LoadedScenario.FilePath));
    ScenarioRunner->Start(LoadedScenario.Steps, RecordingResolution, ScenarioMoveSpeed, RecordingFileName);
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
        EegHudWidget->SetRunner(EegRunner);
        EegHudWidget->SetTelemetry(DroneTelemetry);
    }
    else
    {
        FScenarioLog::Error(TEXT("Could not spawn EEG HUD widget. Set EegHudWidgetClass to a Widget Blueprint "
                                 "on the player controller; running mode continues without graphs."));
    }

    EegRunner->Start(ScenarioMoveSpeed);
}

void ADroneSimPlayerController::HandleScenarioFinished()
{
    const FString SavedPath = ScenarioRunner->GetRecordingPath();

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

auto ADroneSimPlayerController::ShouldUseTouchControls() const -> bool
{
    // are we on a mobile platform? Should we force touch?
    return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
