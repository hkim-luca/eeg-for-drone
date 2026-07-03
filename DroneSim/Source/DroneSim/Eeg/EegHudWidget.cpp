#include "EegHudWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EegGraphPanel.h"
#include "EegRunnerComponent.h"
#include "Styling/CoreStyle.h"
#include "Telemetry/DroneMinimapWidget.h"
#include "Telemetry/DroneTelemetryComponent.h"

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

void UEegHudWidget::SetRunner(UEegRunnerComponent *InRunner)
{
    Runner = InRunner;
    if (GraphPanel != nullptr)
    {
        GraphPanel->SetRunner(InRunner);
    }
}

void UEegHudWidget::SetTelemetry(UDroneTelemetryComponent *InTelemetry)
{
    Telemetry = InTelemetry;
    if (MinimapPanel != nullptr)
    {
        MinimapPanel->SetTelemetry(InTelemetry);
    }
}

void UEegHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // one text line per action inside the designed box, so the winning line can be
    // recolored on its own (a single multi-line TextBlock has only one color); right-aligned
    // so the trailing "%" value lines up regardless of action-name length
    if (ProbabilityBox != nullptr)
    {
        ProbabilityBox->ClearChildren();
        for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
        {
            UTextBlock *Line = NewObject<UTextBlock>(this);
            Line->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", ProbabilityFontSize));
            Line->SetColorAndOpacity(FSlateColor(ProbabilityColor));
            Line->SetJustification(ETextJustify::Right);
            UVerticalBoxSlot *VerticalSlot = ProbabilityBox->AddChildToVerticalBox(Line);
            if (VerticalSlot != nullptr)
            {
                VerticalSlot->SetHorizontalAlignment(HAlign_Right);
            }
            ProbabilityLines.Add(Line);
        }
    }
}

void UEegHudWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    const UEegRunnerComponent *Pinned = Runner.Get();
    if (Pinned == nullptr)
    {
        return;
    }

    // link state, colored so a broken connection is obvious; FEegClient reconnects on its own
    if (ConnectionText != nullptr)
    {
        const bool bConnected = Pinned->IsServerConnected();
        ConnectionText->SetText(bConnected ? NSLOCTEXT("EegHud", "Connected", "EEG SERVER: CONNECTED")
                                           : NSLOCTEXT("EegHud", "Reconnecting", "EEG SERVER: RECONNECTING..."));
        ConnectionText->SetColorAndOpacity(bConnected ? FSlateColor(FLinearColor(0.2f, 1.0f, 0.4f))
                                                      : FSlateColor(FLinearColor(1.0f, 0.3f, 0.2f)));
    }

    // per-action probability distribution of the last result; the winning line is highlighted
    if (!ProbabilityLines.IsEmpty())
    {
        const TConstArrayView<float> Probs = Pinned->GetLastActionProbs();

        int32 BestIndex = INDEX_NONE; // stays INDEX_NONE until the first result arrives
        for (int32 Index = 0; Index < EegConfig::ProbCount; ++Index)
        {
            if (Probs[Index] > 0.0f && (BestIndex == INDEX_NONE || Probs[Index] > Probs[BestIndex]))
            {
                BestIndex = Index;
            }
        }

        for (int32 Index = 0; Index < ProbabilityLines.Num(); ++Index)
        {
            // zero-padded fixed width (00.0% ... 99.9%) so the lines do not jitter as values change
            ProbabilityLines[Index]->SetText(FText::FromString(FString::Printf(
                TEXT("%s %04.1f%%"), *ScenarioActionName(EegConfig::ProbOrder[Index]), Probs[Index] * 100.0f)));
            ProbabilityLines[Index]->SetColorAndOpacity(
                FSlateColor(Index == BestIndex ? ProbabilityHighlightColor : ProbabilityColor));
        }
    }

    // flight status report, bottom-left: attitude, speed and WGS84 position of the pawn
    if (const UDroneTelemetryComponent *PinnedTelemetry = Telemetry.Get();
        PinnedTelemetry != nullptr && StatusText != nullptr)
    {
        const FString Lat = FormatDms(PinnedTelemetry->GetLatitude(), TEXT("N"), TEXT("S"));
        const FString Lon = FormatDms(PinnedTelemetry->GetLongitude(), TEXT("E"), TEXT("W"));

        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("Drone state\nALT   %.1f m\nSPD   %.1f m/s\nHDG   %.0f°\nROLL  %.1f°\nPITCH %.1f°\nYAW   "
                 "%.1f°\nLAT   %s\nLON   %s\nALT(MSL) %.1f m"),
            PinnedTelemetry->GetAltitudeMeters(), PinnedTelemetry->GetSpeedMps(), PinnedTelemetry->GetHeadingDeg(),
            PinnedTelemetry->GetRollDeg(), PinnedTelemetry->GetPitchDeg(), PinnedTelemetry->GetYawDeg(), *Lat, *Lon,
            PinnedTelemetry->GetAltitudeMslMeters())));
    }
}
