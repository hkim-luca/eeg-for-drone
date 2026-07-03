#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "ScenarioMenuWidget.generated.h"

class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRecordingRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunningRequested);

/**
 *  Initial screen widget shown before scenario playback; the CSV file name is
 *  generated from the save timestamp, so no file name input is shown.
 *  The two modes start from the keyboard only (no mouse input): Space starts
 *  scenario recording, R starts EEG running mode; the widget takes keyboard
 *  focus while shown so it can receive the key presses.
 *  The layout is designed in a Widget Blueprint that uses this class as its parent;
 *  the designed text block must be named WarningText to bind.
 */
UCLASS(Abstract)
class UScenarioMenuWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Fired when the Space bar is pressed to start recording */
    UPROPERTY(BlueprintAssignable, Category = "Scenario")
    FOnRecordingRequested OnRecordingRequested;

    /** Fired when R is pressed to start EEG running mode */
    UPROPERTY(BlueprintAssignable, Category = "Scenario")
    FOnRunningRequested OnRunningRequested;

    /** Shows a warning line (e.g. scenario load errors) */
    UFUNCTION(BlueprintCallable, Category = "Scenario")
    void ShowWarning(const FText &Message);

  protected:
    void NativeOnInitialized() override;
    auto NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply override;

  private:
    /** Warning line shown on scenario load errors; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> WarningText;
};
