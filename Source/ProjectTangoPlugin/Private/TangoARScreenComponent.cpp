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
#include "TangoARScreenComponent.h"
#include "TangoDevice.h"


UTangoARScreenComponent::UTangoARScreenComponent(const class FObjectInitializer& ObjectInitializer)
{
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;
	auto MeshFinder = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/ProjectTangoPlugin/TangoPlane.TangoPlane'"));
	auto MaterialFinder = ConstructorHelpers::FObjectFinder<UMaterial>(TEXT("Material'/ProjectTangoPlugin/TangoCameraPassthroughMaterial.TangoCameraPassthroughMaterial'"));
	if (MeshFinder.Succeeded())
	{
		FoundMesh = MeshFinder.Object;
	}
	else
	{
		FoundMesh = nullptr;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoARScreenComponent::UTangoARScreenComponent: Unable to retrieve mesh from common folder!"));
	}
	if (MaterialFinder.Succeeded())
	{
		FoundMaterial = MaterialFinder.Object;
	}
	else
	{
		FoundMaterial = nullptr;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoARScreenComponent::UTangoARScreenComponent: Unable to retrieve material from common folder!"));
	}

}

float UTangoARScreenComponent::GetLatestImageTimeStamp()
{
	if (UTangoDevice::Get().getTangoDeviceImagePointer())
	{
		return UTangoDevice::Get().getTangoDeviceImagePointer()->GetImageBufferTimestamp();
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoARScreenComponent::GetLatestImageTimeStamp: Unable to retrieve timestamp!"));
		return 0.0f;
	}
}

void UTangoARScreenComponent::BeginPlay()
{
	Super::BeginPlay();
}

/*
 *  UTangoARScreenComponent::SetupMaterial()
 *  This function sets up the components of the ARCamera component to allow for the passthrough camera functionality to take place without the user needing to perform boilerplace setup.
 *  It references materials and meshes stored within the content folder of the plugin, and adds them to the scene in the correct arrangement in order to achieve the passthrough camera effect.
 */
void UTangoARScreenComponent::SetupMaterial()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoARScreenComponent::SetupMaterial: Called!"));
	if (StaticMesh == nullptr)
	{
		SetStaticMesh(FoundMesh);
		if (FoundMesh == nullptr)
		{
			UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoARScreenComponent::SetupMaterial: Mesh is null and unable to locate default mesh!"));
			return;
		}
	}


	if (FoundMaterial)
	{
		auto Inst = UMaterialInstanceDynamic::Create(FoundMaterial, this);

		Inst->SetTextureParameterValue(FName("PackedYMaskTexture"), UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture());
		Inst->SetTextureParameterValue(FName("PackedUVMaskTexture"), UTangoDevice::Get().getTangoDeviceImagePointer()->GetCrTexture());
		auto Intrin = UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR);
		Inst->SetVectorParameterValue(FName("CameraMaterialVector"), FLinearColor(0, 0, Intrin.Width, Intrin.Height));
		Inst->SetVectorParameterValue(FName("Intrinsics"), FLinearColor(Intrin.Cx, Intrin.Cy, Intrin.Fx, Intrin.Fy));
		if (Intrin.Distortion.Num() >= 3 || Intrin.CalibrationType != ETangoCalibrationType::POLYNOMIAL_3_PARAMETERS)
		{
			Inst->SetVectorParameterValue(FName("Distortion"), FLinearColor(Intrin.Distortion[0], Intrin.Distortion[1], Intrin.Distortion[2]));
		}
		else
		{
			UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoARScreenComponent::SetupMaterial: Unable to retrieve distortion values!"));
		}
		SetMaterial(0, Inst);
		bInitializedMaterial = true;
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoARScreenComponent::SetupMaterial: Success!"));
		return;
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoARScreenComponent::SetupMaterial: Material could not be located!"));
		return;
	}
}

void UTangoARScreenComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	if (!bInitializedMaterial)
	{
		if (UTangoDevice::Get().getTangoDeviceImagePointer())
		{
			if (UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture())
			{
				SetupMaterial();
			}
		}
	}
}