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

#pragma once

#include "Components/ActorComponent.h"
#include "TangoImageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTangoImageAvailable, float, Timestamp);

USTRUCT()
struct FInitMaterialListEntry
{
	GENERATED_BODY()

	UPROPERTY()
		UMaterialInstanceDynamic* Instance;
	FName PackedYMaskTextureName;
	FName PackedUVMaskTextureName;
	FName MaterialVectorName;

	FInitMaterialListEntry(UMaterialInstanceDynamic* _Instance, FName _PackedYMaskTextureName, FName _PackedUVMaskTextureName, FName _MaterialVectorName)
		: Instance(_Instance), PackedYMaskTextureName(_PackedYMaskTextureName), PackedUVMaskTextureName(_PackedUVMaskTextureName), MaterialVectorName(_MaterialVectorName)
	{
	}

	FInitMaterialListEntry() {}
};


UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class PROJECTTANGOPLUGIN_API UTangoImageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable)
		FOnTangoImageAvailable OnTangoImageAvailable;

	UTangoImageComponent(const class FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/*
	*	Returns a float which represents the time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	* @param The Unreal Engine / Tango Image interface object.
	* @return The time (in seconds since the Tango service was started) when the latest Image pose was retrieved.
	*/
	UFUNCTION(Category = "Tango|Camera", BlueprintPure, meta = (ToolTip = "Get camera Y texture view. Also returns a Timestamp of when the image was captured and whether the result image is valid.", keyword = "image, camera, view, texture"))
		UTexture* GetCameraViewYTexture(float& Timestamp, bool& IsValid);

	/*
	*	Get UV component of camera texture view. Also returns a timestamp of when the image was captured and whether the result image is valid.
	* @param Target The Unreal Engine / Tango Image interface object.
	* @param Timestamp The seconds since tango service was started, when this texture was generated.
	* @param IsValid Returns true if the resulting image is valid.
	* @return Returns a texture reference of what the camera is currently seeing.
	*/
	UFUNCTION(Category = "Tango|Camera", BlueprintPure, meta = (ToolTip = "Get camera UV texture view. Also returns a Timestamp of when the image was captured and whether the result image is valid.", keyword = "image, camera, view, texture"))
		UTexture* GetCameraViewUVTexture(float& Timestamp, bool& IsValid);

	/*
	*	Prepares a Material for displaying the Color Camera view. Useful for using the PackedYUVToRGB Material node.
	* @param Target The Unreal Engine / Tango Image interface object.
	* @param Instance The Material Instance that should be updated.
	* @param YTextureName The name of the Material Texture Parameter of the YTexture.
	*	@param The name of the Material Texture Parameter of the UVTexture.
	* @param The name of the Material Parameter for the Camera view width in Pixels.
	* @return True when the Material was successfully prepared.
	*/
	UFUNCTION(Category = "Tango|Camera", BluePrintCallable, meta = (ToolTip = "Setup the AR Material for AR rendering.", keyword = "image, texture, material, camera, AR, alternate reality"))
		bool PrepareTangoCameraColorMaterial(UMaterialInstanceDynamic* Instance, FName PackedYMaskTextureName = FName("PackedYMaskTexture"), FName PackedUVMaskTextureName = FName("PackedUVMaskTexture"), FName MaterialVectorName = FName("CameraMaterialVector"));

	/*
	*	Get Y component of camera texture view. Also returns a timestamp of when the image was captured and whether the result image is valid.
	* @param Target The Unreal Engine / Tango Image interface object.
	* @param Timestamp The seconds since tango service was started, when this texture was generated.
	* @param IsValid Returns true if the resulting image is valid.
	* @return Returns a texture reference of the current RGB output from the camera.
	*/
	UFUNCTION(Category = "Tango|Camera", BluePrintPure, meta = (ToolTip = "Get the latest cameraimage timestamp.", keyword = "image, timestamp, time, seconds, camera"))
		float  GetLatestImageTimeStamp();
private:
	UPROPERTY()
		float LastBroadCastedTimestamp = 0;

	UPROPERTY()
		TArray<FInitMaterialListEntry> InitMaterialList;
	UFUNCTION()
		void UpdateTangoCameraMaterials();

};