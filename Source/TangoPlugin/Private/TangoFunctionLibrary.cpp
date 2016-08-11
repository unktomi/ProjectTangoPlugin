/*Copyright 2016 Google
Author: Opaque Media Group
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

#include "TangoPluginPrivatePCH.h"
#include "TangoDevice.h"
#include "TangoFunctionLibrary.h"
#include "TangoDataTypes.h"

void UTangoFunctionLibrary::ConnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().StartTangoService(Configuration, RuntimeConfiguration);
}

void UTangoFunctionLibrary::DisconnectTangoService()
{
	UTangoDevice::Get().StopTangoService();
}

void UTangoFunctionLibrary::ReconnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().RestartService(Configuration, RuntimeConfiguration);
}

bool UTangoFunctionLibrary::IsTangoServiceRunning()
{
	return UTangoDevice::Get().IsTangoServiceRunning();
}

FTangoCameraIntrinsics UTangoFunctionLibrary::GetCameraIntrinsics(TEnumAsByte<ETangoCameraType::Type> CameraID)
{
	return UTangoDevice::Get().GetCameraIntrinsics(CameraID);
}

TArray<FTangoAreaDescription> UTangoFunctionLibrary::GetAllAreaDescriptionData()
{
	return UTangoDevice::Get().GetAreaDescriptions();
}

FTangoAreaDescription UTangoFunctionLibrary::GetLoadedAreaDescription()
{
	FTangoAreaDescription CurrentAreaDescription;

	TArray<FTangoAreaDescription> AreaDescriptions = GetAllAreaDescriptionData();
	FString LoadedUUID = UTangoDevice::Get().GetLoadedAreaDescriptionUUID();

	for (int i = 0; i < AreaDescriptions.Num(); i++)
	{
		if (AreaDescriptions[i].UUID == LoadedUUID)
		{
			CurrentAreaDescription = AreaDescriptions[i];
			break;
		}
	}

	return CurrentAreaDescription;
}

FTangoConfig UTangoFunctionLibrary::GetTangoConfig(FTangoRuntimeConfig& RuntimeConfig)
{
	RuntimeConfig = UTangoDevice::Get().GetCurrentRuntimeConfig();
	return UTangoDevice::Get().GetCurrentConfig();
}

bool UTangoFunctionLibrary::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration)
{
	return UTangoDevice::Get().SetTangoRuntimeConfig(Configuration);
}
