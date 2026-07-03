#include "DroneTapeGauge.h"
#include "DroneTelemetryComponent.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

/** Slate widget painting a vertical scrolling tape gauge for one telemetry field */
class SDroneTapePanel final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SDroneTapePanel)
    {
    }
    SLATE_END_ARGS()

    void Construct(const FArguments &InArgs)
    {
        SetCanTick(false); // repainted every frame anyway because the game viewport is volatile
    }

    void SetTelemetry(const TWeakObjectPtr<const UDroneTelemetryComponent> &InTelemetry)
    {
        Telemetry = InTelemetry;
    }

    void SetStyle(EDroneTapeField InField, const FString &InLabel, const FString &InUnit, float InVisibleRange,
                  float InTickInterval, const FLinearColor &InBackgroundColor, const FLinearColor &InTapeColor,
                  const FLinearColor &InPointerColor, int32 InFontSize, const FVector2D &InDesiredSize)
    {
        Field = InField;
        Label = InLabel;
        Unit = InUnit;
        VisibleRange = FMath::Max(InVisibleRange, 1.0f);
        TickInterval = FMath::Max(InTickInterval, 0.1f);
        BackgroundColor = InBackgroundColor;
        TapeColor = InTapeColor;
        PointerColor = InPointerColor;
        FontSize = InFontSize;
        DesiredSize = InDesiredSize;
    }

    auto ComputeDesiredSize(float) const -> FVector2D override
    {
        return DesiredSize;
    }

    auto OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry, const FSlateRect &MyCullingRect,
                 FSlateWindowElementList &OutDrawElements, int32 LayerId, const FWidgetStyle &InWidgetStyle,
                 bool bParentEnabled) const -> int32 override;

  private:
    TWeakObjectPtr<const UDroneTelemetryComponent> Telemetry;
    EDroneTapeField Field = EDroneTapeField::Altitude;
    FString Label = TEXT("ALT");
    FString Unit = TEXT("m");
    float VisibleRange = 60.0f;
    float TickInterval = 10.0f;
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    FLinearColor TapeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.92f);
    FLinearColor PointerColor = FLinearColor(0.03f, 0.45f, 0.14f, 1.0f);
    int32 FontSize = 12;
    FVector2D DesiredSize = FVector2D(74.0f, 220.0f);
};

auto SDroneTapePanel::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                              const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements, int32 LayerId,
                              const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();

    if (BackgroundColor.A > 0.0f)
    {
        FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                                   FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);
    }

    const UDroneTelemetryComponent *Pinned = Telemetry.Get();
    if (Pinned == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    // altitude = height above ground (AGL); speed = horizontal ground speed, the value real
    // drone OSDs label H.S (vertical speed is reported separately in the telemetry readout)
    const double Value =
        Field == EDroneTapeField::Altitude ? Pinned->GetAltitudeAglMeters() : Pinned->GetHorizontalSpeedMps();
    const float CenterY = Size.Y * 0.5f;

    // pixels per unit of value; values increase upward, so a positive delta subtracts from Y
    const float PixelsPerUnit = Size.Y / VisibleRange;
    // OSD-style black outline keeps the floating text readable over bright scenery
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
    Font.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));

    // the gauge label gets its own opaque header band; the scrolling tape is clipped to the
    // area below it so tick labels can never run into the label text
    const float HeaderHeight = static_cast<float>(FontSize) * 1.5f;
    OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry.ToPaintGeometry(
        FVector2f(Size.X, Size.Y - HeaderHeight), FSlateLayoutTransform(FVector2f(0.0f, HeaderHeight)))));

    const double FirstTickValue = FMath::FloorToDouble(Value / TickInterval) * TickInterval - TickInterval;
    const double LastTickValue = Value + VisibleRange * 0.5 + TickInterval;
    for (double TickValue = FirstTickValue; TickValue <= LastTickValue; TickValue += TickInterval)
    {
        const float TickY = CenterY - static_cast<float>((TickValue - Value)) * PixelsPerUnit;
        if (TickY < -static_cast<float>(FontSize) || TickY > Size.Y + static_cast<float>(FontSize))
        {
            continue;
        }

        // leave a gap around the center readout: with no panel boxes (OSD overlay style),
        // ticks passing under the current-value text would collide with it
        if (FMath::Abs(TickY - CenterY) < static_cast<float>(FontSize) * 1.2f)
        {
            continue;
        }

        const float TickLength = Size.X * 0.25f;
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                     {FVector2f(0.0f, TickY), FVector2f(TickLength, TickY)}, ESlateDrawEffect::None,
                                     TapeColor, false, 1.0f);

        const FString TickText = FString::Printf(TEXT("%.0f"), TickValue);
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(
                FVector2f(Size.X - TickLength, static_cast<float>(FontSize)),
                FSlateLayoutTransform(FVector2f(TickLength + 2.0f, TickY - static_cast<float>(FontSize) * 0.5f))),
            TickText, Font, ESlateDrawEffect::None, TapeColor);
    }

    OutDrawElements.PopClip();

    // center pointer line and current-value readout, drawn on top of the scrolling tape
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                 {FVector2f(0.0f, CenterY), FVector2f(Size.X, CenterY)}, ESlateDrawEffect::None,
                                 PointerColor, false, 2.0f);

    FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
    LabelFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
    FSlateDrawElement::MakeText(
        OutDrawElements, LayerId + 3,
        AllottedGeometry.ToPaintGeometry(
            FVector2f(Size.X, static_cast<float>(FontSize)),
            FSlateLayoutTransform(FVector2f(4.0f, (HeaderHeight - static_cast<float>(FontSize)) * 0.5f))),
        Label, LabelFont, ESlateDrawEffect::None, TapeColor);

    // current-value readout over the pointer line; the tick loop above leaves a gap here
    const FString ValueText = FString::Printf(TEXT("%.1f%s"), Value, *Unit);
    FSlateDrawElement::MakeText(
        OutDrawElements, LayerId + 3,
        AllottedGeometry.ToPaintGeometry(
            FVector2f(Size.X, static_cast<float>(FontSize)),
            FSlateLayoutTransform(FVector2f(4.0f, CenterY - static_cast<float>(FontSize) * 0.5f))),
        ValueText, LabelFont, ESlateDrawEffect::None, PointerColor);

    return LayerId + 4;
}

void UDroneTapeGauge::SetTelemetry(const UDroneTelemetryComponent *InTelemetry)
{
    PendingTelemetry = InTelemetry;
    if (Panel.IsValid())
    {
        Panel->SetTelemetry(PendingTelemetry);
    }
}

void UDroneTapeGauge::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Panel.IsValid())
    {
        Panel->SetStyle(Field, Label, Unit, VisibleRange, TickInterval, BackgroundColor, TapeColor, PointerColor,
                        FontSize, PanelDesiredSize);
    }
}

void UDroneTapeGauge::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Panel.Reset();
}

auto UDroneTapeGauge::RebuildWidget() -> TSharedRef<SWidget>
{
    Panel = SNew(SDroneTapePanel);
    Panel->SetTelemetry(PendingTelemetry);
    Panel->SetStyle(Field, Label, Unit, VisibleRange, TickInterval, BackgroundColor, TapeColor, PointerColor, FontSize,
                    PanelDesiredSize);
    return Panel.ToSharedRef();
}
