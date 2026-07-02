#pragma once

#include "CoreMinimal.h"

/**
 *  KST (UTC+9) wall-clock helpers shared by CSV recording, log lines and file names.
 *  FDateTime carries no time zone, so KST values are UTC shifted by +9 hours and
 *  the ISO 8601 offset suffix must be appended explicitly.
 */
namespace ScenarioTime
{
/** Current wall-clock time in KST (UTC+9) */
inline auto NowKst() -> FDateTime
{
    return FDateTime::UtcNow() + FTimespan::FromHours(9);
}

/** ISO 8601 with explicit KST offset: YYYY-MM-DDTHH:mm:ss.sss+09:00 (%s is the millisecond field) */
inline auto ToIso8601Kst(const FDateTime &Kst) -> FString
{
    return Kst.ToString(TEXT("%Y-%m-%dT%H:%M:%S.%s")) + TEXT("+09:00");
}

/** Compact stamp for file names: yyyymmdd_hhmmss */
inline auto ToFileStampKst(const FDateTime &Kst) -> FString
{
    return Kst.ToString(TEXT("%Y%m%d_%H%M%S"));
}

/** Readable stamp for log lines: yyyy-mm-dd hh:mm:ss.mmm */
inline auto ToLogStampKst(const FDateTime &Kst) -> FString
{
    return Kst.ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
}
} // namespace ScenarioTime
