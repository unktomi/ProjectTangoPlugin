/*Copyright 2016 Opaque Media Group

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

using UnrealBuildTool;
using System.IO;

public class ProjectTangoPlugin : ModuleRules
{
	public ProjectTangoPlugin(TargetInfo Target)
	{
		//Add this modules headers, and the Tango API headers as public includes
		PublicIncludePaths.AddRange(new string[] {
            "ProjectTangoPlugin/Public",
			Path.Combine(ModuleDirectory, "../../ThirdParty/Public")
		});

		//Add this modules source code
		PrivateIncludePaths.Add("ProjectTangoPlugin/Private");

		//Specify what Unreal 4 Modules the plug-in is dependent on
		PublicDependencyModuleNames.AddRange(new string[]
		{
				"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"ShaderCore",
			"RHI"
		}
		
			);
		PrivateDependencyModuleNames.AddRange(new string[] {
			"CoreUObject",
			"Engine",
			"RHI",
			"RenderCore",
            "Core"
        });

		//For adding settings to the Project Settings menu.
		PrivateIncludePathModuleNames.Add("Settings");

		//If we are building for android, specify extra rules for it to compile and run using the Tango API.
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch");
			AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(ModuleDirectory, "ProjectTangoPlugin_APL.xml")));
			PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "../../ThirdParty/libtango_client_api.so"));
		}
	}
}
