#include "DroneMinimapWidget.h"
#include "DroneTelemetryComponent.h"
#include "Layout/Clipping.h"
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
                  const FLinearColor &InTrailColor, const FLinearColor &InDroneIconColor,
                  const FLinearColor &InHomeColor, float InGridSpacingMeters, float InMinViewRadiusMeters,
                  const FVector2D &InDesiredSize)
    {
        GridColor = InGridColor;
        BackgroundColor = InBackgroundColor;
        TrailColor = InTrailColor;
        DroneIconColor = InDroneIconColor;
        HomeColor = InHomeColor;
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
    FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.06f);
    FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.6f);
    FLinearColor TrailColor = FLinearColor(0.03f, 0.45f, 0.14f, 0.6f);
    FLinearColor DroneIconColor = FLinearColor(0.75f, 0.35f, 0.02f, 0.8f);
    FLinearColor HomeColor = FLinearColor(0.5f, 0.52f, 0.5f, 0.6f);
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

    // grid/trail/icon math intentionally draws a little past the panel edge (see the +1 line
    // count buffer below), so clip everything after the background to the panel's own bounds
    OutDrawElements.PushClip(FSlateClippingZone(AllottedGeometry));

    const FVector2D CurrentPosition = Pinned->GetCurrentPositionMeters();
    const FVector2D HomePosition = Pinned->GetHomePositionMeters();
    const TConstArrayView<FVector2D> Trail = Pinned->GetTrailPoints();

    // world bounding box (East/North meters) covering the home point, the trail and the
    // current position, with margin, so the view auto-scales to the area the drone covered
    FBox2D Bounds(ForceInit);
    Bounds += CurrentPosition;
    Bounds += HomePosition;
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

    // flight trail; the current position is appended as the last point so the line always
    // reaches the drone icon instead of stopping at the last sample (trail points are only
    // recorded once per TrailSampleInterval, but the icon moves every frame)
    if (!Trail.IsEmpty())
    {
        TArray<FVector2f> TrailScreenPoints;
        TrailScreenPoints.Reserve(Trail.Num() + 1);
        for (const FVector2D &Point : Trail)
        {
            TrailScreenPoints.Add(WorldToScreen(Point));
        }
        TrailScreenPoints.Add(WorldToScreen(CurrentPosition));
        FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(),
                                     TrailScreenPoints, ESlateDrawEffect::None, TrailColor, false, 1.5f);
    }

    // home (takeoff) point: small hollow square, like the "H" marker on real drone maps
    const FVector2f HomeCenter = WorldToScreen(HomePosition);
    constexpr float HomeHalfSize = 4.0f;
    FSlateDrawElement::MakeLines(
        OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
        {HomeCenter + FVector2f(-HomeHalfSize, -HomeHalfSize), HomeCenter + FVector2f(HomeHalfSize, -HomeHalfSize),
         HomeCenter + FVector2f(HomeHalfSize, HomeHalfSize), HomeCenter + FVector2f(-HomeHalfSize, HomeHalfSize),
         HomeCenter + FVector2f(-HomeHalfSize, -HomeHalfSize)},
        ESlateDrawEffect::None, HomeColor, false, 1.5f);

    // drone icon: circle at the current position plus a heading tick. HeadingDeg is the compass
    // azimuth from true north (0 = north = screen up), so the tick direction is (sin, -cos) of it.
    const FVector2f IconCenter = WorldToScreen(CurrentPosition);
    TArray<FVector2f> CirclePoints;
    CirclePoints.Reserve(DroneIconCircleSegments + 1);
    for (int32 Segment = 0; Segment <= DroneIconCircleSegments; ++Segment)
    {
        const float Angle = (2.0f * PI * static_cast<float>(Segment)) / static_cast<float>(DroneIconCircleSegments);
        CirclePoints.Add(IconCenter + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * DroneIconRadiusPixels);
    }
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(), CirclePoints,
                                 ESlateDrawEffect::None, DroneIconColor, false, 1.5f);

    const float HeadingRadians = FMath::DegreesToRadians(Pinned->GetHeadingDeg());
    const FVector2f HeadingDirection(FMath::Sin(HeadingRadians), -FMath::Cos(HeadingRadians));
    const FVector2f HeadingTip = IconCenter + HeadingDirection * DroneIconHeadingLengthPixels;
    FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3, AllottedGeometry.ToPaintGeometry(),
                                 {IconCenter, HeadingTip}, ESlateDrawEffect::None, DroneIconColor, false, 1.5f);

    OutDrawElements.PopClip();
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
        Panel->SetStyle(GridColor, BackgroundColor, TrailColor, DroneIconColor, HomeColor, GridSpacingMeters,
                        MinViewRadiusMeters, PanelDesiredSize);
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
    Panel->SetStyle(GridColor, BackgroundColor, TrailColor, DroneIconColor, HomeColor, GridSpacingMeters,
                    MinViewRadiusMeters, PanelDesiredSize);
    return Panel.ToSharedRef();
}
