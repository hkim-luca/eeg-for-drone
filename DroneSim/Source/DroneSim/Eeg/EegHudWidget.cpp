#include "EegHudWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EegGraphPanel.h"
#include "EegRunnerComponent.h"
#include "Styling/CoreStyle.h"
#include "Telemetry/DroneAttitudeIndicator.h"
#include "Telemetry/DroneCompassTape.h"
#include "Telemetry/DroneMinimapWidget.h"
#include "Telemetry/DroneProbabilityPanel.h"
#include "Telemetry/DroneTapeGauge.h"
#include "Telemetry/DroneTelemetryComponent.h"

namespace
{
/** StatusLines order: title line followed by one line per field */
constexpr int32 StatusLineCount = 7;

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

void UEegHudWidget::SetRunner(UEegRunnerComponent *InRunner)
{
    Runner = InRunner;
    if (GraphPanel != nullptr)
    {
        GraphPanel->SetRunner(InRunner);
    }
    if (ProbabilityPanel != nullptr)
    {
        ProbabilityPanel->SetRunner(InRunner);
    }
}

void UEegHudWidget::SetTelemetry(UDroneTelemetryComponent *InTelemetry)
{
    Telemetry = InTelemetry;
    if (MinimapPanel != nullptr)
    {
        MinimapPanel->SetTelemetry(InTelemetry);
    }
    if (AltitudeGauge != nullptr)
    {
        AltitudeGauge->SetTelemetry(InTelemetry);
    }
    if (SpeedGauge != nullptr)
    {
        SpeedGauge->SetTelemetry(InTelemetry);
    }
    if (AttitudeIndicator != nullptr)
    {
        AttitudeIndicator->SetTelemetry(InTelemetry);
    }
    if (HeadingTape != nullptr)
    {
        HeadingTape->SetTelemetry(InTelemetry);
    }
}

void UEegHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // one text line per position field inside the designed box; left-aligned since these
    // are label-value pairs rather than a ranked list
    if (StatusBox != nullptr)
    {
        StatusBox->ClearChildren();
        for (int32 Index = 0; Index < StatusLineCount; ++Index)
        {
            UTextBlock *Line = NewObject<UTextBlock>(this);
            Line->SetFont(FCoreStyle::GetDefaultFontStyle(Index == 0 ? "Bold" : "Regular", StatusFontSize));
            Line->SetColorAndOpacity(FSlateColor(StatusColor));
            // OSD-style drop shadow keeps the floating text readable over bright scenery
            Line->SetShadowOffset(FVector2D(1.0, 1.0));
            Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
            Line->SetJustification(ETextJustify::Left);
            UVerticalBoxSlot *VerticalSlot = StatusBox->AddChildToVerticalBox(Line);
            if (VerticalSlot != nullptr)
            {
                VerticalSlot->SetHorizontalAlignment(HAlign_Left);
            }
            StatusLines.Add(Line);
        }
    }
}

void UEegHudWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // link state, colored so a broken connection is obvious; FEegClient reconnects on its own
    if (const UEegRunnerComponent *PinnedRunner = Runner.Get(); PinnedRunner != nullptr && ConnectionText != nullptr)
    {
        const bool bConnected = PinnedRunner->IsServerConnected();
        ConnectionText->SetText(bConnected ? NSLOCTEXT("EegHud", "Connected", "EEG SERVER: CONNECTED")
                                           : NSLOCTEXT("EegHud", "Reconnecting", "EEG SERVER: RECONNECTING..."));
        ConnectionText->SetColorAndOpacity(bConnected ? FSlateColor(FLinearColor(0.03f, 0.45f, 0.14f))
                                                      : FSlateColor(FLinearColor(0.7f, 0.1f, 0.05f)));
    }

    // OSD-style telemetry readout, bottom-left, mirroring what real drone pilots monitor:
    // flight time, distance to home, vertical speed, position; AGL/H.S/HDG/attitude are
    // shown by the dedicated gauges
    if (const UDroneTelemetryComponent *PinnedTelemetry = Telemetry.Get();
        PinnedTelemetry != nullptr && StatusLines.Num() == StatusLineCount)
    {
        const FString Lat = FormatDms(PinnedTelemetry->GetLatitude(), TEXT("N"), TEXT("S"));
        const FString Lon = FormatDms(PinnedTelemetry->GetLongitude(), TEXT("E"), TEXT("W"));
        const int32 FlightSeconds = static_cast<int32>(PinnedTelemetry->GetFlightTimeSeconds());

        StatusLines[0]->SetText(NSLOCTEXT("EegHud", "StatusTitle", "TELEMETRY"));
        StatusLines[1]->SetText(
            FText::FromString(FString::Printf(TEXT("FLT   %02d:%02d"), FlightSeconds / 60, FlightSeconds % 60)));
        StatusLines[2]->SetText(
            FText::FromString(FString::Printf(TEXT("DST   %.1f m"), PinnedTelemetry->GetDistanceToHomeMeters())));
        StatusLines[3]->SetText(
            FText::FromString(FString::Printf(TEXT("V.S   %+.1f m/s"), PinnedTelemetry->GetVerticalSpeedMps())));
        StatusLines[4]->SetText(FText::FromString(FString::Printf(TEXT("LAT   %s"), *Lat)));
        StatusLines[5]->SetText(FText::FromString(FString::Printf(TEXT("LON   %s"), *Lon)));
        StatusLines[6]->SetText(
            FText::FromString(FString::Printf(TEXT("ALT   %.1f m MSL"), PinnedTelemetry->GetAltitudeMslMeters())));
    }
}
