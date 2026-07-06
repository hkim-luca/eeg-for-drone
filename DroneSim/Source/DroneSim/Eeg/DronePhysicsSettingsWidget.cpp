#include "DronePhysicsSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Scenario/DronePhysicsConfig.h"
#include "Scenario/DronePhysicsPresets.h"
#include "Styling/CoreStyle.h"

namespace
{
/** One editable parameter: exactly one of the three field pointers is set */
struct FParameterRow
{
    const TCHAR *Section;
    const TCHAR *Label;
    double FDronePhysicsSettings::*DoubleField;
    float FDronePhysicsSettings::*FloatField;
    int32 FDronePhysicsSettings::*IntField;
    double Min;
    double Max;
    int32 FractionalDigits;
};

// clang-format off
const FParameterRow ParameterRows[] = {
    {TEXT("AIRFRAME"), TEXT("MASS [kg]"),              &FDronePhysicsSettings::MassKg,              nullptr, nullptr, 0.1, 30.0, 3},
    {nullptr,          TEXT("ARM LENGTH [m]"),         &FDronePhysicsSettings::ArmLengthM,          nullptr, nullptr, 0.05, 1.0, 3},
    {nullptr,          TEXT("INERTIA XX [kg m2]"),     &FDronePhysicsSettings::InertiaXX,           nullptr, nullptr, 0.0001, 10.0, 5},
    {nullptr,          TEXT("INERTIA YY [kg m2]"),     &FDronePhysicsSettings::InertiaYY,           nullptr, nullptr, 0.0001, 10.0, 5},
    {nullptr,          TEXT("INERTIA ZZ [kg m2]"),     &FDronePhysicsSettings::InertiaZZ,           nullptr, nullptr, 0.0001, 10.0, 5},
    {TEXT("MOTOR"),    TEXT("TIME CONST [s]"),         &FDronePhysicsSettings::MotorTimeConstantS,  nullptr, nullptr, 0.005, 0.5, 3},
    {nullptr,          TEXT("THRUST COEF kT"),         &FDronePhysicsSettings::ThrustCoefficient,   nullptr, nullptr, 1e-7, 1e-3, 9},
    {nullptr,          TEXT("TORQUE COEF kQ"),         &FDronePhysicsSettings::TorqueCoefficient,   nullptr, nullptr, 1e-9, 1e-4, 10},
    {nullptr,          TEXT("MAX SPEED [rad/s]"),      &FDronePhysicsSettings::MotorMaxRadS,        nullptr, nullptr, 100.0, 10000.0, 0},
    {nullptr,          TEXT("ROTOR INERTIA [kg m2]"),  &FDronePhysicsSettings::RotorInertiaKgM2,    nullptr, nullptr, 0.0, 0.01, 7},
    {nullptr,          TEXT("ROTOR RADIUS [m]"),       &FDronePhysicsSettings::RotorRadiusM,        nullptr, nullptr, 0.01, 1.0, 3},
    {TEXT("ENVIRONMENT"), TEXT("GRAVITY [m/s2]"),      &FDronePhysicsSettings::GravityMS2,          nullptr, nullptr, 0.0, 30.0, 3},
    {nullptr,          TEXT("AIR DENSITY [kg/m3]"),    &FDronePhysicsSettings::AirDensity,          nullptr, nullptr, 0.0, 2.0, 3},
    {nullptr,          TEXT("DRAG LINEAR"),            &FDronePhysicsSettings::DragLinear,          nullptr, nullptr, 0.0, 10.0, 3},
    {nullptr,          TEXT("DRAG QUADRATIC"),         &FDronePhysicsSettings::DragQuadratic,       nullptr, nullptr, 0.0, 10.0, 3},
    {nullptr,          TEXT("GROUND EFFECT"),          &FDronePhysicsSettings::GroundEffectStrength, nullptr, nullptr, 0.0, 1.0, 2},
    {nullptr,          TEXT("WIND X [m/s]"),           &FDronePhysicsSettings::WindXMS,             nullptr, nullptr, -30.0, 30.0, 1},
    {nullptr,          TEXT("WIND Y [m/s]"),           &FDronePhysicsSettings::WindYMS,             nullptr, nullptr, -30.0, 30.0, 1},
    {nullptr,          TEXT("GUST [m/s]"),             &FDronePhysicsSettings::GustIntensityMS,     nullptr, nullptr, 0.0, 15.0, 1},
    {TEXT("CONTROL"),  TEXT("MAX SPEED [m/s]"),        &FDronePhysicsSettings::MaxSpeedMS,          nullptr, nullptr, 0.5, 40.0, 1},
    {nullptr,          TEXT("MAX CLIMB [m/s]"),        &FDronePhysicsSettings::MaxClimbRateMS,      nullptr, nullptr, 0.1, 10.0, 1},
    {nullptr,          TEXT("MAX TILT [deg]"),         &FDronePhysicsSettings::MaxTiltDeg,          nullptr, nullptr, 1.0, 45.0, 1},
    {nullptr,          TEXT("TAKEOFF ALT [m]"),        &FDronePhysicsSettings::TakeoffAltitudeM,    nullptr, nullptr, 0.0, 50.0, 1},
    {nullptr,          TEXT("VEL P GAIN"),             &FDronePhysicsSettings::VelPGain,            nullptr, nullptr, 0.1, 20.0, 2},
    {nullptr,          TEXT("VEL I GAIN"),             &FDronePhysicsSettings::VelIGain,            nullptr, nullptr, 0.0, 10.0, 2},
    {nullptr,          TEXT("ATT P GAIN"),             &FDronePhysicsSettings::AttPGain,            nullptr, nullptr, 0.5, 50.0, 1},
    {nullptr,          TEXT("RATE P GAIN"),            &FDronePhysicsSettings::RatePGain,           nullptr, nullptr, 1.0, 200.0, 1},
    {nullptr,          TEXT("RATE D GAIN"),            &FDronePhysicsSettings::RateDGain,           nullptr, nullptr, 0.0, 5.0, 2},
    {nullptr,          TEXT("SUBSTEP [Hz]"),           nullptr, nullptr, &FDronePhysicsSettings::SubstepHz, 250.0, 4000.0, 0},
    {TEXT("SETTLE"),   TEXT("SPEED THRESH [cm/s]"),    nullptr, &FDronePhysicsSettings::SettleSpeedThreshold, nullptr, 0.1, 100.0, 1},
    {nullptr,          TEXT("TILT THRESH [deg]"),      nullptr, &FDronePhysicsSettings::SettleTiltThreshold, nullptr, 0.01, 5.0, 2},
};
// clang-format on

constexpr int32 ParameterRowCount = UE_ARRAY_COUNT(ParameterRows);

/** OSD palette shared with the telemetry widgets */
const FLinearColor PanelBackground(0.01f, 0.012f, 0.01f, 0.88f);
const FLinearColor LabelColor(0.5f, 0.52f, 0.5f, 1.0f);
const FLinearColor SectionColor(0.75f, 0.35f, 0.02f, 1.0f);
const FLinearColor TitleColor(0.03f, 0.45f, 0.14f, 1.0f);

auto RowValue(const FDronePhysicsSettings &Settings, const FParameterRow &Row) -> double
{
    if (Row.DoubleField != nullptr)
    {
        return Settings.*Row.DoubleField;
    }
    if (Row.FloatField != nullptr)
    {
        return Settings.*Row.FloatField;
    }
    return Settings.*Row.IntField;
}

void SetRowValue(FDronePhysicsSettings &Settings, const FParameterRow &Row, double Value)
{
    if (Row.DoubleField != nullptr)
    {
        Settings.*Row.DoubleField = Value;
    }
    else if (Row.FloatField != nullptr)
    {
        Settings.*Row.FloatField = static_cast<float>(Value);
    }
    else
    {
        Settings.*Row.IntField = FMath::RoundToInt32(Value);
    }
}
} // namespace

