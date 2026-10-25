using System.IO;
using System.Collections.Generic;
using UnrealBuildTool;

public class DroneSim : ModuleRules
{
	public DroneSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// BrainFlow's C++ API throws BrainFlowException; the EEG source catches it.
		bEnableExceptions = true;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"Json",
			"Sockets",
			"Networking"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DroneSim",
			"DroneSim/Scenario",
			"DroneSim/Eeg"
		});

		SetupBrainFlow(Target);
		StageChannelMap(Target);
	}

	// Ships the editable OpenBCI channel map as a loose file at <ProjectDir>/mappings in
	// packaged builds; the editor reads it from the repository root instead.
	private void StageChannelMap(ReadOnlyTargetRules Target)
	{
		string MapFile = Path.Combine(ModuleDirectory, "..", "..", "..", "mappings", "openbci_to_sim.json");
		if (Target.Type != TargetType.Editor && File.Exists(MapFile))
		{
			RuntimeDependencies.Add("$(ProjectDir)/mappings/openbci_to_sim.json", MapFile, StagedFileType.NonUFS);
		}
	}

	// Links the BrainFlow C++ SDK for OpenBCI acquisition. Drop the SDK's compiled C++
	// package under DroneSim/ThirdParty/BrainFlow:
	//   ThirdParty/BrainFlow/inc/*.h              (board_shim.h, brainflow_constants.h, ...)
	//   ThirdParty/BrainFlow/Win64/*.lib          (BoardController.lib, DataHandler.lib, ...)
	//   ThirdParty/BrainFlow/Win64/*.dll          (BoardController.dll + its dependency DLLs)
	// The DLLs must also be reachable at runtime (staged for packaged builds below; for
	// in-editor PIE, keep them next to UnrealEditor.exe or on PATH).
	private void SetupBrainFlow(ReadOnlyTargetRules Target)
	{
		string Root = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "BrainFlow");
		string IncDir = Path.Combine(Root, "inc");
		if (!Directory.Exists(IncDir))
		{
			// SDK not present yet: the project still compiles, but EegOpenBciSource is
			// compiled out (see WITH_BRAINFLOW) so there is no link dependency.
			PublicDefinitions.Add("WITH_BRAINFLOW=0");
			return;
		}

		PublicDefinitions.Add("WITH_BRAINFLOW=1");
		PublicSystemIncludePaths.Add(IncDir);

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string BinDir = Path.Combine(Root, "Win64");
			foreach (string Lib in Directory.GetFiles(BinDir, "*.lib"))
			{
				PublicAdditionalLibraries.Add(Lib);
			}
			foreach (string Dll in Directory.GetFiles(BinDir, "*.dll"))
			{
				string Name = Path.GetFileName(Dll);
				// stage the DLL next to the packaged executable
				RuntimeDependencies.Add("$(BinaryOutputDir)/" + Name, Dll);
			}
		}
	}
}
