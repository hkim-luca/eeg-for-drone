#include "DroneMinimapWidget.h"
#include "DroneTelemetryComponent.h"
#include "Math/Box2D.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
constexpr int32 DroneIconCircleSegments = 12;
constexpr float DroneIconRadiusPixels = 5.0f;
constexpr float DroneIconHeadingLengthPixels = 14.0f;
} // namespace

/** Slate widget painting the north-up grid, flight trail and heading icon */
class SDroneMinimapPanel final : public SLeafWidget
{
  public:
    SLATE_BEGIN_ARGS(SDroneMinimapPanel)
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

    void SetStyle(const FLinearColor &InGridColor, const FLinearColor &InBackgroundColor,
                  const FLinearColor &InTrailColor, const FLinearColor &InDroneIconColor, float InGridSpacingMeters,
                  float InMinViewRadiusMeters, const FVector2D &InDesiredSize)
    {
        GridColor = InGridColor;
        BackgroundColor = InBackgroundColor;
        TrailColor = InTrailColor;
        DroneIconColor = InDroneIconColor;
        GridSpacingMeters = InGridSpacingMeters;
        MinViewRadiusMeters = InMinViewRadiusMeters;
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
    FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.15f);
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    FLinearColor TrailColor = FLinearColor(0.1f, 0.9f, 0.4f, 1.0f);
    FLinearColor DroneIconColor = FLinearColor(1.0f, 0.85f, 0.1f, 1.0f);
    float GridSpacingMeters = 10.0f;
    float MinViewRadiusMeters = 10.0f;
    FVector2D DesiredSize = FVector2D(260.0f, 260.0f);
};