auto UDronePhysicsSettingsWidget::Initialize() -> bool
{
    const bool bOk = Super::Initialize();
    // the panel handles P/Esc itself, so it must be able to take keyboard focus
    SetIsFocusable(true);
    if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        BuildLayout();
        RebuildFromConfig();
    }
    return bOk;
}

void UDronePhysicsSettingsWidget::BuildLayout()
{
    UCanvasPanel *Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
    WidgetTree->RootWidget = Canvas;

    // full-screen editor: the panel stretches to every viewport edge and the sections
    // spread out into side-by-side columns instead of one scrolling list
    UBorder *Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(PanelBackground);
    Panel->SetPadding(FMargin(48.0f, 32.0f));
    UCanvasPanelSlot *PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    PanelSlot->SetOffsets(FMargin(0.0f));

    UVerticalBox *Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->SetContent(Root);

    // header bar: title on the left, airframe preset selector on the right
    UHorizontalBox *Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Root->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));

    UTextBlock *Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Title->SetText(FText::FromString(TEXT("DRONE PHYSICS")));
    Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 22));
    Title->SetColorAndOpacity(FSlateColor(TitleColor));
    UHorizontalBoxSlot *TitleSlot = Header->AddChildToHorizontalBox(Title);
    TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    TitleSlot->SetVerticalAlignment(VAlign_Center);

    AddPresetRow(*Header);

    // body: a 2x3 grid of parameter groups so the height is used as generously as
    // the width - AIRFRAME/MOTOR/ENVIRONMENT across the top, CONTROL/SETTLE below
    UHorizontalBox *GridRows[2];
    for (UHorizontalBox *&GridRow : GridRows)
    {
        GridRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
        UVerticalBoxSlot *RowSlot = Root->AddChildToVerticalBox(GridRow);
        RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
    }

    const auto AddColumn = [this](UHorizontalBox &GridRow) -> UVerticalBox * {
        UVerticalBox *NewColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        UHorizontalBoxSlot *ColumnSlot = GridRow.AddChildToHorizontalBox(NewColumn);
        ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 40.0f, 0.0f));
        return NewColumn;
    };

    SpinBoxes.Reset();
    UVerticalBox *CurrentColumn = nullptr;
    int32 BottomRowColumns = 0;
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount; ++RowIndex)
    {
        const FParameterRow &Row = ParameterRows[RowIndex];
        if (Row.Section != nullptr)
        {
            const bool bBottomRow = FCString::Strcmp(Row.Section, TEXT("CONTROL")) == 0 ||
                                    FCString::Strcmp(Row.Section, TEXT("SETTLE")) == 0;
            CurrentColumn = AddColumn(*GridRows[bBottomRow ? 1 : 0]);
            BottomRowColumns += bBottomRow ? 1 : 0;
            AddSectionHeader(*CurrentColumn, Row.Section);
        }
        AddParameterRow(*CurrentColumn, RowIndex);
    }

    // pad the bottom row with empty cells so its columns line up with the top row's
    for (int32 Filler = BottomRowColumns; Filler < 3; ++Filler)
    {
        AddColumn(*GridRows[1]);
    }

    // footer: actions gathered bottom-right
    UHorizontalBox *ButtonBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Root->AddChildToVerticalBox(ButtonBar)->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));

    USpacer *ButtonSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
    UHorizontalBoxSlot *SpacerSlot = ButtonBar->AddChildToHorizontalBox(ButtonSpacer);
    SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    AddButton(*ButtonBar, TEXT("SAVE"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleSave);
    AddButton(*ButtonBar, TEXT("DEFAULTS"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleReset);
    AddButton(*ButtonBar, TEXT("CLOSE"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleClose);
}

void UDronePhysicsSettingsWidget::AddPresetRow(UHorizontalBox &Header)
{
    UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(TEXT("PRESET")));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 13));
    Label->SetColorAndOpacity(FSlateColor(SectionColor));
    UHorizontalBoxSlot *LabelSlot = Header.AddChildToHorizontalBox(Label);
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

    PresetCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
    Presets = DronePhysicsPresets::LoadPresets();
    for (const FDroneAirframePreset &Preset : Presets)
    {
        PresetCombo->AddOption(Preset.Name);
    }
    PresetCombo->OnSelectionChanged.AddDynamic(this, &UDronePhysicsSettingsWidget::HandlePresetSelected);
    PresetCombo->OnGenerateWidgetEvent.BindDynamic(this, &UDronePhysicsSettingsWidget::HandleGeneratePresetItem);

    USizeBox *ComboSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ComboSize->SetWidthOverride(280.0f);
    ComboSize->SetContent(PresetCombo);
    UHorizontalBoxSlot *ComboSlot = Header.AddChildToHorizontalBox(ComboSize);
    ComboSlot->SetVerticalAlignment(VAlign_Center);
}

