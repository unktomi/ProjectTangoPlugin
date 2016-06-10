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
#include "ProjectTangoPluginPrivatePCH.h"
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

		static FMatrix FrustumMatrix(float left, float right, float bottom, float top, float nearVal, float farVal)
		{
			FMatrix Result;
			Result.SetIdentity();
			Result.M[0][0] = (2.0f * nearVal) / (right - left);
			Result.M[1][1] = (2.0f * nearVal) / (top - bottom);
			Result.M[2][0] = -(right + left) / (right - left);
			Result.M[2][1] = -(top + bottom) / (top - bottom);
			Result.M[2][2] = farVal / (farVal - nearVal);
			Result.M[2][3] = 1.0f;
			Result.M[3][2] = -(farVal * nearVal) / (farVal - nearVal);
			Result.M[3][3] = 0;

			return Result;
		}

		static void GetNearProjectionPlane(const FIntPoint ViewPortSize)
		{
			float widthRatio = (float)ViewPortSize.X / (float)ColorCameraIntrinsics.Width;
			float heightRatio = (float)ViewPortSize.Y / (float)ColorCameraIntrinsics.Height;

			float uOffset, vOffset;
			if (widthRatio >= heightRatio)
			{
				uOffset = 0;
				vOffset = (1 - (heightRatio / widthRatio)) / 2;
			}
			else
			{
				uOffset = (1 - (widthRatio / heightRatio)) / 2;
				vOffset = 0;
			}
			UVShift.X = uOffset;
			UVShift.Y = vOffset;
			UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoViewExtension::GetNearProjectionPlane: UVShift: %f %f"), uOffset, vOffset);
			uOffset = 0.05f;
			vOffset = 0.0f;

			float Xscale = NearFarDistance.X / ColorCameraIntrinsics.Fx;
			float Yscale = NearFarDistance.X / ColorCameraIntrinsics.Fy;

			NearPlaneLowerLeft.X = (-ColorCameraIntrinsics.Cx + (uOffset * ColorCameraIntrinsics.Width))*Xscale;
			NearPlaneUpperRight.X = (ColorCameraIntrinsics.Width - ColorCameraIntrinsics.Cx - (uOffset * ColorCameraIntrinsics.Width))*Xscale;
			// OpenGL coordinates has y pointing downwards so we negate this term.
			NearPlaneLowerLeft.Y = (-ColorCameraIntrinsics.Height + ColorCameraIntrinsics.Cy + (vOffset * ColorCameraIntrinsics.Height))*Yscale;
			NearPlaneUpperRight.Y = (ColorCameraIntrinsics.Cy - (vOffset * ColorCameraIntrinsics.Height))*Yscale;
		}

		static FMatrix GetProjectionMatrix()
		{
			FMatrix OffAxisProjectionMatrix = FrustumMatrix(NearPlaneLowerLeft.X, NearPlaneUpperRight.X, NearPlaneLowerLeft.Y, NearPlaneUpperRight.Y, NearFarDistance.X, NearFarDistance.Y);

			FMatrix matFlipZ;
			matFlipZ.SetIdentity();
			matFlipZ.M[2][2] = -1.0f;
			matFlipZ.M[3][2] = 1.0f;

			FMatrix result = OffAxisProjectionMatrix * matFlipZ;
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
				if (UTangoDevice::Get().getTangoDeviceMotionPointer() && GEngine->GameViewport)
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