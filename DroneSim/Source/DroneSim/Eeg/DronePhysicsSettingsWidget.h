#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "DronePhysicsSettingsWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UDronePhysicsSettingsViewModel;
class USpinBox;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhysicsSettingsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPhysicsSettingsCloseRequested);

/**
 *  In-game editor for the drone physics parameters; the view of an MVVM pair with
 *  UDronePhysicsSettingsViewModel. The whole layout is built in C++ through the
 *  widget tree - no Widget Blueprint is needed; create it with CreateWidget using
 *  this class directly. One spin-box row per parameter descriptor, grouped into
 *  airframe / motor / environment / control sections. Every read, write and rule
 *  goes through the ViewModel: this class only builds controls, forwards input to
 *  the commands and repaints on OnStateChanged. SAVE persists via the ViewModel
 *  and surfaces here as OnSettingsSaved; DEFAULTS restores the C++ defaults, and
 *  CLOSE (button, P or Esc) asks the owner to dismiss the panel.
 */
UCLASS()
class UDronePhysicsSettingsWidget : public UUserWidget
{
    GENERATED_BODY()

  public:
    /** Fired when SAVE persisted the config; the active flight physics and the
     *  dashboard pick the new values up here, not on every edit, so slider drags
     *  neither disturb the flight nor flood the server */
    UPROPERTY(BlueprintAssignable, Category = "Drone Physics")
    FOnPhysicsSettingsChanged OnSettingsSaved;

    /** Fired when the panel wants to be closed (CLOSE button, P or Esc key) */
    UPROPERTY(BlueprintAssignable, Category = "Drone Physics")
    FOnPhysicsSettingsCloseRequested OnCloseRequested;

    /** Reloads every control from the ViewModel state */
    void RefreshFromViewModel();

  protected:
    auto Initialize() -> bool override;
    auto NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply override;

  private:
    /** Builds the full-screen hierarchy: header bar, per-group columns, button bar */
    void BuildLayout();

    /** Adds the airframe preset selector to the right side of the header bar */
    void AddPresetRow(class UHorizontalBox &Header);

    /** Adds an amber section caption to the list */
    void AddSectionHeader(UVerticalBox &List, const FString &Title);

    /** Adds one label + spin box row for the ViewModel parameter descriptor RowIndex */
    void AddParameterRow(UVerticalBox &List, int32 RowIndex);

    /** Adds one label + checkbox row and returns the checkbox for binding */
    auto AddToggleRow(UVerticalBox &List, const FString &Label) -> UCheckBox *;

    /** Adds one bottom button and returns it */
    auto AddButton(class UHorizontalBox &Bar, const FString &Caption) -> UButton *;

    /** Forwards every spin box value to the ViewModel (called on any edit) */
    UFUNCTION()
    void HandleValueChanged(float InValue);

    /** Forwards the yaw control mode toggle to the ViewModel */
    UFUNCTION()
    void HandleMouseYawChanged(bool bIsChecked);

    /** Forwards the Left/Right action mapping toggle (strafe vs turn) to the ViewModel */
    UFUNCTION()
    void HandleTurnModeChanged(bool bIsChecked);

    /** Forwards the chosen weight-class preset to the ViewModel */
    UFUNCTION()
    void HandlePresetSelected(FString SelectedItem, ESelectInfo::Type SelectionType);

    /** Light-on-dark text for the preset combo (closed display and dropdown rows);
     *  the combo's default item text is black and unreadable on this panel */
    UFUNCTION()
    UWidget *HandleGeneratePresetItem(FString Item);

    UFUNCTION()
    void HandleSave();

    UFUNCTION()
    void HandleReset();

    UFUNCTION()
    void HandleClose();

    /** State owner and command target of this view */
    UPROPERTY()
    TObjectPtr<UDronePhysicsSettingsViewModel> ViewModel;

    /** Spin boxes in parameter-descriptor order, for bulk read/write */
    UPROPERTY()
    TArray<TObjectPtr<USpinBox>> SpinBoxes;

    /** Weight-class preset selector (MICRO 250 g .. HEAVY LIFT 25 kg) */
    UPROPERTY()
    TObjectPtr<UComboBoxString> PresetCombo;

    /** Yaw control mode checkbox (CONTROL section) */
    UPROPERTY()
    TObjectPtr<UCheckBox> MouseYawCheck;

    /** Left/Right action mapping checkbox: strafe (off) or turn in place (on) */
    UPROPERTY()
    TObjectPtr<UCheckBox> TurnModeCheck;

    /** Guards the edit handlers while RefreshFromViewModel is writing the controls */
    bool bRebuilding = false;
};
