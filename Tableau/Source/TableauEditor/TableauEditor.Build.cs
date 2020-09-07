
using UnrealBuildTool;

public class TableauEditor : ModuleRules
{
	public TableauEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "Projects"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{

                "TableauAsset",
                "ApplicationCore",
                "SceneOutliner"
            }
			);

        if(Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "AssetRegistry",
                    "AssetTools",
                    "BlueprintGraph",
                    "ContentBrowser",
                    "Core",
                    "CoreUObject",
                    "DesktopWidgets",
                    "EditorStyle",
                    "Engine",
                    "InputCore",
                    "LevelEditor",
                    "Projects",
                    "PropertyEditor",
                    "SceneOutliner",
                    "Slate",
                    "SlateCore",
                    "UnrealEd",
                    "RenderCore",
					"Foliage",
					"Json",
					"Landscape",
					"AdvancedPreviewScene",
					"RHI"
                }
            );
        }


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
