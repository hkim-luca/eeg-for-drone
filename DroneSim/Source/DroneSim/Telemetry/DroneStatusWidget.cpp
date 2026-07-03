#include "DroneStatusWidget.h"
#include "Components/TextBlock.h"
#include "DroneMinimapWidget.h"
#include "DroneTelemetryComponent.h"

namespace
{
/** Formats a signed degree value as DMS with a hemisphere letter, e.g. 37°33'59.4"N */
auto FormatDms(double Degrees, const TCHAR *PositiveHemisphere, const TCHAR *NegativeHemisphere) -> FString
{
    const TCHAR *Hemisphere = Degrees >= 0.0 ? PositiveHemisphere : NegativeHemisphere;
    const double AbsDegrees = FMath::Abs(Degrees);
    const int32 WholeDegrees = static_cast<int32>(AbsDegrees);
    const double MinutesFloat = (AbsDegrees - WholeDegrees) * 60.0;
    const int32 WholeMinutes = static_cast<int32>(MinutesFloat);
    const double Seconds = (MinutesFloat - WholeMinutes) * 60.0;
    return FString::Printf(TEXT("%d°%02d'%04.1f\"%s"), WholeDegrees, WholeMinutes, Seconds, Hemisphere);
}
} // namespace

void UDroneStatusWidget::SetTelemetry(UDroneTelemetryComponent *InTelemetry)
{
    Telemetry = InTelemetry;
    if (MinimapPanel != nullptr)
    {
        MinimapPanel->SetTelemetry(InTelemetry);
    }
}

void UDroneStatusWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const UDroneTelemetryComponent *Pinned = Telemetry.Get();
    if (Pinned == nullptr || StatusText == nullptr)
    {
        return;
    }

    const FString Lat = FormatDms(Pinned->GetLatitude(), TEXT("N"), TEXT("S"));
    const FString Lon = FormatDms(Pinned->GetLongitude(), TEXT("E"), TEXT("W"));

    StatusText->SetText(FText::FromString(FString::Printf(
        TEXT("Drone state\nALT   %.1f m\nSPD   %.1f m/s\nHDG   %.0f°\nROLL  %.1f°\nPITCH %.1f°\nYAW   "
             "%.1f°\nLAT   %s\nLON   %s\nALT(MSL) %.1f m"),
        Pinned->GetAltitudeMeters(), Pinned->GetSpeedMps(), Pinned->GetHeadingDeg(), Pinned->GetRollDeg(),
        Pinned->GetPitchDeg(), Pinned->GetYawDeg(), *Lat, *Lon, Pinned->GetAltitudeMslMeters())));
}
