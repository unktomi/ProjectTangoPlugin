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

FTangoAreaDescription UTangoAreaLearningComponent::SaveCurrentArea(FString Filename, bool& IsSuccessful)
{
	if (UTangoDevice::Get().getTangoDeviceAreaLearningPointer())
	{
		return UTangoDevice::Get().getTangoDeviceAreaLearningPointer()->SaveCurrentArea(Filename, IsSuccessful);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::SaveCurrentArea: Tango Area Learning not enabled"));
		return FTangoAreaDescription();
	}

}

FTangoAreaDescriptionMetaData UTangoAreaLearningComponent::GetMetaData(FString UUID, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::GetMetaData: Starting metadata get."));

	return UTangoDevice::Get().GetMetaData(UUID, IsSuccessful);
}


void UTangoAreaLearningComponent::SaveMetaData(FString UUID, FTangoAreaDescriptionMetaData NewMetadata, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::SaveMetaData: Starting metadata save."));

	if (UTangoDevice::Get().getTangoDeviceAreaLearningPointer())
	{
		UTangoDevice::Get().getTangoDeviceAreaLearningPointer()->SaveMetaData(UUID, NewMetadata, IsSuccessful);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::SaveMetaData: Tango Area Learning pointer not present."))
		IsSuccessful = false;
	}
	return;
}

void UTangoAreaLearningComponent::ImportADF(FString Filepath, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::ImportADF: Starting Area import."));

	UTangoDevice::Get().ImportCurrentArea(Filepath, IsSuccessful);
}

void UTangoAreaLearningComponent::ExportADF(FString UUID, FString Filepath, bool& IsSuccessful)
{
	UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::ExportADF: Starting area export."));

	UTangoDevice::Get().ExportCurrentArea(UUID, Filepath, IsSuccessful);
}
