// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Example : ModuleRules
{
	public Example(ReadOnlyTargetRules Target) : base(Target)
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
			"HTTP",
			"Landscape",
			"Foliage",
			"MeshDescription",
			"StaticMeshDescription",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Example",
			"Example/Variant_Platforming",
			"Example/Variant_Platforming/Animation",
			"Example/Variant_Combat",
			"Example/Variant_Combat/AI",
			"Example/Variant_Combat/Animation",
			"Example/Variant_Combat/Gameplay",
			"Example/Variant_Combat/Interfaces",
			"Example/Variant_Combat/UI",
			"Example/Variant_SideScrolling",
			"Example/Variant_SideScrolling/AI",
			"Example/Variant_SideScrolling/Gameplay",
			"Example/Variant_SideScrolling/Interfaces",
			"Example/Variant_SideScrolling/UI",
            "ThirdParty",
            "ThirdParty/llama.cpp",
            "ThirdParty/llama.cpp/include",
            "ThirdParty/llama.cpp/ggml/include",
		});

        PublicDelayLoadDLLs.AddRange(new string[] {
            Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","bin","Release","llama.dll"),
            Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","bin","Release","ggml.dll"),
            Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","bin","Release","ggml-base.dll"),
            Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","bin","Release","ggml-cpu.dll")
        });

        PublicAdditionalLibraries.AddRange(new string[] {
			Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","src","Release","llama.lib"),
			Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","common","Release","common.lib"),
			Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","ggml","src","Release","ggml.lib"),
			Path.Combine(ModuleDirectory,"../ThirdParty","llama.cpp","build","ggml","src","Release","ggml-base.lib")
		});



        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsys		

        // Add any include paths 
        // Add any import libraries or static libraries
    }
}
