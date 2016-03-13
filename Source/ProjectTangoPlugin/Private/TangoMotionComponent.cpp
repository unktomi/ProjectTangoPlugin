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
#include "TangoMotionComponent.h"
#include "TangoImageComponent.h"
#include "TangoDevice.h"

UTangoMotionComponent::UTangoMotionComponent() : Super()
{
	LatestPoseTimeStamp = 0.0;
	bWantsInitializeComponent = true;
	bWantsBeginPlay = false;
}

void UTangoMotionComponent::LateUpdate()
{
	if (UTangoDevice::Get().getTangoDeviceMotionPointer())
	{
		//Only move the component if the pose status is valid.
		FTangoPoseData LatestPose = UTangoDevice::Get().getTangoDeviceMotionPointer()->GetPoseAtTime(MotionComponentFrameOfReference, ARImage == nullptr ? 0 : ARImage->GetLatestImageTimeStamp());
		if (LatestPose.StatusCode == ETangoPoseStatus::VALID)
		{
			SetRelativeLocation(LatestPose.Position);
			SetRelativeRotation(FRotator(LatestPose.QuatRotation));
		}
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoMotionComponent::LateUpdate: Could not update transfrom because Tango Service is not connect or has motion tracking disabled!"));
	}
}

void UTangoMotionComponent::InitializeComponent()
{
	UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoMotionComponent::InitializeComponent: Called!"));
	Super::InitializeComponent();
	LateUpdateHandle = UTangoDevice::Get().ViewExtension.Get().OnPreRender().AddUFunction(this, FName(TEXT("LateUpdate")));
}

void UTangoMotionComponent::BeginDestroy()
{
	Super::BeginDestroy();
	UTangoDevice::Get().ViewExtension.Get().OnPreRender().Remove(LateUpdateHandle);
}

void UTangoMotionComponent::RunInARMode(bool Enable, UTangoImageComponent * ImageComponent)
{
	ARImage = Enable ? ImageComponent : nullptr;
}

void UTangoMotionComponent::SetupPoseEvents(TArray<FTangoCoordinateFramePair> FramePairs)
{
	UTangoDevice::Get().AddTangoMotionComponent(this, FramePairs);
}

FTangoPoseData UTangoMotionComponent::GetTangoPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp)
{
	return UTangoDevice::Get().getTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().getTangoDeviceMotionPointer()->GetPoseAtTime(FrameOfReference, Timestamp) : FTangoPoseData();
}

TEnumAsByte<ETangoPoseStatus::Type> UTangoMotionComponent::GetTangoPoseStatus(float& Timestamp)
{
	FTangoPoseData LatestPose = UTangoDevice::Get().getTangoDeviceMotionPointer() != nullptr ? UTangoDevice::Get().getTangoDeviceMotionPointer()->GetPoseAtTime(MotionComponentFrameOfReference, 0) : FTangoPoseData();
	Timestamp = LatestPose.Timestamp;
	return LatestPose.StatusCode;
}

void UTangoMotionComponent::ResetMotionTracking()
{
	if(UTangoDevice::Get().getTangoDeviceMotionPointer() != nullptr)
		UTangoDevice::Get().getTangoDeviceMotionPointer()->ResetMotionTracking();
}

bool UTangoMotionComponent::IsLocalized()
{
	if (UTangoDevice::Get().getTangoDeviceMotionPointer())
	{
		return UTangoDevice::Get().getTangoDeviceMotionPointer()->IsLocalized();
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoAreaLearningComponent::IsLocalized: Tango Motion tracking not enabled"));
		return false;
	}
}