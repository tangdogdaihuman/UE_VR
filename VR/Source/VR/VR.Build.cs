// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class VR : ModuleRules
{
	public VR(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd",
			"AssetTools",
			"Kismet",
			"KismetCompiler",
			"BlueprintGraph",
			"EditorScriptingUtilities"
		});

		PublicIncludePaths.AddRange(new string[] {
			"VR",
			"VR/Core",
			"VR/Interaction",
			"VR/Physics",
			"VR/UI",
			"VR/Scene",
			"VR/Level",
			"VR/Editor",
			"VR/Variant_Horror",
			"VR/Variant_Horror/UI",
			"VR/Variant_Shooter",
			"VR/Variant_Shooter/AI",
			"VR/Variant_Shooter/UI",
			"VR/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
