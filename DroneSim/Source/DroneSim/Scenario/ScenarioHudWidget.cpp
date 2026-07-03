#include "ScenarioHudWidget.h"
#include "Components/TextBlock.h"
#include "ScenarioTypes.h"

void UScenarioHudWidget::SetActionLabel(const FString &ActionLabel)
{
    if (ActionText == nullptr)
    {
        return;
    }

    // labels outside the four directional actions (WAIT, CONNECTING, ...) get no arrow
    EScenarioAction Action;
    const FString Prefix = ParseScenarioActionName(ActionLabel, Action) ? ScenarioActionArrow(Action) : FString();
    ActionText->SetText(FText::FromString(Prefix + ActionLabel));
}

void UScenarioHudWidget::HideSaveResult()
{
    if (SaveResultText != nullptr)
    {
        SaveResultText->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UScenarioHudWidget::ShowSaveResult(const FText &Message)
{
    if (SaveResultText != nullptr)
    {
        SaveResultText->SetText(Message);
        SaveResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
