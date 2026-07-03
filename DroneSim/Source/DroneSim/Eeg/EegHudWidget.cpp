#include "EegHudWidget.h"
#include "Components/TextBlock.h"
#include "EegGraphPanel.h"
#include "EegRunnerComponent.h"

void UEegHudWidget::SetRunner(UEegRunnerComponent *InRunner)
{
    Runner = InRunner;
    if (GraphPanel != nullptr)
    {
        GraphPanel->SetRunner(InRunner);
    }
}

void UEegHudWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const UEegRunnerComponent *Pinned = Runner.Get();
    if (Pinned == nullptr)
    {
        return;
    }

    // link state, colored so a broken connection is obvious; FEegClient reconnects on its own
    if (ConnectionText != nullptr)
    {
        const bool bConnected = Pinned->IsServerConnected();
        ConnectionText->SetText(bConnected ? NSLOCTEXT("EegHud", "Connected", "EEG SERVER: CONNECTED")
                                           : NSLOCTEXT("EegHud", "Reconnecting", "EEG SERVER: RECONNECTING..."));
        ConnectionText->SetColorAndOpacity(bConnected ? FSlateColor(FLinearColor(0.2f, 1.0f, 0.4f))
                                                      : FSlateColor(FLinearColor(1.0f, 0.3f, 0.2f)));
    }

    // rolling classification accuracy reported by the server with each action result
    if (AccuracyText != nullptr)
    {
        const float Accuracy = Pinned->GetLastAccuracyPercent();
        AccuracyText->SetText(Accuracy < 0.0f ? NSLOCTEXT("EegHud", "AccuracyPending", "ACCURACY: --")
                                              : FText::FromString(FString::Printf(TEXT("ACCURACY: %.1f%%"), Accuracy)));
    }

    // ground truth comes from the simulated device; comparing it with the big inferred
    // action label shows misclassifications directly on screen
    if (StatusText != nullptr)
    {
        StatusText->SetText(FText::FromString(
            FString::Printf(TEXT("TRUE: %s"), *ScenarioActionName(Pinned->GetSimulator().GetTrueAction()))));
    }
}
