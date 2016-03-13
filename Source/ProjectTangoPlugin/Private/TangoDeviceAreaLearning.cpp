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
	bool IsDeleted = false;

#if PLATFORM_ANDROID
	std::string t = TCHAR_TO_UTF8(*UUID);
	const char* returnvalue = t.c_str();
	if (TangoService_deleteAreaDescription(returnvalue) == TANGO_SUCCESS)
	{
		IsDeleted = true;
	}
#endif

	return IsDeleted;
}

FTangoAreaDescription TangoDeviceAreaLearning::SaveCurrentArea(FString Filename, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveCurrentArea: Called"));
	FTangoAreaDescription SavedData;
	IsSuccessful = false;

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
			IsSuccessful = true;
			//CurrentADFFile = FString(UUID); //@TODO: Update config struct in TangoDevice?
			SavedData = FTangoAreaDescription(FString(UUID), Filename);
		}
#endif
	}

	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveCurrentArea: FINISHED"));
	return SavedData;
}

void TangoDeviceAreaLearning::SaveMetaData(FString UUID, FTangoAreaDescriptionMetaData NewMetadata, bool& IsSuccessful)
{
	IsSuccessful = false;

#if PLATFORM_ANDROID
	TangoAreaDescriptionMetadata Metadata;

	//Filename argument conversion
	const char* NameKey = "name"; //key for filename
	std::string ConvertedName = TCHAR_TO_UTF8(*(NewMetadata.Filename));
	const char* NameValue = ConvertedName.c_str();

	std::string ConvertedUUID = TCHAR_TO_UTF8(*UUID);
	const char* ConvertedUUIDValue = ConvertedUUID.c_str();

	const char* TransformationKey = "transformation";
	std::string ConvertedTransformationKey = TCHAR_TO_UTF8(TransformationKey);

	double TransformationElements[7] = { 0, 0, 0, 0, 0, 0, 1 };
	//@TODO: Replace these magic numbers (8: bytes per double value, 7: number of elements in array) with constant values
	uint TransformationArrayByteCount = (uint)(8 * 7);

	//Translation
	TransformationElements[0] = NewMetadata.TransformationX;
	TransformationElements[1] = NewMetadata.TransformationY;
	TransformationElements[2] = NewMetadata.TransformationZ;
	TransformationElements[3] = NewMetadata.TransformationQX;
	TransformationElements[4] = NewMetadata.TransformationQY;
	TransformationElements[5] = NewMetadata.TransformationQZ;
	TransformationElements[6] = NewMetadata.TransformationQW;

	if (TangoService_getAreaDescriptionMetadata(ConvertedUUIDValue, &Metadata) != TANGO_SUCCESS)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::SaveMetaData: Call to get MetaData pointer from Tango was not successful, could not save metadata!"));
		IsSuccessful = false;
		return;
	}
	
	if((TangoAreaDescriptionMetadata_set(Metadata, NameKey, (NewMetadata.Filename).Len(), NameValue) != TANGO_SUCCESS)
		|| (TangoAreaDescriptionMetadata_set(Metadata, TransformationKey, TransformationArrayByteCount, (char*)(TransformationElements)) != TANGO_SUCCESS)
		|| (TangoService_saveAreaDescriptionMetadata(ConvertedUUIDValue, Metadata) != TANGO_SUCCESS))
	{
		//Failed to save data!
		UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::SaveMetaData: Calls to Tango were not successful, could not save metadata!"));
		IsSuccessful = false;
	}
	else
	{
		IsSuccessful = true;
		//Saved data correctly.
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::SaveMetaData: successfully overwrote file metadata."));
	}
	TangoAreaDescriptionMetadata_free(Metadata);
#endif
}


//END - Tango Area Learning functions

