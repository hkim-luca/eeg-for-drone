#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 *  File logger for external deployments. Writes timestamped (KST) lines to
 *  Saved/ScenarioLogs/Scenario_<session start>.log, flushing per line so the
 *  log survives crashes. Works in Shipping builds where UE logging is disabled.
 *  Every entry is mirrored to the LogDroneSim UE log category.
 *  Game-thread only.
 */
class FScenarioLog
{
public:
	/** Appends an INFO line */
	static void Info(const FString& Message);

	/** Appends an ERROR line */
	static void Error(const FString& Message);

	/** Logs build/engine/OS/map information; call once at session start */
	static void LogSessionHeader(const UWorld* World);

	/** Returns the last OS error as "code: description" for error context */
	static auto SystemError() -> FString;

private:
	static void Write(const TCHAR* Severity, const FString& Message);
	static auto GetLogFilePath() -> const FString&;
};
