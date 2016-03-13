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
#include "TangoServiceSingleton.h"
#include "TangoDataTypes.h"

void UTangoServiceSingleton::ConnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().StartTangoService(Configuration, RuntimeConfiguration);
}

void UTangoServiceSingleton::DisconnectTangoService()
{
	UTangoDevice::Get().StopTangoService();
}

void UTangoServiceSingleton::ReconnectTangoService(FTangoConfig Configuration, FTangoRuntimeConfig RuntimeConfiguration)
{
	UTangoDevice::Get().RestartService(Configuration, RuntimeConfiguration);
}

bool UTangoServiceSingleton::IsTangoServiceRunning()
{
	return UTangoDevice::Get().IsTangoServiceRunning();
}

FTangoCameraIntrinsics UTangoServiceSingleton::GetCameraIntrinsics(TEnumAsByte<ETangoCameraType::Type> CameraID)
{
	return UTangoDevice::Get().GetCameraIntrinsics(CameraID);
}

TArray<FTangoAreaDescription> UTangoServiceSingleton::GetAllAreaDescriptionData()
{
	return UTangoDevice::Get().GetAreaDescriptions();
}

FTangoAreaDescription UTangoServiceSingleton::GetLoadedAreaDescription()
{
	FTangoAreaDescription CurrentAreaDescription;

	TArray<FTangoAreaDescription> AreaDescriptions = GetAllAreaDescriptionData();
	FString LoadedUUID = UTangoDevice::Get().GetLoadedAreaDescriptionUUID();

	for (int i = 0; i < AreaDescriptions.Num(); i++)
	{
		if (AreaDescriptions[i].UUID == LoadedUUID)
		{
			CurrentAreaDescription = AreaDescriptions[i];
			break;
		}
	}

	return CurrentAreaDescription;
}

FTangoAreaDescription UTangoServiceSingleton::GetLastAreaDesciption()
{
	return UTangoDevice::Get().GetAreaDescriptions().Top();
}

static void _Frustum(FMatrix& m,float left, float right, float bottom, float top, float zNear, float zFar)
{
	m.M[0][0] = 2.0f * zNear / (right - left);
	m.M[0][1] = 0.0;
	m.M[0][2] = (right + left) / (right - left);
	m.M[0][3] = 0.0;

	m.M[1][0] = 0.0;
	m.M[1][1] = 2.0f * zNear / (top - bottom);
	m.M[1][2] = (top + bottom) / (top - bottom);
	m.M[1][3] = 0.0;

	m.M[2][0] = 0.0;
	m.M[2][1] = 0.0;
	m.M[2][2] = -(zFar + zNear) / (zFar - zNear);
	m.M[2][3] = -(2 * zFar * zNear) / (zFar - zNear);

	m.M[3][0] = 0.0;
	m.M[3][1] = 0.0;
	m.M[3][2] = -1.0;
	m.M[3][3] = 0.0;
}

void UTangoServiceSingleton::PrepareCameraForAugmentedReality(UCameraComponent * Camera,float NearPlane,float FarPlane,APlayerController* Controller)
{
	auto Intrin = GetCameraIntrinsics(ETangoCameraType::COLOR);
	Camera->FieldOfView = FMath::RadiansToDegrees<float>(2.0f * FMath::Atan(0.5f * Intrin.Width / Intrin.Fx));
	//UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoServiceSingleton::PrepareCameraForAugmentedReality: Camera->FieldOfView %f "), Camera->FieldOfView);
	//return;

	//NearPlane = 10.0f;
	//FarPlane = 10000.0f;

	//ULocalPlayer* LocalPlayer = (ULocalPlayer*)Controller->Player;

	//if (LocalPlayer != NULL && LocalPlayer->ViewportClient != NULL && LocalPlayer->ViewportClient->Viewport != NULL)
	//{
	//	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
	//		LocalPlayer->ViewportClient->Viewport,
	//		Controller->GetWorld()->Scene,
	//		LocalPlayer->ViewportClient->EngineShowFlags).SetRealtimeUpdate(true));


	//	auto Intrin = GetCameraIntrinsics(ETangoCameraType::COLOR);

	//	FIntPoint ViewPortSize = LocalPlayer->ViewportClient->Viewport->GetSizeXY();
	//	float widthRatio = (float)ViewPortSize.X / (float)Intrin.Width;
	//	float heightRatio = (float)ViewPortSize.Y / (float)Intrin.Height;
	//	float uOffset, vOffset;
	//	if (widthRatio >= heightRatio)
	//	{
	//		uOffset = 0;
	//		vOffset = (1 - (heightRatio / widthRatio)) / 2;
	//	}
	//	else
	//	{
	//		uOffset = (1 - (widthRatio / heightRatio)) / 2;
	//		vOffset = 0;
	//	}

	//	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoServiceSingleton::PrepareCameraForAugmentedReality: uOffset %f vOffset %f"), uOffset, vOffset);

	//	float xscale = NearPlane / Intrin.Fx;
	//	float yscale = NearPlane / Intrin.Fy;

	//	float pixelLeft = -Intrin.Cx + (uOffset * Intrin.Width);
	//	float pixelRight = Intrin.Width - Intrin.Cx - (uOffset * Intrin.Width);

	//	float pixelBottom = -Intrin.Height + Intrin.Cy + (vOffset * Intrin.Height);
	//	float pixelTop = Intrin.Cy - (vOffset * Intrin.Height);

	//	FVector ViewLocation;
	//	FRotator ViewRotation;
	//	FSceneView* SceneView = LocalPlayer->CalcSceneView(&ViewFamily, ViewLocation, ViewRotation, LocalPlayer->ViewportClient->Viewport);

	//	//SceneView

	//	UE_LOG(ProjectTangoPlugin, Log, TEXT("Left %f Right %f Bottom %f Top %f xScale %f yScale %f Nearplane %f FarPlane %f"), pixelLeft, pixelRight, pixelBottom, pixelTop,xscale,yscale,NearPlane,FarPlane);
	//	_Frustum(SceneView->ViewMatrices.ProjMatrix,pixelLeft * xscale, pixelRight * xscale, pixelBottom * yscale, pixelTop * yscale, NearPlane, FarPlane);
	//	//SceneView->ViewMatrices.ProjMatrix.SetIdentity();
	//}
}


FTangoConfig UTangoServiceSingleton::GetTangoConfig(FTangoRuntimeConfig& RuntimeConfig)
{
	RuntimeConfig = UTangoDevice::Get().GetCurrentRuntimeConfig();
	return UTangoDevice::Get().GetCurrentConfig();
}

bool UTangoServiceSingleton::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration)
{
	return UTangoDevice::Get().SetTangoRuntimeConfig(Configuration);
}