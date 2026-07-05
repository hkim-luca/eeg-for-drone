#include "DronePhysicsSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Scenario/DronePhysicsConfig.h"
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
    {TEXT("AIRFRAME"), TEXT("MASS [kg]"),              &FDronePhysicsSettings::MassKg,              nullptr, nullptr, 0.1, 10.0, 3},
    {nullptr,          TEXT("ARM LENGTH [m]"),         &FDronePhysicsSettings::ArmLengthM,          nullptr, nullptr, 0.05, 1.0, 3},
    {nullptr,          TEXT("INERTIA XX [kg m2]"),     &FDronePhysicsSettings::InertiaXX,           nullptr, nullptr, 0.0001, 1.0, 5},
    {nullptr,          TEXT("INERTIA YY [kg m2]"),     &FDronePhysicsSettings::InertiaYY,           nullptr, nullptr, 0.0001, 1.0, 5},
    {nullptr,          TEXT("INERTIA ZZ [kg m2]"),     &FDronePhysicsSettings::InertiaZZ,           nullptr, nullptr, 0.0001, 1.0, 5},
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
    {nullptr,          TEXT("RATE D GAIN"),            &FDronePhysicsSettings::RateDGain,           nullptr, nullptr, 0.0, 10.0, 2},
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

    UBorder *Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    Panel->SetBrushColor(PanelBackground);
    Panel->SetPadding(FMargin(14.0f));
    UCanvasPanelSlot *PanelSlot = Canvas->AddChildToCanvas(Panel);
    PanelSlot->SetAutoSize(true);
    PanelSlot->SetAnchors(FAnchors(0.0f, 0.5f));
    PanelSlot->SetAlignment(FVector2D(0.0, 0.5));
    PanelSlot->SetPosition(FVector2D(40.0, 0.0));

    UVerticalBox *Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Panel->SetContent(Column);

    UTextBlock *Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Title->SetText(FText::FromString(TEXT("DRONE PHYSICS")));
    Title->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 14));
    Title->SetColorAndOpacity(FSlateColor(TitleColor));
    Column->AddChildToVerticalBox(Title)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

    USizeBox *ScrollLimit = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    ScrollLimit->SetMaxDesiredHeight(620.0f);
    Column->AddChildToVerticalBox(ScrollLimit);

    UScrollBox *Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
    ScrollLimit->SetContent(Scroll);

    UVerticalBox *List = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
    Scroll->AddChild(List);

    SpinBoxes.Reset();
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount; ++RowIndex)
    {
        if (ParameterRows[RowIndex].Section != nullptr)
        {
            AddSectionHeader(*List, ParameterRows[RowIndex].Section);
        }
        AddParameterRow(*List, RowIndex);
    }

    UHorizontalBox *ButtonBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    Column->AddChildToVerticalBox(ButtonBar)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

    AddButton(*ButtonBar, TEXT("SAVE"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleSave);
    AddButton(*ButtonBar, TEXT("DEFAULTS"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleReset);
    AddButton(*ButtonBar, TEXT("CLOSE"))->OnClicked.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleClose);
}

void UDronePhysicsSettingsWidget::AddSectionHeader(UVerticalBox &List, const FString &SectionTitle)
{
    UTextBlock *Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Header->SetText(FText::FromString(SectionTitle));
    Header->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 11));
    Header->SetColorAndOpacity(FSlateColor(SectionColor));
    List.AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));
}

void UDronePhysicsSettingsWidget::AddParameterRow(UVerticalBox &List, int32 RowIndex)
{
    const FParameterRow &Row = ParameterRows[RowIndex];

    UHorizontalBox *Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    List.AddChildToVerticalBox(Line)->SetPadding(FMargin(0.0f, 1.0f));

    UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Row.Label));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 10));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    UHorizontalBoxSlot *LabelSlot = Line->AddChildToHorizontalBox(Label);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));

    USizeBox *SpinSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    SpinSize->SetWidthOverride(150.0f);
    Line->AddChildToHorizontalBox(SpinSize);

    USpinBox *Spin = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass());
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
    Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", 10));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    Button->AddChild(Text);
    UHorizontalBoxSlot *ButtonSlot = Bar.AddChildToHorizontalBox(Button);
    ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
    return Button;
}

void UDronePhysicsSettingsWidget::RebuildFromConfig()
{
    const FDronePhysicsSettings &Settings = UDronePhysicsConfig::Get()->Settings;
    bRebuilding = true;
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount && RowIndex < SpinBoxes.Num(); ++RowIndex)
    {
        SpinBoxes[RowIndex]->SetValue(RowValue(Settings, ParameterRows[RowIndex]));
    }
    bRebuilding = false;
}

void UDronePhysicsSettingsWidget::HandleValueChanged(float InValue)
{
    if (bRebuilding)
    {
        return;
    }

    FDronePhysicsSettings &Settings = UDronePhysicsConfig::Get()->Settings;
    for (int32 RowIndex = 0; RowIndex < ParameterRowCount && RowIndex < SpinBoxes.Num(); ++RowIndex)
    {
        SetRowValue(Settings, ParameterRows[RowIndex], SpinBoxes[RowIndex]->GetValue());
    }
    OnSettingsChanged.Broadcast();
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
