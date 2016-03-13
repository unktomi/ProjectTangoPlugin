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
#include "TangoImageComponent.h"
#include "TangoDevice.h"
#include "TangoDataTypes.h"

UTangoImageComponent::UTangoImageComponent(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bWantsInitializeComponent = false;
	bWantsBeginPlay = false;
	PrimaryComponentTick.bCanEverTick = true;
}

void UTangoImageComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (UTangoDevice::Get().getTangoDeviceImagePointer())
	{
		if (UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture())
		{
			float TimeStamp = UTangoDevice::Get().getTangoDeviceImagePointer()->GetImageBufferTimestamp();
			if (TimeStamp != LastBroadCastedTimestamp)
			{
				LastBroadCastedTimestamp = TimeStamp;
				OnTangoImageAvailable.Broadcast(TimeStamp);
			}
		}
	}
	UpdateTangoCameraMaterials();
}

UTexture* UTangoImageComponent::GetCameraViewYTexture(float& Timestamp, bool& IsValid)
{
	UTexture * Texture = nullptr;
	IsValid = false;
	Timestamp = 0.0;

	if (UTangoDevice::Get().getTangoDeviceImagePointer())
	{
		Timestamp = UTangoDevice::Get().getTangoDeviceImagePointer()->GetImageBufferTimestamp();
		Texture = UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture();
		IsValid = Texture != nullptr;
	}

	return Texture;
}

UTexture* UTangoImageComponent::GetCameraViewUVTexture(float& Timestamp, bool& IsValid)
{
	UTexture * Texture = nullptr;
	IsValid = false;
	Timestamp = 0.0;

	if (UTangoDevice::Get().getTangoDeviceImagePointer())
	{
		Timestamp = UTangoDevice::Get().getTangoDeviceImagePointer()->GetImageBufferTimestamp();
		Texture = UTangoDevice::Get().getTangoDeviceImagePointer()->GetCrTexture();
		IsValid = Texture != nullptr;
	}
	
	return Texture;
}

void UTangoImageComponent::UpdateTangoCameraMaterials()
{
	if (InitMaterialList.Num() > 0 && UTangoDevice::Get().getTangoDeviceImagePointer() != nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Somethings in list and we have a pointer"));
		if (UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture() == nullptr)
		{
			UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Texture not initialized"));
			return;
		}

		if (UTangoDevice::Get().getTangoDeviceImagePointer()->GetCrTexture() == nullptr)
		{
			UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Texture not initialized"));
			return;
		}


		for (auto& Entry : InitMaterialList)
		{
			if (Entry.Instance != nullptr)
			{
				UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Found valid entry. Setting up material"));
				Entry.Instance->SetTextureParameterValue(Entry.PackedYMaskTextureName, UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture());
				Entry.Instance->SetTextureParameterValue(Entry.PackedUVMaskTextureName, UTangoDevice::Get().getTangoDeviceImagePointer()->GetCrTexture());
				Entry.Instance->SetVectorParameterValue(Entry.MaterialVectorName, FVector(0, 0, UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR).Width));
			}
			else
			{
				UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Found invalid entry. Ignoreing it"));
			}
		}
		InitMaterialList.Empty();
	}
	else if (InitMaterialList.Num() > 0)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoImageComponent::UpdateTangoCameraMaterials: Somethings in list and we DONT have a pointer"));
	}
}

bool UTangoImageComponent::PrepareTangoCameraColorMaterial(UMaterialInstanceDynamic* Instance, FName PackedYMaskTextureName, FName PackedUVMaskTextureName, FName MaterialVectorName)
{
	if (!UTangoDevice::Get().IsTangoServiceRunning())
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::PrepareTangoCameraColorMaterial: Could not prepare AR Material since TangoService is not connected"));
		return false;
	}
	else if (UTangoDevice::Get().getTangoDeviceImagePointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::PrepareTangoCameraColorMaterial: Could not prepare AR Material since Color Camera is not enabled"));
		return false;
	}
	else if (Instance == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::PrepareTangoCameraColorMaterial: Instance is nullptr"));
		return false;
	}
	else
	{
		InitMaterialList.Emplace(FInitMaterialListEntry(Instance, PackedYMaskTextureName, PackedUVMaskTextureName, MaterialVectorName));
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoImageComponent::PrepareTangoCameraColorMaterial: Called and working!"));
		return true;
	}
}

float UTangoImageComponent::GetLatestImageTimeStamp()
{
	if (UTangoDevice::Get().getTangoDeviceImagePointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoImageComponent::GetLatestImageTimeStamp: Color Camera is not enabled"));
		return 0.0f;
	}
	else
	{
		return UTangoDevice::Get().getTangoDeviceImagePointer()->GetImageBufferTimestamp();
	}
}
