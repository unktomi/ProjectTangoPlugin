#pragma once
/*Copyright 2016 Google
Author: Opaque Media Group
 
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.*/

#include "SceneViewExtension.h"
#include "TangoDataTypes.h"
#include "ITangoAR.h"

/** View extension object that can persist on the render thread without the components */
class FTangoViewExtension : public ISceneViewExtension, public TSharedFromThis<FTangoViewExtension, ESPMode::ThreadSafe>
{
public:
	FTangoViewExtension(ITangoARInterface* MotionComponent) {  ARComponent = MotionComponent; CurrentStride = 0; }
	virtual ~FTangoViewExtension() {}

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {};

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override;
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override;

	ITangoARInterface* ARComponent;

private:


	/** Walks the component hierarchy gathering scene proxies */
	void GatherSceneProxiesAndCameras(USceneComponent* Component);
	/** Checks whether this SceneView has to be adjusted or not*/
	bool IdentifyViewWithCameraComponent(const FSceneView* InView,int32 Stride);
	const bool GetLateUpdateTransform(FTransform & Transform, int32 Stride, bool bIsAbsolute);
	//void MoveViewAndDoProjection(const FSceneView* InView, const FMatrix& LateUpdateTransform);

	int32 FindStride(int32 Time);
	//How many positions should be stored. Should be greater than what the Renderthread could ever lag behind.
#define MaxCameraStride 4
	int32 CurrentStride;
	uint32 FrameNumber[MaxCameraStride];
	/*
	*	Late update primitive info for accessing valid scene proxy info. From the time the info is gathered
	*  to the time it is later accessed the render proxy can be deleted. To ensure we only access a proxy that is
	*  still valid we cache the primitive's scene info AND a pointer to it's own cached index. If the primitive
	*  is deleted or removed from the scene then attempting to access it via it's index will result in a different
	*  scene info than the cached scene info.
	*/
	struct LateUpdatePrimitiveInfo
	{
		const int32*			IndexAddress;
		FPrimitiveSceneInfo*	SceneInfo;
	};

	TArray<LateUpdatePrimitiveInfo> LateUpdateSceneProxies[MaxCameraStride];
	/** Camera component positions and rotations since we have no other way to identify them :(*/

	struct CamData {
		FVector Pos;
		FRotator Rot;
		AActor* Actor;
		CamData(FVector P, FRotator R, AActor* A)
		{
			Pos = P;
			Rot = R;
			Actor = A;
		}
	};
	TArray<CamData> Cameras[MaxCameraStride];
	/** Poses buffered by GetLateUpdateTransform*/
	FTangoPoseData LatestPose;
	float LatestPoseStamp;
	FTangoPoseData Poses[MaxCameraStride];
	int32 PoseFrame[MaxCameraStride];
};