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
#include "TangoPluginPrivatePCH.h"
#include "TangoDataTypes.h"
#include "TangoDevice.h"

namespace TangoARHelpers
{
	namespace
	{
		bool bDataIsFilled = false;
		FTangoCameraIntrinsics ColorCameraIntrinsics;
		FMatrix ProjectionMatrix;
		FVector2D UVShift;
		FVector2D NearPlaneLowerLeft, NearPlaneUpperRight;
		FVector2D NearFarDistance;

		static FMatrix FrustumMatrix(float Left, float Right, float Bottom, float Top, float NearVal, float FarVal)
		{
			FMatrix Result;
			Result.SetIdentity();
			Result.M[0][0] = (2.0f * NearVal) / (Right - Left);
			Result.M[1][1] = (2.0f * NearVal) / (Top - Bottom);
			Result.M[2][0] = -(Right + Left) / (Right - Left);
			Result.M[2][1] = -(Top + Bottom) / (Top - Bottom);
			Result.M[2][2] = FarVal / (FarVal - NearVal);
			Result.M[2][3] = 1.0f;
			Result.M[3][2] = -(FarVal * NearVal) / (FarVal - NearVal);
			Result.M[3][3] = 0;

			return Result;
		}

		static void GetNearProjectionPlane(const FIntPoint ViewPortSize)
		{
			float WidthRatio = (float)ViewPortSize.X / (float)ColorCameraIntrinsics.Width;
			float HeightRatio = (float)ViewPortSize.Y / (float)ColorCameraIntrinsics.Height;

			float UOffset, VOffset;
			if (WidthRatio >= HeightRatio)
			{
				UOffset = 0;
				VOffset = (1 - (HeightRatio / WidthRatio)) / 2;
			}
			else
			{
				UOffset = (1 - (WidthRatio / HeightRatio)) / 2;
				VOffset = 0;
			}
			UVShift.X = UOffset;
			UVShift.Y = VOffset;
			UE_LOG(TangoPlugin, Log, TEXT("FTangoViewExtension::GetNearProjectionPlane: UVShift: %f %f"), UOffset, VOffset);
			UOffset = 0.05f;
			VOffset = 0.0f;

			float XScale = NearFarDistance.X / ColorCameraIntrinsics.Fx;
			float YScale = NearFarDistance.X / ColorCameraIntrinsics.Fy;

			NearPlaneLowerLeft.X = (-ColorCameraIntrinsics.Cx + (UOffset * ColorCameraIntrinsics.Width))*XScale;
			NearPlaneUpperRight.X = (ColorCameraIntrinsics.Width - ColorCameraIntrinsics.Cx - (UOffset * ColorCameraIntrinsics.Width))*XScale;
			// OpenGL coordinates has y pointing downwards so we negate this term.
			NearPlaneLowerLeft.Y = (-ColorCameraIntrinsics.Height + ColorCameraIntrinsics.Cy + (VOffset * ColorCameraIntrinsics.Height))*YScale;
			NearPlaneUpperRight.Y = (ColorCameraIntrinsics.Cy - (VOffset * ColorCameraIntrinsics.Height))*YScale;
		}

		static FMatrix GetProjectionMatrix()
		{
			FMatrix OffAxisProjectionMatrix = FrustumMatrix(NearPlaneLowerLeft.X, NearPlaneUpperRight.X, NearPlaneLowerLeft.Y, NearPlaneUpperRight.Y, NearFarDistance.X, NearFarDistance.Y);

			FMatrix MatFlipZ;
			MatFlipZ.SetIdentity();
			MatFlipZ.M[2][2] = -1.0f;
			MatFlipZ.M[3][2] = 1.0f;

			FMatrix result = OffAxisProjectionMatrix * MatFlipZ;
			result.M[2][2] = 0.0f;
			result.M[3][0] = 0.0f;
			result.M[3][1] = 0.0f;
			result *= 1.0f / result.M[0][0];
			result.M[3][2] = NearFarDistance.X;
			return result;
		}

		static bool FillARData()
		{
			if (bDataIsFilled)
			{
				return true;
			}
			ColorCameraIntrinsics = UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR);
			if (UTangoDevice::Get().IsTangoServiceRunning() && GEngine)
			{
				if (UTangoDevice::Get().GetTangoDeviceMotionPointer() && GEngine->GameViewport)
				{
					if (GEngine->GameViewport->Viewport)
					{
						NearFarDistance.X = GNearClippingPlane;
						NearFarDistance.Y = 12000.0f;

						GetNearProjectionPlane(GEngine->GameViewport->Viewport->GetSizeXY());

						ProjectionMatrix = GetProjectionMatrix();
						bDataIsFilled = true;
					}
				}
			}
			return false;
		}
	}

	static FMatrix GetUnadjustedProjectionMatrix()
	{
		FillARData();
		return FrustumMatrix(NearPlaneLowerLeft.X, NearPlaneUpperRight.X, NearPlaneLowerLeft.Y, NearPlaneUpperRight.Y, NearFarDistance.X, NearFarDistance.Y);
	}

	static FTangoCameraIntrinsics GetARCameraIntrinsics()
	{
		FillARData();
		return ColorCameraIntrinsics;
	}

	static FVector2D GetARUVShift()
	{
		FillARData();
		return UVShift;
	}

	static FMatrix GetARProjectionMatrix()
	{
		FillARData();
		return ProjectionMatrix;
	}

	static void GetNearPlane(FVector2D& LowerLeft,FVector2D& UpperRight,FVector2D& NearFarPlaneDistance)
	{
		FillARData();
		LowerLeft = NearPlaneLowerLeft;
		UpperRight = NearPlaneUpperRight;
		NearFarPlaneDistance = NearFarDistance;
	}

	static bool DataIsReady()
	{
		FillARData();
		return bDataIsFilled;
	}
}