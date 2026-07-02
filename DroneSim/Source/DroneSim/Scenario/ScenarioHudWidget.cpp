#include "ScenarioHudWidget.h"
#include "Components/TextBlock.h"

void UScenarioHudWidget::SetActionLabel(const FString &ActionLabel)
{
    if (ActionText != nullptr)
    {
        ActionText->SetText(FText::FromString(ActionLabel));
    }
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
