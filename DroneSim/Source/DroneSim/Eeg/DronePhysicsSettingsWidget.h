#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "DronePhysicsSettingsWidget.generated.h"

class UButton;
class USpinBox;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhysicsSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhysicsSettingsCloseRequested);

/**
 *  In-game editor for the drone physics parameters (UDronePhysicsConfig). The whole
 *  layout is built in C++ through the widget tree - no Widget Blueprint is needed;
 *  create it with CreateWidget using this class directly. One spin-box row per
 *  parameter, grouped into airframe / motor / environment / control sections; every
 *  change is written to the config immediately and broadcast through
 *  OnSettingsChanged so the active flight physics can pick it up live.
 *  SAVE persists to the Game ini, DEFAULTS restores the C++ defaults, and CLOSE
 *  (button, P or Esc) asks the owner to dismiss the panel.
 */
UCLASS()
class UDronePhysicsSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Fired after any parameter edit was written into UDronePhysicsConfig */
    UPROPERTY(BlueprintAssignable, Category = "Drone Physics")
    FOnPhysicsSettingsChanged OnSettingsChanged;

    /** Fired when the panel wants to be closed (CLOSE button, P or Esc key) */
    UPROPERTY(BlueprintAssignable, Category = "Drone Physics")
    FOnPhysicsSettingsCloseRequested OnCloseRequested;

    /** Reloads every spin box from the current config values */
    void RebuildFromConfig();

  protected:
    auto Initialize() -> bool override;
    auto NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply override;

  private:
    /** Builds the panel hierarchy: title, section headers, parameter rows, buttons */
    void BuildLayout();

    /** Adds an amber section caption to the list */
    void AddSectionHeader(UVerticalBox &List, const FString &Title);

    /** Adds one label + spin box row for the parameter table entry RowIndex */
    void AddParameterRow(UVerticalBox &List, int32 RowIndex);

    /** Adds one bottom button and returns it */
    auto AddButton(class UHorizontalBox &Bar, const FString &Caption) -> UButton *;

    /** Writes every spin box value back into the config (called on any edit) */
    UFUNCTION()
    void HandleValueChanged(float InValue);

    UFUNCTION()
    void HandleSave();

    UFUNCTION()
    void HandleReset();

    UFUNCTION()
    void HandleClose();

    /** Spin boxes in parameter-table order, for bulk read/write */
    UPROPERTY()
    TArray<TObjectPtr<USpinBox>> SpinBoxes;

    /** Guards HandleValueChanged while RebuildFromConfig is writing the spin boxes */
    bool bRebuilding = false;
};
