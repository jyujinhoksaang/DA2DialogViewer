// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DA2DialogViewer : ModuleRules
{
	public DA2DialogViewer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"InputCore",
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"Projects",
				"XmlParser",
				"AudioMixer"
			}
		);
	}
}
