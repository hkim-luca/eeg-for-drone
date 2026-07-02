// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DroneSim : ModuleRules
{
	public DroneSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DroneSim",
			"DroneSim/Variant_Platforming",
			"DroneSim/Variant_Platforming/Animation",
			"DroneSim/Variant_Combat",
			"DroneSim/Variant_Combat/AI",
			"DroneSim/Variant_Combat/Animation",
			"DroneSim/Variant_Combat/Gameplay",
			"DroneSim/Variant_Combat/Interfaces",
			"DroneSim/Variant_Combat/UI",
			"DroneSim/Variant_SideScrolling",
			"DroneSim/Variant_SideScrolling/AI",
			"DroneSim/Variant_SideScrolling/Gameplay",
			"DroneSim/Variant_SideScrolling/Interfaces",
			"DroneSim/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
