// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CH6_Shop_Simulator : ModuleRules
{
	public CH6_Shop_Simulator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicIncludePaths.Add(ModuleDirectory);

        PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
            "AIModule",
            "NavigationSystem",
			"Niagara",
		});

	}
}
