#include "DronePhysicsSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DronePhysicsSettingsViewModel.h"
#include "Styling/CoreStyle.h"

namespace
{
/** OSD palette shared with the telemetry widgets */
const FLinearColor PanelBackground(0.01f, 0.012f, 0.01f, 0.88f);
const FLinearColor LabelColor(0.5f, 0.52f, 0.5f, 1.0f);
const FLinearColor SectionColor(0.75f, 0.35f, 0.02f, 1.0f);
const FLinearColor TitleColor(0.03f, 0.45f, 0.14f, 1.0f);
} // namespace

auto UDronePhysicsSettingsWidget::Initialize() -> bool
{
    const bool bOk = Super::Initialize();
    // the panel handles P/Esc itself, so it must be able to take keyboard focus
    SetIsFocusable(true);
    if (WidgetTree != nullptr && WidgetTree->RootWidget == nullptr)
    {
        ViewModel = NewObject<UDronePhysicsSettingsViewModel>(this);
        ViewModel->OnStateChanged.AddUObject(this, &UDronePhysicsSettingsWidget::RefreshFromViewModel);
        BuildLayout();
        RefreshFromViewModel();
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
    UVerticalBox *ControlColumn = nullptr;
    int32 BottomRowColumns = 0;
    for (int32 RowIndex = 0; RowIndex < UDronePhysicsSettingsViewModel::ParameterCount(); ++RowIndex)
    {
        const TCHAR *Section = UDronePhysicsSettingsViewModel::ParameterDesc(RowIndex).Section;
        if (Section != nullptr)
        {
            const bool bBottomRow =
                FCString::Strcmp(Section, TEXT("CONTROL")) == 0 || FCString::Strcmp(Section, TEXT("SETTLE")) == 0;
            CurrentColumn = AddColumn(*GridRows[bBottomRow ? 1 : 0]);
            BottomRowColumns += bBottomRow ? 1 : 0;
            AddSectionHeader(*CurrentColumn, Section);
            if (FCString::Strcmp(Section, TEXT("CONTROL")) == 0)
            {
                ControlColumn = CurrentColumn;
            }
        }
        AddParameterRow(*CurrentColumn, RowIndex);
    }

    // the yaw/action mode toggles are checkboxes, not spin boxes, so they sit outside
    // the parameter table; append them below the CONTROL section
    MouseYawCheck = AddToggleRow(*ControlColumn, TEXT("MOUSE YAW (OFF: CAM LOCKED)"));
    MouseYawCheck->OnCheckStateChanged.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleMouseYawChanged);
    TurnModeCheck = AddToggleRow(*ControlColumn, TEXT("TURN WITH LEFT/RIGHT"));
    TurnModeCheck->OnCheckStateChanged.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleTurnModeChanged);

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
    for (const FString &PresetName : ViewModel->GetPresetNames())
    {
        PresetCombo->AddOption(PresetName);
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
    const UDronePhysicsSettingsViewModel::FParameterDesc &Desc =
        UDronePhysicsSettingsViewModel::ParameterDesc(RowIndex);

    UHorizontalBox *Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    List.AddChildToVerticalBox(Line)->SetPadding(FMargin(0.0f, 3.0f));

    UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(Desc.Label));
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
    Spin->SetMinValue(Desc.Min);
    Spin->SetMaxValue(Desc.Max);
    Spin->SetMinSliderValue(Desc.Min);
    Spin->SetMaxSliderValue(Desc.Max);
    Spin->SetMinFractionalDigits(0);
    Spin->SetMaxFractionalDigits(Desc.FractionalDigits);
    Spin->SetDelta(Desc.FractionalDigits > 0 ? 0.0f : 1.0f);
    Spin->OnValueChanged.AddDynamic(this, &UDronePhysicsSettingsWidget::HandleValueChanged);
    SpinSize->SetContent(Spin);

    SpinBoxes.Add(Spin);
}

auto UDronePhysicsSettingsWidget::AddToggleRow(UVerticalBox &List, const FString &LabelText) -> UCheckBox *
{
    UHorizontalBox *Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
    List.AddChildToVerticalBox(Line)->SetPadding(FMargin(0.0f, 3.0f));

    UTextBlock *Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Label->SetText(FText::FromString(LabelText));
    Label->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 11));
    Label->SetColorAndOpacity(FSlateColor(LabelColor));
    UHorizontalBoxSlot *LabelSlot = Line->AddChildToHorizontalBox(Label);
    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    LabelSlot->SetVerticalAlignment(VAlign_Center);
    LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));

    UCheckBox *Check = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
    UHorizontalBoxSlot *CheckSlot = Line->AddChildToHorizontalBox(Check);
    CheckSlot->SetVerticalAlignment(VAlign_Center);
    return Check;
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

void UDronePhysicsSettingsWidget::RefreshFromViewModel()
{
    bRebuilding = true;
    for (int32 RowIndex = 0; RowIndex < UDronePhysicsSettingsViewModel::ParameterCount() && RowIndex < SpinBoxes.Num();
         ++RowIndex)
    {
        SpinBoxes[RowIndex]->SetValue(ViewModel->GetParameterValue(RowIndex));
    }
    if (MouseYawCheck != nullptr)
    {
        MouseYawCheck->SetIsChecked(ViewModel->GetMouseYawControl());
    }
    if (TurnModeCheck != nullptr)
    {
        TurnModeCheck->SetIsChecked(ViewModel->GetTurnWithLeftRight());
    }
    if (PresetCombo != nullptr) // Direct selection, so HandlePresetSelected ignores it
    {
        const FString PresetName = ViewModel->GetPresetName();
        if (PresetCombo->FindOptionIndex(PresetName) != INDEX_NONE)
        {
            PresetCombo->SetSelectedOption(PresetName);
        }
        else // hand-edited values: CUSTOM is not an airframe option
        {
            PresetCombo->ClearSelection();
        }
    }
    bRebuilding = false;
}

void UDronePhysicsSettingsWidget::HandleValueChanged(float InValue)
{
    if (bRebuilding)
    {
        return;
    }

    // the spin box event does not say which box changed; forward every value and let
    // the ViewModel ignore the unchanged ones
    for (int32 RowIndex = 0; RowIndex < UDronePhysicsSettingsViewModel::ParameterCount() && RowIndex < SpinBoxes.Num();
         ++RowIndex)
    {
        ViewModel->SetParameterValue(RowIndex, SpinBoxes[RowIndex]->GetValue());
    }
}

void UDronePhysicsSettingsWidget::HandleMouseYawChanged(bool bIsChecked)
{
    if (bRebuilding)
    {
        return;
    }
    ViewModel->SetMouseYawControl(bIsChecked);
}

void UDronePhysicsSettingsWidget::HandleTurnModeChanged(bool bIsChecked)
{
    if (bRebuilding)
    {
        return;
    }
    ViewModel->SetTurnWithLeftRight(bIsChecked);
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
    ViewModel->SelectPreset(SelectedItem);
}

void UDronePhysicsSettingsWidget::HandleSave()
{
    ViewModel->Save();
    OnSettingsSaved.Broadcast();
}

void UDronePhysicsSettingsWidget::HandleReset()
{
    ViewModel->RestoreDefaults();
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
