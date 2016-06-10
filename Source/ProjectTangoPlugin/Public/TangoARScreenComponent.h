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

#include "Components/StaticMeshComponent.h"
#include "TangoARScreenComponent.generated.h"

UCLASS(ClassGroup = Tango, meta = (BlueprintSpawnableComponent))
class PROJECTTANGOPLUGIN_API UTangoARScreenComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UTangoARScreenComponent(const class FObjectInitializer& ObjectInitializer);

	/*
	*	Get Y component of camera texture view. Also returns a timestamp of when the image was captured and whether the result image is valid.
	* @param Target The Unreal Engine / Tango ARScreen component.
	* @param Timestamp The seconds since tango service was started, when this texture was generated.
	* @param IsValid Returns true if the resulting image is valid.
	* @return Returns a texture reference of the current RGB output from the camera.
	*/
	UFUNCTION(Category = "Tango|Image", meta = (ToolTip = "Returns the timestamp of the latest camera passthrough imgae", keyword = "image, AR, augmented reality"), BluePrintPure)
		float GetLatestImageTimeStamp();
protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
private:
	void SetupMaterial();

	bool bInitializedMaterial = false;

	UStaticMesh* FoundMesh;
	UMaterial* FoundMaterial;
};
