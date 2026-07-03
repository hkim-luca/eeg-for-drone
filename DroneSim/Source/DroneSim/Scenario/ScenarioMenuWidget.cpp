#include "ScenarioMenuWidget.h"
#include "Components/TextBlock.h"

void UScenarioMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // keyboard focus is required to receive NativeOnKeyDown for the Space bar shortcut
    SetIsFocusable(true);
}

auto UScenarioMenuWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply
{
    if (InKeyEvent.GetKey() == EKeys::SpaceBar)
    {
        OnRecordingRequested.Broadcast();
        return FReply::Handled();
    }

    if (InKeyEvent.GetKey() == EKeys::R)
    {
        OnRunningRequested.Broadcast();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UScenarioMenuWidget::ShowWarning(const FText &Message)
{
    if (WarningText != nullptr)
    {
        WarningText->SetText(Message);
        WarningText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
