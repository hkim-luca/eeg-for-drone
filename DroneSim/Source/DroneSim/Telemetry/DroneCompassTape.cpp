#include "DroneCompassTape.h"
#include "DroneTelemetryComponent.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
/** Cardinal letter at a normalized [0, 360) heading, or empty if not exactly on one */
auto CardinalLetter(double NormalizedHeading) -> FString
{
    if (FMath::IsNearlyEqual(NormalizedHeading, 0.0, 0.01))
    {
        return TEXT("N");
    }
    if (FMath::IsNearlyEqual(NormalizedHeading, 90.0, 0.01))
    {
        return TEXT("E");
    }
    if (FMath::IsNearlyEqual(NormalizedHeading, 180.0, 0.01))
    {
        return TEXT("S");
    }
    if (FMath::IsNearlyEqual(NormalizedHeading, 270.0, 0.01))
    {
        return TEXT("W");
    }
    return FString();
}
} // namespace

/** Slate widget painting a horizontal scrolling compass heading tape */
class SDroneCompassPanel final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SDroneCompassPanel)
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

    void SetStyle(float InVisibleRange, float InTickInterval, const FLinearColor &InBackgroundColor,
                  const FLinearColor &InTapeColor, const FLinearColor &InPointerColor, int32 InFontSize,
                  const FVector2D &InDesiredSize)
    {
        VisibleRange = FMath::Clamp(InVisibleRange, 10.0f, 360.0f);
        TickInterval = FMath::Max(InTickInterval, 1.0f);
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
    float VisibleRange = 120.0f;
    float TickInterval = 30.0f;
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    FLinearColor TapeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);
    FLinearColor PointerColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.8f);
    int32 FontSize = 12;
    FVector2D DesiredSize = FVector2D(220.0f, 44.0f);
};

auto SDroneCompassPanel::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                                 const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements,
                                 int32 LayerId, const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const
    -> int32
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

    const double Heading = Pinned->GetHeadingDeg();
    const float CenterX = Size.X * 0.5f;
    const float PixelsPerDegree = Size.X / VisibleRange;
    // OSD-style black outline keeps the floating text readable over bright scenery
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", FontSize);
    Font.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

    OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

    const double FirstTick = FMath::FloorToDouble((Heading - VisibleRange * 0.5) / TickInterval) * TickInterval;
    const double LastTick = Heading + VisibleRange * 0.5 + TickInterval;
    const float TickLength = Size.Y * 0.25f;
    for (double TickValue = FirstTick; TickValue <= LastTick; TickValue += TickInterval)
    {
        const float TickX = CenterX + static_cast<float>(TickValue - Heading) * PixelsPerDegree;
        if (TickX < -static_cast<float>(FontSize) || TickX > Size.X + static_cast<float>(FontSize))
        {
            continue;
        }

        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                     {FVector2f(TickX, 0.0f), FVector2f(TickX, TickLength)}, ESlateDrawEffect::None,
                                     TapeColor, false, 1.0f);

        const double Normalized = FMath::Fmod(FMath::Fmod(TickValue, 360.0) + 360.0, 360.0);
        const FString CardinalText = CardinalLetter(Normalized);
        const FString TickText = CardinalText.IsEmpty() ? FString::Printf(TEXT("%.0f"), Normalized) : CardinalText;
        const bool bCardinal = !CardinalText.IsEmpty();
        // centered under the tick line, using the measured text width
        const FVector2f TextSize(FontMeasure->Measure(TickText, Font));
        FSlateDrawElement::MakeText(
            OutDrawElements, LayerId + 1,
            AllottedGeometry.ToPaintGeometry(
                TextSize, FSlateLayoutTransform(FVector2f(TickX - TextSize.X * 0.5f, TickLength + 2.0f))),
            TickText, Font, ESlateDrawEffect::None, bCardinal ? PointerColor : TapeColor);
    }

    OutDrawElements.PopClip();

    // fixed center pointer marking the current heading
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                 {FVector2f(CenterX, 0.0f), FVector2f(CenterX, Size.Y)}, ESlateDrawEffect::None,
                                 PointerColor, false, 1.0f);

    // numeric readout centered in the bottom band, below the tick-label band
    FSlateFontInfo BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
    BoldFont.OutlineSettings = FFontOutlineSettings(1, FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    const FString HeadingText = FString::Printf(TEXT("%03.0f"), Heading);
    const FVector2f HeadingTextSize(FontMeasure->Measure(HeadingText, BoldFont));
    FSlateDrawElement::MakeText(
        OutDrawElements, LayerId + 3,
        AllottedGeometry.ToPaintGeometry(
            HeadingTextSize,
            FSlateLayoutTransform(FVector2f(CenterX - HeadingTextSize.X * 0.5f, Size.Y - HeadingTextSize.Y - 2.0f))),
        HeadingText, BoldFont, ESlateDrawEffect::None, PointerColor);

    return LayerId + 4;
}

void UDroneCompassTape::SetTelemetry(const UDroneTelemetryComponent *InTelemetry)
{
    PendingTelemetry = InTelemetry;
    if (Panel.IsValid())
    {
        Panel->SetTelemetry(PendingTelemetry);
    }
}

void UDroneCompassTape::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Panel.IsValid())
    {
        Panel->SetStyle(VisibleRange, TickInterval, BackgroundColor, TapeColor, PointerColor, FontSize,
                        PanelDesiredSize);
    }
}

void UDroneCompassTape::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Panel.Reset();
}

auto UDroneCompassTape::RebuildWidget() -> TSharedRef<SWidget>
{
    Panel = SNew(SDroneCompassPanel);
    Panel->SetTelemetry(PendingTelemetry);
    Panel->SetStyle(VisibleRange, TickInterval, BackgroundColor, TapeColor, PointerColor, FontSize, PanelDesiredSize);
    return Panel.ToSharedRef();
}
