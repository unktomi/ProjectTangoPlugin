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

#include "ProjectTangoPluginPrivatePCH.h"
#include "TangoDeviceAreaLearning.h"
#include "TangoDevice.h"
#include "stdlib.h"

//Required includes for making the calls to Java functions
#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

TangoDeviceAreaLearning::TangoDeviceAreaLearning()
{
}

TangoDeviceAreaLearning::~TangoDeviceAreaLearning()
{
}

//START - Tango Area Learning functions

bool TangoDeviceAreaLearning::DeleteAreaDescription(FString UUID)
{
	bool bIsDeleted = false;

#if PLATFORM_ANDROID
	std::string t = TCHAR_TO_UTF8(*UUID);
	const char* returnvalue = t.c_str();
	if (TangoService_deleteAreaDescription(returnvalue) == TANGO_SUCCESS)
	{
		bIsDeleted = true;
	}
#endif

	return bIsDeleted;
}

FTangoAreaDescription TangoDeviceAreaLearning::SaveCurrentArea(FString Filename, bool& bIsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveCurrentArea: Called"));
	FTangoAreaDescription SavedData;
	bIsSuccessful = false;

	if (!UTangoDevice::Get().IsLearningModeEnabled())
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" TangoDeviceAreaLearning::SaveCurrentArea: Attempted to save area description when learning mode is disabled!"));
	}
	else if (UTangoDevice::Get().getTangoDeviceMotionPointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" TangoDeviceAreaLearning::SaveCurrentArea: Attempted to save area description when tracking was not enabled!"));
	}
	else if (!UTangoDevice::Get().getTangoDeviceMotionPointer()->IsLocalized())
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" TangoDeviceAreaLearning::SaveCurrentArea: Attempted to save area description when not localized yet!"));
	}
	else
	{
#if PLATFORM_ANDROID
		TangoUUID UUID;
		TangoAreaDescriptionMetadata Metadata;
		const char* Key = "name"; //key for filename

		std::string Converted = TCHAR_TO_UTF8(*Filename);
		const char* Value = Converted.c_str();

		if (TangoService_saveAreaDescription(&UUID) != TANGO_SUCCESS)
		{
			UE_LOG(ProjectTangoPlugin, Warning, TEXT(" TangoDeviceAreaLearning::SaveCurrentArea: Save was not successful"));
		}
		else if (TangoService_getAreaDescriptionMetadata(UUID, &Metadata) != TANGO_SUCCESS
			|| TangoAreaDescriptionMetadata_set(Metadata, Key, Filename.Len(), Value) != TANGO_SUCCESS
			|| TangoService_saveAreaDescriptionMetadata(UUID, Metadata) != TANGO_SUCCESS)
		{
			UE_LOG(ProjectTangoPlugin, Warning, TEXT(" TangoDeviceAreaLearning::SaveCurrentArea: Save occurred but filename was not properly set for UUID: %s"), UUID);
		}
		else
		{
			bIsSuccessful = true;
			//CurrentADFFile = FString(UUID); //@TODO: Update config struct in TangoDevice?
			SavedData = FTangoAreaDescription(FString(UUID), Filename);
		}
#endif
	}

	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveCurrentArea: FINISHED"));
	return SavedData;
}

//END - Tango Area Learning functions

