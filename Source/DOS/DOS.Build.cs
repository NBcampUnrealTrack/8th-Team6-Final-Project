// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DOS : ModuleRules
{
	public DOS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets subfolder headers (Timeline/, Case/, Achievements/) include each other by
		// path relative to the module root, e.g. #include "Case/DOSCaseSubsystem.h".
		PublicIncludePaths.Add(ModuleDirectory);

		// DeveloperSettings backs UDOSAudioSettings' Project Settings page.
		// AIModule/NavigationSystem: ADOSScheduledCharacter가 웨이포인트까지 NavMesh 위를
		// 실제로 걸어간다 (보간 배치로는 벽을 통과해 미끄러진다).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "UMG", "DeveloperSettings", "AIModule", "NavigationSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
