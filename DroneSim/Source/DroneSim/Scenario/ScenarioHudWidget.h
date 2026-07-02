#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "ScenarioHudWidget.generated.h"

class UTextBlock;

/**
 *  In-game HUD shown during scenario playback:
 *  displays the currently applied action at the top center of the screen, and
 *  separately the recording save result once playback ends.
 *  SaveResultText starts hidden per its default visibility in the Widget Blueprint;
 *  HideSaveResult() resets it, called by the player controller when the recording
 *  menu becomes active again, and ShowSaveResult() reveals the next result.
 *  The layout is designed in a Widget Blueprint that uses this class as its parent;
 *  the designed text blocks must be named ActionText and SaveResultText to bind.
 */
UCLASS(Abstract)
class UScenarioHudWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Updates the action label; signature matches FOnScenarioActionChanged for direct binding */
    UFUNCTION(BlueprintCallable, Category = "Scenario")
    void SetActionLabel(const FString &ActionLabel);

    /** Hides the save result line; call when the recording menu becomes active again */
    UFUNCTION(BlueprintCallable, Category = "Scenario")
    void HideSaveResult();

    /** Reveals and shows the recording save result (success or failure), independent of the action label */
    UFUNCTION(BlueprintCallable, Category = "Scenario")
    void ShowSaveResult(const FText &Message);

  private:
    /** Text block showing the current action; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ActionText;

    /** Text block showing the recording save result; designed in the Widget Blueprint */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> SaveResultText;
};
