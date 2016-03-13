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
#include "TangoDevice.h"
#include "TangoFromToCObject.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

FString UTangoDevice::GetLoadedAreaDescriptionUUID()
{
	return CurrentConfig.AreaDescription.UUID;
}

TArray<FString> UTangoDevice::GetAllUUIDs()
{
	TArray<FString> ReturnValues;
#if PLATFORM_ANDROID
	char* uuid;
	if (TangoService_getAreaDescriptionUUIDList(&uuid) == TANGO_SUCCESS)
	{
		FString(uuid).ParseIntoArray(ReturnValues, TEXT(","), true);
	}
#endif
	return ReturnValues;
}

TArray<FTangoAreaDescription> UTangoDevice::GetAreaDescriptions()
{
	TArray<FTangoAreaDescription> AreaDescriptions;
#if PLATFORM_ANDROID
	const char* key = "name";

	TArray<FString> UUIDs = GetAllUUIDs();
	for (int i = 0; i < UUIDs.Num(); i++)
	{
		std::string t = TCHAR_TO_UTF8(*UUIDs[i]);
		const char* uuid = t.c_str();

		TangoAreaDescriptionMetadata metadata;
		char* name;
		size_t size = 0;
		if (TangoService_getAreaDescriptionMetadata(uuid, &metadata) == TANGO_SUCCESS
			&& TangoAreaDescriptionMetadata_get(metadata, key, &size, &name) == TANGO_SUCCESS)
		{
			AreaDescriptions.Add(FTangoAreaDescription(UUIDs[i], FString(name)));
		}
	}
#endif
	return AreaDescriptions;
}

FTangoAreaDescriptionMetaData UTangoDevice::GetMetaData(FString UUID, bool& IsSuccessful)
{
	//Set up return variables
	FTangoAreaDescriptionMetaData Result = FTangoAreaDescriptionMetaData();

#if PLATFORM_ANDROID

	//Set up metadata C style structures
	TangoAreaDescriptionMetadata Metadata;

	//Set up Unreal style structures

	//Prepare arguments
	std::string ConvertedUUIDKey = TCHAR_TO_UTF8(*UUID);
	const char* ConvertedUUIDKeyCString = ConvertedUUIDKey.c_str();

	std::string ConvertedNameKey = "name";
	const char* ConvertedNameKeyCString = ConvertedNameKey.c_str();
	size_t NameValueSize;
	char* NameValue;

	std::string ConvertedTransformationKey = "transformation";
	const char* ConvertedTransformationKeyCString = ConvertedTransformationKey.c_str();
	size_t TransformValueSize;
	double TransformValue[7] = { 0, 0, 0, 0, 0, 0, 1 };
	char* TransformValuePointer = reinterpret_cast<char*>(&TransformValue);

	std::string ConvertedDateMsSinceEpochKey = "date_ms_since_epoch";
	const char* ConvertedDateMsSinceEpochKeyCString = ConvertedDateMsSinceEpochKey.c_str();
	size_t DateMsSinceEpochValueSize;
	uint64 DateMsSinceEpochValue;

	char* DatePointer = reinterpret_cast<char*>(&DateMsSinceEpochValue);

	//Poll service & populate metadata pointer
	if (TangoService_getAreaDescriptionMetadata(ConvertedUUIDKeyCString, &Metadata) != TANGO_SUCCESS)
	{
		IsSuccessful = false;
		UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Metadata object."));
		return FTangoAreaDescriptionMetaData();
	}

	//Poll service for metadata properties
	if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedNameKeyCString, &NameValueSize, &NameValue) == TANGO_SUCCESS)
	{
		if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedTransformationKeyCString, &TransformValueSize, &TransformValuePointer) == TANGO_SUCCESS)
		{
			if (TangoAreaDescriptionMetadata_get(Metadata, ConvertedDateMsSinceEpochKeyCString, &DateMsSinceEpochValueSize, &DatePointer) == TANGO_SUCCESS)
			{
				//We read everything OK, now read in the metadata
				Result.Filename = FString(NameValue);

				double* TVP = reinterpret_cast<double*>(TransformValuePointer);
				//Read in transformation
				Result.TransformationX = TVP[0];
				Result.TransformationY = TVP[1];
				Result.TransformationZ = TVP[2];
				Result.TransformationQX = TVP[3];
				Result.TransformationQY = TVP[4];
				Result.TransformationQZ = TVP[5];
				Result.TransformationQW = TVP[6];

				//Read in milliseconds since epoch
				uint64 NativeInteger = 0;
				NativeInteger = *(reinterpret_cast<uint64*>(DatePointer));
				int32 ConvertedInteger = static_cast<int32>(NativeInteger);
				Result.MillisecondsSinceUnixEpoch = ConvertedInteger;
				IsSuccessful = true;
			}
			else
			{
				//Result was not successful!
				UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Date."));
				IsSuccessful = false;
			}
		}
		else
		{
			//Result was not successful!
			UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Transformation."));
			IsSuccessful = false;
		}
	}
	else
	{
		//Result was not successful!
		UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDeviceAreaLearning::GetMetaData: Could not get Name string."));

		IsSuccessful = false;
	}

#endif
	return Result;
}

void UTangoDevice::ImportCurrentArea(FString Filepath, bool&IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ImportCurrentArea: Starting import"));

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		//Prepare the arguments
		jstring TangoImportFilenameArg = Env->NewStringUTF(TCHAR_TO_UTF8(*Filepath));

		static jmethodID ADFImportIntentMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestImportPermission", "(Ljava/lang/String;)V", false);
		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ADFImportIntentMethod, TangoImportFilenameArg);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ImportCurrentArea: Error: Could not get Java environment!"));
	}

#endif

	return;

}

void UTangoDevice::ExportCurrentArea(FString UUID, FString Filepath, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Starting export"));
	//Make the call to Java

#if PLATFORM_ANDROID
	jobject AppContext = NULL;

	if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
	{
		//Prepare the arguments
		jstring TangoExportUUIDArg = Env->NewStringUTF(TCHAR_TO_UTF8(*UUID));
		jstring TangoExportFilenameArg = Env->NewStringUTF(TCHAR_TO_UTF8(*Filepath));

		static jmethodID ADFExportIntentMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestExportPermission", "(Ljava/lang/String;Ljava/lang/String;)V", false);

		FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, ADFExportIntentMethod, TangoExportUUIDArg, TangoExportFilenameArg);

	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Error: Could not get Java environment!"));
	}
#endif
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::ExportCurrentArea: Finished export"));
}