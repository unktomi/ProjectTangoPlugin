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
#include "TangoAreaLearningComponent.h"
#include "TangoDevice.h"

UTangoAreaLearningComponent::UTangoAreaLearningComponent(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

bool UTangoAreaLearningComponent::DeleteAreaDescription(FString UUID)
{
	if (UTangoDevice::Get().getTangoDeviceAreaLearningPointer())
	{
		return UTangoDevice::Get().getTangoDeviceAreaLearningPointer()->DeleteAreaDescription(UUID);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::DeleteAreaDescription: Tango Area Learning not enabled"));
		return false;
	}
}

bool UTangoAreaLearningComponent::IsLearningModeEnabled()
{
	return UTangoDevice::Get().IsLearningModeEnabled();
}

FTangoAreaDescription UTangoAreaLearningComponent::SaveCurrentArea(FString Filename, bool& bIsSuccessful)
{
	if (UTangoDevice::Get().getTangoDeviceAreaLearningPointer())
	{
     	return UTangoDevice::Get().getTangoDeviceAreaLearningPointer()->SaveCurrentArea(Filename, bIsSuccessful);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::SaveCurrentArea: Tango Area Learning not enabled"));
        bIsSuccessful = false;
		return FTangoAreaDescription();
	}

}

FTangoAreaDescriptionMetaData UTangoAreaLearningComponent::GetMetaData(FTangoAreaDescription AreaDescription, bool& bIsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoAreaLearningComponent::GetMetaData: Starting metadata get."));

	return UTangoDevice::Get().GetMetaData(AreaDescription.UUID, bIsSuccessful);
}


void UTangoAreaLearningComponent::SaveMetaData(FTangoAreaDescription AreaDescription, FTangoAreaDescriptionMetaData NewMetadata, bool& bIsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoAreaLearningComponent::SaveMetaData: Starting metadata save."));
    UTangoDevice::Get().SaveMetaData(AreaDescription.UUID, NewMetadata, bIsSuccessful);
	return;
}

void UTangoAreaLearningComponent::ImportADF(FString Filepath, bool& bIsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoAreaLearningComponent::ImportADF: Starting Area import."));

	UTangoDevice::Get().ImportCurrentArea(Filepath, bIsSuccessful);
}

void UTangoAreaLearningComponent::ExportADF(FString UUID, FString Filepath, bool& bIsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoAreaLearningComponent::ExportADF: Starting area export."));

	UTangoDevice::Get().ExportCurrentArea(UUID, Filepath, bIsSuccessful);
}
