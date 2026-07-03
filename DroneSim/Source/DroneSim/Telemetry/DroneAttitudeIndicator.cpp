#include "DroneAttitudeIndicator.h"
#include "DroneTelemetryComponent.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

/** Slate widget painting the sky/ground horizon and a fixed aircraft symbol */
class SDroneAttitudePanel final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SDroneAttitudePanel)
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

    void SetStyle(const FLinearColor &InSkyColor, const FLinearColor &InGroundColor, const FLinearColor &InLineColor,
                  float InPixelsPerPitchDegree, const FVector2D &InDesiredSize)
    {
        SkyColor = InSkyColor;
        GroundColor = InGroundColor;
        LineColor = InLineColor;
        PixelsPerPitchDegree = FMath::Max(InPixelsPerPitchDegree, 0.1f);
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
    FLinearColor SkyColor = FLinearColor(0.15f, 0.35f, 0.65f, 0.85f);
    FLinearColor GroundColor = FLinearColor(0.35f, 0.25f, 0.1f, 0.85f);
    FLinearColor LineColor = FLinearColor::White;
    float PixelsPerPitchDegree = 3.0f;
    FVector2D DesiredSize = FVector2D(140.0f, 140.0f);
};

auto SDroneAttitudePanel::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                                  const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements,
                                  int32 LayerId, const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const
    -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();
    if (Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    const UDroneTelemetryComponent *Pinned = Telemetry.Get();
    const double Roll = Pinned != nullptr ? Pinned->GetRollDeg() : 0.0;
    const double Pitch = Pinned != nullptr ? Pinned->GetPitchDeg() : 0.0;

    const FVector2f Center(Size.X * 0.5f, Size.Y * 0.5f);
    // generous overscan so the horizon rotation never reveals a gap at the panel corners
    const float Margin = FMath::Max(Size.X, Size.Y);
    // pitching the nose up shows more sky, which moves the horizon line down on screen
    const float HorizonY = Center.Y + static_cast<float>(Pitch) * PixelsPerPitchDegree;
    // the horizon rotates opposite to the bank so it stays level with the real world as the
    // aircraft symbol (fixed, drawn after the clip below) rolls with the drone
    const float RollRadians = FMath::DegreesToRadians(static_cast<float>(-Roll));

    const FSlateBrush *WhiteBrush = FCoreStyle::Get().GetBrush("WhiteBrush");

    OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

    const FVector2f SkyPosition(-Margin, HorizonY - Margin * 2.0f);
    const FVector2f SkySize(Size.X + Margin * 2.0f, Margin * 2.0f);
    FSlateDrawElement::MakeRotatedBox(OutDrawElements, LayerId + 1,
                                      AllottedGeometry.ToPaintGeometry(SkySize, FSlateLayoutTransform(SkyPosition)),
                                      WhiteBrush, ESlateDrawEffect::None, RollRadians, Center - SkyPosition,
                                      FSlateDrawElement::RelativeToElement, SkyColor);

    const FVector2f GroundPosition(-Margin, HorizonY);
    const FVector2f GroundSize(Size.X + Margin * 2.0f, Margin * 2.0f);
    FSlateDrawElement::MakeRotatedBox(
        OutDrawElements, LayerId + 1,
        AllottedGeometry.ToPaintGeometry(GroundSize, FSlateLayoutTransform(GroundPosition)), WhiteBrush,
        ESlateDrawEffect::None, RollRadians, Center - GroundPosition, FSlateDrawElement::RelativeToElement,
        GroundColor);

    OutDrawElements.PopClip();

    // fixed aircraft symbol (two wing stubs and a center gap), drawn unrotated at the true
    // center so it always represents the drone's own frame of reference
    const float WingHalfWidth = Size.X * 0.18f;
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                 {FVector2f(Center.X - WingHalfWidth, Center.Y), FVector2f(Center.X - 4.0f, Center.Y)},
                                 ESlateDrawEffect::None, LineColor, false, 2.0f);
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                 {FVector2f(Center.X + 4.0f, Center.Y), FVector2f(Center.X + WingHalfWidth, Center.Y)},
                                 ESlateDrawEffect::None, LineColor, false, 2.0f);

    return LayerId + 3;
}

void UDroneAttitudeIndicator::SetTelemetry(const UDroneTelemetryComponent *InTelemetry)
{
    PendingTelemetry = InTelemetry;
    if (Panel.IsValid())
    {
        Panel->SetTelemetry(PendingTelemetry);
    }
}

void UDroneAttitudeIndicator::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Panel.IsValid())
    {
        Panel->SetStyle(SkyColor, GroundColor, LineColor, PixelsPerPitchDegree, PanelDesiredSize);
    }
}

void UDroneAttitudeIndicator::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Panel.Reset();
}

auto UDroneAttitudeIndicator::RebuildWidget() -> TSharedRef<SWidget>
{
    Panel = SNew(SDroneAttitudePanel);
    Panel->SetTelemetry(PendingTelemetry);
    Panel->SetStyle(SkyColor, GroundColor, LineColor, PixelsPerPitchDegree, PanelDesiredSize);
    return Panel.ToSharedRef();
}