auto SDroneMinimapPanel::OnPaint(const FPaintArgs &Args, const FGeometry &AllottedGeometry,
                                 const FSlateRect &MyCullingRect, FSlateWindowElementList &OutDrawElements,
                                 int32 LayerId, const FWidgetStyle &InWidgetStyle, bool /*bParentEnabled*/) const
    -> int32
{
    const FVector2f Size = AllottedGeometry.GetLocalSize();

    FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
                               FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, BackgroundColor);

    const UDroneTelemetryComponent *Pinned = Telemetry.Get();
    if (Pinned == nullptr || Size.X <= 0.0f || Size.Y <= 0.0f)
    {
        return LayerId + 1;
    }

    const FVector2D CurrentPosition = Pinned->GetCurrentPositionMeters();
    const TConstArrayView<FVector2D> Trail = Pinned->GetTrailPoints();

    // world bounding box (East/North meters) covering the trail and the current position,
    // with margin, so the view auto-scales to whatever area the drone has covered
    FBox2D Bounds(ForceInit);
    Bounds += CurrentPosition;
    for (const FVector2D &Point : Trail)
    {
        Bounds += Point;
    }
    const FVector2D Center = Bounds.GetCenter();
    const FVector2D Extent = Bounds.GetExtent();
    const float ViewRadiusMeters =
        FMath::Max3(static_cast<float>(Extent.X), static_cast<float>(Extent.Y), MinViewRadiusMeters) * 1.2f;

    const float PixelsPerMeter = (0.5f * FMath::Min(Size.X, Size.Y)) / ViewRadiusMeters;
    const FVector2f ScreenCenter = Size * 0.5f;

    // East -> screen +X, North -> screen -Y (screen Y grows downward; north stays "up")
    auto WorldToScreen = [&](const FVector2D &WorldPoint) -> FVector2f {
        const FVector2D Offset = WorldPoint - Center;
        return ScreenCenter +
               FVector2f(static_cast<float>(Offset.X) * PixelsPerMeter, static_cast<float>(-Offset.Y) * PixelsPerMeter);
    };

    // grid lines at fixed world spacing, spanning the visible view radius
    const int32 LineCountEachSide = FMath::CeilToInt(ViewRadiusMeters / GridSpacingMeters) + 1;
    for (int32 Index = -LineCountEachSide; Index <= LineCountEachSide; ++Index)
    {
        const double Offset = static_cast<double>(Index) * GridSpacingMeters;

        const FVector2f Vertical0 = WorldToScreen(FVector2D(Center.X + Offset, Center.Y - ViewRadiusMeters));
        const FVector2f Vertical1 = WorldToScreen(FVector2D(Center.X + Offset, Center.Y + ViewRadiusMeters));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                     {Vertical0, Vertical1}, ESlateDrawEffect::None, GridColor, false, 1.0f);

        const FVector2f Horizontal0 = WorldToScreen(FVector2D(Center.X - ViewRadiusMeters, Center.Y + Offset));
        const FVector2f Horizontal1 = WorldToScreen(FVector2D(Center.X + ViewRadiusMeters, Center.Y + Offset));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(),
                                     {Horizontal0, Horizontal1}, ESlateDrawEffect::None, GridColor, false, 1.0f);
    }

    // flight trail
    if (Trail.Num() >= 2)
    {
        TArray<FVector2f> TrailScreenPoints;
        TrailScreenPoints.Reserve(Trail.Num());
        for (const FVector2D &Point : Trail)
        {
            TrailScreenPoints.Add(WorldToScreen(Point));
        }
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                     TrailScreenPoints, ESlateDrawEffect::None, TrailColor, false, 2.0f);
    }

    // drone icon: circle at the current position plus a heading tick. HeadingDeg is the actor's
    // Yaw, and Yaw 0 faces +X (East, per the East/North convention in DroneTelemetryComponent),
    // which maps directly to screen (cos, sin) since East is screen +X and North is screen -Y.
    const FVector2f IconCenter = WorldToScreen(CurrentPosition);
    TArray<FVector2f> CirclePoints;
    CirclePoints.Reserve(DroneIconCircleSegments + 1);
    for (int32 Segment = 0; Segment <= DroneIconCircleSegments; ++Segment)
    {
        const float Angle = (2.0f * PI * static_cast<float>(Segment)) / static_cast<float>(DroneIconCircleSegments);
        CirclePoints.Add(IconCenter + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * DroneIconRadiusPixels);
    }
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), CirclePoints,
                                 ESlateDrawEffect::None, DroneIconColor, false, 2.0f);

    const float HeadingRadians = FMath::DegreesToRadians(Pinned->GetHeadingDeg());
    const FVector2f HeadingDirection(FMath::Cos(HeadingRadians), FMath::Sin(HeadingRadians));
    const FVector2f HeadingTip = IconCenter + HeadingDirection * DroneIconHeadingLengthPixels;
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
                                 {IconCenter, HeadingTip}, ESlateDrawEffect::None, DroneIconColor, false, 2.0f);

    return LayerId + 4;
}

void UDroneMinimapWidget::SetTelemetry(const UDroneTelemetryComponent *InTelemetry)
{
    PendingTelemetry = InTelemetry;
    if (Panel.IsValid())
    {
        Panel->SetTelemetry(PendingTelemetry);
    }
}

void UDroneMinimapWidget::SynchronizeProperties()
{
    Super::SynchronizeProperties();
    if (Panel.IsValid())
    {
        Panel->SetStyle(GridColor, BackgroundColor, TrailColor, DroneIconColor, GridSpacingMeters, MinViewRadiusMeters,
                        PanelDesiredSize);
    }
}

void UDroneMinimapWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    Panel.Reset();
}

auto UDroneMinimapWidget::RebuildWidget() -> TSharedRef<SWidget>
{
    Panel = SNew(SDroneMinimapPanel);
    Panel->SetTelemetry(PendingTelemetry);
    Panel->SetStyle(GridColor, BackgroundColor, TrailColor, DroneIconColor, GridSpacingMeters, MinViewRadiusMeters,
                    PanelDesiredSize);
    return Panel.ToSharedRef();
}
