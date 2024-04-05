// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PDOpenSource : ModuleRules
{
	public PDOpenSource(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
