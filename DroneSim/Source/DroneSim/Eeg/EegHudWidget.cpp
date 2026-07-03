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
/** StatusLines order: title line followed by one line per field */
constexpr int32 StatusLineCount = 10;

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

    // one text line per status field inside the designed box, matching ProbabilityBox's
    // pattern; left-aligned since these are label-value pairs rather than a ranked list
    if (StatusBox != nullptr)
    {
        StatusBox->ClearChildren();
        for (int32 Index = 0; Index < StatusLineCount; ++Index)
        {
            UTextBlock *Line = NewObject<UTextBlock>(this);
            Line->SetFont(FCoreStyle::GetDefaultFontStyle(Index == 0 ? "Bold" : "Regular", StatusFontSize));
            Line->SetColorAndOpacity(FSlateColor(StatusColor));
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

    // flight status report, bottom-left: attitude, speed and WGS84 position of the pawn,
    // one field per line so each can be styled independently like ProbabilityLines
    if (const UDroneTelemetryComponent *PinnedTelemetry = Telemetry.Get();
        PinnedTelemetry != nullptr && StatusLines.Num() == StatusLineCount)
    {
        const FString Lat = FormatDms(PinnedTelemetry->GetLatitude(), TEXT("N"), TEXT("S"));
        const FString Lon = FormatDms(PinnedTelemetry->GetLongitude(), TEXT("E"), TEXT("W"));

        StatusLines[0]->SetText(NSLOCTEXT("EegHud", "StatusTitle", "Drone state"));
        StatusLines[1]->SetText(
            FText::FromString(FString::Printf(TEXT("ALT   %.1f m"), PinnedTelemetry->GetAltitudeMeters())));
        StatusLines[2]->SetText(
            FText::FromString(FString::Printf(TEXT("SPD   %.1f m/s"), PinnedTelemetry->GetSpeedMps())));
        StatusLines[3]->SetText(
            FText::FromString(FString::Printf(TEXT("HDG   %.0f°"), PinnedTelemetry->GetHeadingDeg())));
        StatusLines[4]->SetText(FText::FromString(FString::Printf(TEXT("ROLL  %.1f°"), PinnedTelemetry->GetRollDeg())));
        StatusLines[5]->SetText(
            FText::FromString(FString::Printf(TEXT("PITCH %.1f°"), PinnedTelemetry->GetPitchDeg())));
        StatusLines[6]->SetText(FText::FromString(FString::Printf(TEXT("YAW   %.1f°"), PinnedTelemetry->GetYawDeg())));
        StatusLines[7]->SetText(FText::FromString(FString::Printf(TEXT("LAT   %s"), *Lat)));
        StatusLines[8]->SetText(FText::FromString(FString::Printf(TEXT("LON   %s"), *Lon)));
        StatusLines[9]->SetText(
            FText::FromString(FString::Printf(TEXT("ALT(MSL) %.1f m"), PinnedTelemetry->GetAltitudeMslMeters())));
    }
}