void UDronePhysicsSettingsWidget::AddSectionHeader(UVerticalBox &List, const FString &SectionTitle)
{
    UTextBlock *Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Header->SetText(FText::FromString(SectionTitle));
    Header->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
    Header->SetColorAndOpacity(FSlateColor(SectionColor));
    List.AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 6.0f));
}

void UDronePhysicsSettingsWidget::AddParameterRow(UVerticalBox &List, int32 RowIndex)
{
    const FParameterRow &Row = ParameterRows[RowIndex];

    UHorizontalBox *Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    List.AddChildToVerticalBox(Line)->SetPadding(FMargin(0.0f, 3.0f));

    UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Row.Label));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    UHorizontalBoxSlot *LabelSlot = Line->AddChildToHorizontalBox(Label);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));

    USizeBox *SpinSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SpinSize->SetWidthOverride(190.0f);
    Line->AddChildToHorizontalBox(SpinSize);

    USpinBox *Spin = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass());
    // the value number sits on the spin box's light fill, so it must be dark to read;
    // the widget's own ForegroundColor overrides the style's, so set it here
    Spin->SetForegroundColor(FSlateColor(FLinearColor::Black));
    Spin->SetMinValue(Row.Min);
    Spin->SetMaxValue(Row.Max);
    Spin->SetMinSliderValue(Row.Min);
    Spin->SetMaxSliderValue(Row.Max);
    Spin->SetMinFractionalDigits(0);
    Spin->SetMaxFractionalDigits(Row.FractionalDigits);
    Spin->SetDelta(Row.FractionalDigits > 0 ? 0.0f : 1.0f);
    Spin->OnValueChanged.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleValueChanged);
    SpinSize->SetContent(Spin);

    SpinBoxes.Add(Spin);
}

