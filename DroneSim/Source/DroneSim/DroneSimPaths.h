#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"

/** User-editable settings folder: <package>/config when packaged, release/config of the repo in the editor */
inline auto DroneSimConfigDir() -> FString
{
#if WITH_EDITOR
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../release/config"));
#else
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../config"));
#endif
}

/** Scenario recording output: <package>/Recordings when packaged, Saved/Recordings in the editor */
inline auto DroneSimRecordingDir() -> FString
{
#if WITH_EDITOR
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Recordings"));
#else
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("../Recordings"));
#endif
}
