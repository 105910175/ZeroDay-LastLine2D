// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool; // Unreal's build system types.

public class ZeroDayLastLine : ModuleRules // Build rules for the game module.
{
	public ZeroDayLastLine(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs; // Use precompiled headers to speed up builds.

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Paper2D", "PhysicsCore" }); // Paper2D draws the sprite art; PhysicsCore defines the physics material asset type.

		PrivateDependencyModuleNames.AddRange(new string[] {  }); // Nothing private is needed in every build.

		// Only the editor needs these: the asset generator commandlet uses them to turn the PNG art into
		// Unreal assets. They are left out of game builds so the packaged game stays small and portable.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AssetRegistry", "ImageWrapper" }); // Saving packages, registering assets, decoding PNGs.
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
