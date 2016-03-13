// Copyright 2015 Opaque Multimedia Pty. Ltd. All Rights Reserved

#pragma once
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

#include "TangoDataTypes.h"
#include "Components/SceneComponent.h"
#include "TangoMotionComponent.generated.h"
class UTangoImageComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTangoPoseAvailable, FTangoPoseData, TangoPoseData, FTangoCoordinateFramePair, TangoCoordinateFramePair);

UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class PROJECTTANGOPLUGIN_API UTangoMotionComponent : public USceneComponent
{
	GENERATED_BODY()

	UTangoMotionComponent();
	virtual void BeginDestroy() override;
	virtual void InitializeComponent() override;

public:

	//The BlueprintAssignable function delegate, called every time the Tango's 'OnPoseAvailable' callback occurs
	UPROPERTY(BlueprintAssignable)
		FOnTangoPoseAvailable OnTangoPoseAvailable;

	//The Tango Image Component associated with this motion component for AR synchronization, if any
	UPROPERTY()
		UTangoImageComponent* ARImage;

	//The Frame of Reference which will drive the position and rotation of this component.
	UPROPERTY(Category = "Tango|Motion", meta = (ToolTip = "The Frame of Reference which will drive the position and rotation of this component.", keyword = "motion, frame, coordinate pair, frame of reference, position, rotation", ExposeOnSpawn), BlueprintReadWrite, EditAnywhere)
		FTangoCoordinateFramePair MotionComponentFrameOfReference;

	/**
	* When called, and 'Enable' is set to true, synchronize this Motion component with a given Image component's update to provide better AR results.
	* Please note: when using this function, please disable 'Smooth Pose' in the Configuration options passed into "Connect Tango service",
	* as having this enabled can cause 'sloshing' or desynchronization between the image and the motion component.
	* @param Target The Unreal Engine / Tango Motion Component interface object.
	* @param Enable Whether this Motion component should run in ARMode and synchronize with a Tango Image component.
	* @param ImageComponent  The image component to synchronise this motion component's position output with.
	*/
	UFUNCTION(Category = "Tango|Motion", meta = (ToolTip = "Aligns the tracking with the ColorCamera", keyword = "motion, alternate reality, ar, position, camera"), BlueprintCallable)
		void RunInARMode(bool Enable, UTangoImageComponent* ImageComponent);

	/*
	* Sets the pose events that are received by this component. If called twice, the second FramePair parameter will overwrite the first.
	*	@param FramePairs An array of the FramePairs to recieve the matching events for.
	*/
	UFUNCTION(Category = "Tango|Motion", meta = (ToolTip = "Sets the pose events that are received by this component", keyword = "motion, pose, events, event"), BlueprintCallable)
		void SetupPoseEvents(TArray<FTangoCoordinateFramePair> FramePairs);

	/*
	*	Returns a Tango pose object for the given time relative to a specific frame of reference.
	* @param Target The Unreal Engine / Tango Area Learning interface object.
	* @param Specifies the frame of reference and target frame of reference.
	*	@param Timestamp The timestamp for which the Tango pose data should be retrieved.
	*	A common use case is to take a timestamp returned from the event of another component (such as depth or image) to get the closest matching pose to that particular event.
	* @return TangoPoseData A Tango pose object which most closely matches the input timestamp.
	*/
	UFUNCTION(Category = "Tango|Motion", meta = (ToolTip = "Returns the Tango pose object for the given time.", keyword = "motion, time, timestamp, pose"), BlueprintPure)
		FTangoPoseData GetTangoPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp);

	/*
	*	Returns the status of the pose information returned by the Tango Device.
	* @param Timestamp The function will return the pose status of the pose which most closely matches this timestamp.
	* @return The enum value which describes the current status of this pose- either Valid, Invalid, Initializing or Unknown.
	*/
	UFUNCTION(Category = "Tango|Motion", BlueprintPure, meta = (ToolTip = "Returns the status of the pose information returned by the Tango Device", keyword = "motion, rotation, rotator, offset"))
		TEnumAsByte<ETangoPoseStatus::Type> GetTangoPoseStatus(float& Timestamp);

	/*
	*	Resets the motion tracking of the tango device. This function is equivalent to the TangoService_ResetMotionTracking function.
	* @param Target The Unreal Engine / Tango Motion interface object.
	*/
	UFUNCTION(Category = "Tango|Motion", meta = (ToolTip = "Resets the motion of the Tango device.", keyword = "motion, reset, tracking"), BlueprintCallable)
		void ResetMotionTracking();

	/*
	*	Returns true if the Tango is localized relative to the ADF file passed in as an argument in the Tango Config object when connecting to the Tango service.
	* @param Target The Unreal Engine / Tango Motion interface object.
	* @return Returns true if the Tango is currently localized relative to the loaded ADF.
	*/
	UFUNCTION(Category = "Tango|Motion", BlueprintPure, meta = (ToolTip = "Returns true if the device pose is localised to the loaded area description specified in the Config (if any).", keyword = "motion, localized, localised, area description"))
		bool IsLocalized();

private:
	UFUNCTION()
		void LateUpdate();

	float LatestPoseTimeStamp;

	FVector UpdateLocation;
	FRotator UpdateRotation;
	FDelegateHandle LateUpdateHandle;
};

