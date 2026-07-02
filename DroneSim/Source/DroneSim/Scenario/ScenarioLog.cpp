#include "ScenarioLog.h"
#include "DroneSim.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CoreDelegates.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ScenarioTime.h"

auto FScenarioLog::GetLogFilePath() -> const FString &
{
    static const FString FilePath = []() -> auto {
        const FString FileName =
            FString::Printf(TEXT("Scenario_%s.log"), *ScenarioTime::ToFileStampKst(ScenarioTime::NowKst()));
        const FString Path =
            FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("ScenarioLogs") / FileName);
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
        return Path;
    }();
    return FilePath;
}

void FScenarioLog::Write(const TCHAR *Severity, const FString &Message)
{
    const FString Line = FString::Printf(TEXT("[%s][%s] %s") LINE_TERMINATOR,
                                         *ScenarioTime::ToLogStampKst(ScenarioTime::NowKst()), Severity, *Message);
    FFileHelper::SaveStringToFile(Line, *GetLogFilePath(), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
                                  &IFileManager::Get(), FILEWRITE_Append);
}

void FScenarioLog::Info(const FString &Message)
{
    Write(TEXT("INFO"), Message);
    UE_LOG(LogDroneSim, Log, TEXT("%s"), *Message);
}

void FScenarioLog::Error(const FString &Message)
{
    Write(TEXT("ERROR"), Message);
    UE_LOG(LogDroneSim, Error, TEXT("%s"), *Message);
}

auto FScenarioLog::SystemError() -> FString
{
    const uint32 ErrorCode = FPlatformMisc::GetLastError();
    TCHAR ErrorMessage[1024];
    FPlatformMisc::GetSystemErrorMessage(ErrorMessage, UE_ARRAY_COUNT(ErrorMessage), static_cast<int32>(ErrorCode));
    return FString::Printf(TEXT("%u: %s"), ErrorCode, ErrorMessage);
}

void FScenarioLog::LogSessionHeader(const UWorld *World)
{
    // a missing session-end line in the log means the process crashed or was killed
    static bool bExitHookRegistered = false;
    if (!bExitHookRegistered)
    {
        bExitHookRegistered = true;
        FCoreDelegates::OnPreExit.AddLambda([] { Info(TEXT("===== Session end (clean shutdown) =====")); });
    }

    Info(TEXT("===== DroneSim scenario session start ====="));
    Info(FString::Printf(TEXT("Build: %hs %hs | Engine: %s | OS: %s"), __DATE__, __TIME__,
                         *FEngineVersion::Current().ToString(), *FPlatformMisc::GetOSVersion()));
    if (World != nullptr)
    {
        Info(FString::Printf(TEXT("Map: %s"), *World->GetMapName()));
    }
    Info(FString::Printf(TEXT("Log file: %s"), *GetLogFilePath()));
}
