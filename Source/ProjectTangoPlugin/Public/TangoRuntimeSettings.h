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
#pragma once
#include "TangoRuntimeSettings.generated.h"

UCLASS(config=Engine, defaultconfig)
class PROJECTTANGOPLUGIN_API UTangoRuntimeSettings : public UObject
{
	GENERATED_UCLASS_BODY()

	//Enables usage of the Area Learning Features of the Plug-In by requesting the Intent for Area Learning from Android.
	UPROPERTY(EditAnywhere, config, Category = Tango, meta = (ToolTip = "Wether or not area learning is allowed or not. Will display an intend in the final app."))
		bool bTangoAreaLearningEnabled;
};