auto UDronePhysicsSettingsWidget::AddButton(UHorizontalBox &Bar, const FString &Caption) -> UButton *
{
    UButton *Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock *Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Caption));
    Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    Button->AddChild(Text);
    UHorizontalBoxSlot *ButtonSlot = Bar.AddChildToHorizontalBox(Button);
    ButtonSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
    return Button;
}

void UDronePhysicsSettingsWidget::RebuildFromConfig()
{
    const UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
    bRebuilding = true;
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount && RowIndex < SpinBoxes.Num(); ++RowIndex)
    {
        SpinBoxes[RowIndex]->SetValue(RowValue(Config->Settings, ParameterRows[RowIndex]));
    }
    if (PresetCombo != nullptr) // Direct selection, so HandlePresetSelected ignores it
    {
        PresetCombo->SetSelectedOption(Config->PresetName);
    }
    bRebuilding = false;
}

void UDronePhysicsSettingsWidget::HandleValueChanged(float InValue)
{
    if (bRebuilding)
    {
        return;
    }

    UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount && RowIndex < SpinBoxes.Num(); ++RowIndex)
    {
        SetRowValue(Config->Settings, ParameterRows[RowIndex], SpinBoxes[RowIndex]->GetValue());
    }

    // hand-edited values no longer match any airframe preset
    Config->PresetName = DronePhysicsPresets::CustomName;
    if (PresetCombo != nullptr)
    {
        PresetCombo->ClearSelection();
    }
    OnSettingsChanged.Broadcast();
}

auto UDronePhysicsSettingsWidget::HandleGeneratePresetItem(FString Item) -> UWidget *
{
    UTextBlock *Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(FText::FromString(Item));
    Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 12));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.92f, 0.9f)));
    return Text;
}

void UDronePhysicsSettingsWidget::HandlePresetSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    if (SelectionType == ESelectInfo::Direct) // programmatic changes must not re-apply
    {
        return;
    }

    for (const FDroneAirframePreset &Preset : Presets)
    {
        if (SelectedItem == Preset.Name)
        {
            UDronePhysicsConfig *Config = UDronePhysicsConfig::Get();
            Config->Settings = Preset.Settings;
            Config->PresetName = Preset.Name;
            RebuildFromConfig();
            OnSettingsChanged.Broadcast();
            return;
        }
    }
}

void UDronePhysicsSettingsWidget::HandleSave()
{
    UDronePhysicsConfig::Get()->Save();
}

void UDronePhysicsSettingsWidget::HandleReset()
{
    UDronePhysicsConfig::Get()->ResetToDefaults();
    RebuildFromConfig();
    OnSettingsChanged.Broadcast();
}

void UDronePhysicsSettingsWidget::HandleClose()
{
    OnCloseRequested.Broadcast();
}

auto UDronePhysicsSettingsWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) -> FReply
{
    if (InKeyEvent.GetKey() == EKeys::P || InKeyEvent.GetKey() == EKeys::Escape)
    {
        OnCloseRequested.Broadcast();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
