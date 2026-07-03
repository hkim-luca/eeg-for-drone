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

    // per-action probability distribution of the last result, one line per action
    if (ProbabilityText != nullptr)
    {
        const TConstArrayView<float> Probs = Pinned->GetLastActionProbs();
        FString Lines;
        for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
        {
            // zero-padded fixed width (00.0% ... 99.9%) so the lines do not jitter as values change
            Lines += FString::Printf(TEXT("%s %04.1f%%"), *ScenarioActionName(EegConfig::ProbOrder[Index]),
                                     Probs[Index] * 100.0f);
            if (Index + 1 < EegConfig::ProbCount)
            {
                Lines += TEXT("\n");
            }
        }
        ProbabilityText->SetText(FText::FromString(Lines));
    }
}
