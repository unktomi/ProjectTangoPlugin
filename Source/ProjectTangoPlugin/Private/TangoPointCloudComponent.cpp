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
#include "TangoPointCloudComponent.h"

UTangoPointCloudComponent::UTangoPointCloudComponent(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bWantsBeginPlay = true;
}
UTangoPointCloudComponent::~UTangoPointCloudComponent()
{
}

void UTangoPointCloudComponent::BeginPlay()
{
	UTangoDevice::Get().PointCloudComponents.Add(this);
	Super::BeginPlay();
	InternalPointCloudContainer = NewObject<UPointCloudContainer>(this, TEXT("PointCloudContainer"));
	LatestDepthTimeStamp = 0.0;

}

float UTangoPointCloudComponent::GetCurrentWorldScaleFactor()
{
	return UTangoDevice::Get().GetMetersToWorldScale();
}

int32 UTangoPointCloudComponent::GetMaxPointCount()
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer() == nullptr)
	{
		return 0;
	}
	else
	{
		return UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetMaxVertexCapacity();
	}
}

FVector UTangoPointCloudComponent::GetSinglePoint(int32 Index, float & Timestamp, bool& bIsValidValue)
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer() != nullptr)
	{
		TArray<FVector>& PCloud = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud();
		bIsValidValue = PCloud.Num() > Index && Index >= 0;
		Timestamp = UTangoDevice::Get().getTangoDevicePointCloudPointer() != nullptr ? UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloudTimestamp() : 0;
		return bIsValidValue ? PCloud[Index] : FVector();
	}
	else
	{
		Timestamp = 0;
		bIsValidValue = false;
		return FVector();
	}
}

int32 UTangoPointCloudComponent::GetCurrentPointCount(float & Timestamp)
{
	Timestamp = UTangoDevice::Get().getTangoDevicePointCloudPointer() != nullptr ? UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloudTimestamp() : 0;
	return UTangoDevice::Get().getTangoDevicePointCloudPointer() != nullptr ? UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud().Num() : 0;
}

bool UTangoPointCloudComponent::FindClosestDepthPoint(UCameraComponent* ViewPoint, FVector2D ScreenPoint, FVector& Result, float& Timestamp, float MaxDistanceFromPoint)
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoPointCloudComponent::FindClosestDepthPoint: Could not execute since depth is not activated in tangoconfig!"));
		return false;
	}

	int32 BestIndex = -1;
	float BestDistanceSquared = 0;
	Timestamp = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloudTimestamp();
	TArray<FVector>& PointCloud = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud();

	for (int32 i = 0; i < PointCloud.Num(); ++i)
	{
		FVector2D ScreenPos = ProjectVectorToScreen(ViewPoint,PointCloud[i]);
		float DistanceSquared = FVector2D::DistSquared(ScreenPos, ScreenPoint);

		if (DistanceSquared > MaxDistanceFromPoint * MaxDistanceFromPoint)
		{
			continue;
		}
		if (BestIndex == -1 || DistanceSquared < BestDistanceSquared)
		{
			BestIndex = i;
			BestDistanceSquared = DistanceSquared;
		}
	}

	if (BestIndex >= 0)
	{
		Result = PointCloud[BestIndex];
	}
	return BestIndex != -1;
}

TArray<FVector> UTangoPointCloudComponent::GetAllDepthPointsInArea(UCameraComponent* ViewPoint, FVector2D ScreenPoint, float Range, float& Timestamp)
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoPointCloudComponent::GetAllDepthPointsInArea: Could not execute since depth is not activated in tangoconfig!"));
		return TArray<FVector>();
	}
	auto DepthPoints = TArray<FVector>();

	Timestamp = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloudTimestamp();
	TArray<FVector>& PointCloud = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud();
	for (int32 i = 0; i < PointCloud.Num(); ++i)
	{
		FVector2D ScreenPos = ProjectVectorToScreen(ViewPoint, PointCloud[i]);

		if (FVector2D::DistSquared(ScreenPos, ScreenPoint) < Range * Range)
		{
			DepthPoints.Add(PointCloud[i]);
		}
	}

	return DepthPoints;
}

bool UTangoPointCloudComponent::GetPlaneAtScreenCoordinates(UCameraComponent* ViewPoint, FVector2D ScreenPoint, float PointAreaRadius, float MinPercentage, float PointDistanceThreshold, FVector& PlaneCenter, FPlane& Plane, float& Timestamp)
{
	UWorld* World = GetWorld();
	ULocalPlayer* LocalPlayer = GEngine->GetLocalPlayerFromControllerId(World, 0);

	//@TODO: This is very bad!
	TArray<FVector> ClosestPoints = GetAllDepthPointsInArea(ViewPoint, ScreenPoint, PointAreaRadius, Timestamp);
	PlaneCenter = GetVectorArrayAverage(ClosestPoints);

	Plane = FPlane();

	if (ClosestPoints.Num() < 3)
	{
		return false;
	}

	// Max number of iterations
	int MaxIterations = 50;

	//Threshold to define if a point belongs to a plane or not
	//Distance in UE units from point to plane
	double Threshold = PointDistanceThreshold;

	int MaxFittedPoints = 0;
	double PercentageFitted = 0;
	int FittedPointsCount;

	//RANSAC algorithm to determine inliers
	for (int i = 0; i < MaxIterations; i++)
	{
		FPlane CandidatePlane = MakeRandomPlane(FVector::ForwardVector, ClosestPoints);
		FittedPointsCount = 0;

		//See for every point if it belongs to that Plane or not
		for (int j = 0; j < ClosestPoints.Num(); j++)
		{
			float DistToPlane = CandidatePlane.PlaneDot(ClosestPoints[j]);
			if (DistToPlane < Threshold)
			{
				FittedPointsCount++;
			}
		}

		if (FittedPointsCount > MaxFittedPoints)
		{
			MaxFittedPoints = FittedPointsCount;
			Plane = CandidatePlane;

			PercentageFitted = MaxFittedPoints / ClosestPoints.Num();
			if (PercentageFitted > MinPercentage)
			{
				break;
			}
		}
	}

	// If we couldn't reach the minimum points to be fitted with RANSAC, return false
	if (PercentageFitted < MinPercentage)
	{
		return false;
	}
	return true;
}

FPlane UTangoPointCloudComponent::MakeRandomPlane(FVector CameraForward, TArray<FVector> Points)
{
	if (Points.Num() < 3)
	{
		return FPlane();
	}

	// Choose 3 points randomly.
	int RandomPoint0 = FMath::RandRange(0, Points.Num() - 1);
	int RandomPoint1 = FMath::RandRange(0, Points.Num() - 2);

	// Make sure we handle collisions.
	if (RandomPoint1 == RandomPoint0)
	{
		RandomPoint1++;
	}
	else if (RandomPoint1 < RandomPoint0)
	{
		// We'll make sure to keep RandomPoint0 and RandomPoint1 in sorted order.
		int temp = RandomPoint0;
		RandomPoint0 = RandomPoint1;
		RandomPoint1 = temp;
	}

	int RandomPoint2 = FMath::RandRange(0, Points.Num() - 3);

	// Handle collisions.
	if (RandomPoint2 == RandomPoint0)
	{
		RandomPoint2++;
	}
	if (RandomPoint2 == RandomPoint1)
	{
		RandomPoint2++;
	}

	FVector Point0 = Points[RandomPoint0];
	FVector Point1 = Points[RandomPoint1];
	FVector Point2 = Points[RandomPoint2];

	// Define the plane
	FPlane Plane = FPlane(Point0, Point1, Point2);

	// Make sure that the normal of the plane points towards the camera.
	if (FVector::DotProduct(CameraForward, Plane.GetSafeNormal()) > 0)
	{
		Plane = Plane.Flip();
	}

	return Plane;
}

FVector2D UTangoPointCloudComponent::ProjectVectorToScreen(UCameraComponent* ViewPoint, FVector Location)
{

	static FVector2D ScreenDimensions;
	GEngine->GetLocalPlayerFromControllerId(GWorld, 0)->ViewportClient->GetViewportSize(ScreenDimensions);//@TODO: POSSIBLY WRONG ASSUMPTION: ID = 0

	float wU = FMath::Tan(FMath::DegreesToRadians<float>(ViewPoint->FieldOfView*0.5f)) * Location.X;
	float wV = FMath::Tan(FMath::DegreesToRadians<float>(ViewPoint->FieldOfView*0.5f / ViewPoint->AspectRatio)) * Location.X;

	float U =  Location.Y / wU;
	float V = -Location.Z / wV;

	FVector ReturnValue;
	ReturnValue.X = (ScreenDimensions.X / 2.f) + (U * (ScreenDimensions.X / 2.f));
	ReturnValue.Y = (ScreenDimensions.Y / 2.f) + (V * (ScreenDimensions.Y / 2.f));

	return FVector2D(ReturnValue);
}

FVector UTangoPointCloudComponent::GetVectorArrayAverage(const TArray<FVector>& Vectors)
{
	FVector Sum(0.f);
	FVector Average(0.f);

	if (Vectors.Num() > 0)
	{
		for (int32 VecIdx = 0; VecIdx < Vectors.Num(); VecIdx++)
		{
			Sum += Vectors[VecIdx];
		}

		Average = Sum / ((float)Vectors.Num());
	}

	return Average;
}

UPointCloudContainer* UTangoPointCloudComponent::PassPointCloudReferenceContainer()
{
	return InternalPointCloudContainer;
}

//////////////////////////////////// UPointCloudContainer class //////////////////////////////////////////////////////////////////////////////

UPointCloudContainer::UPointCloudContainer(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

const TArray<FVector>& UPointCloudContainer::GetPointCloudArray(float & Timestamp)
{
	//@TODO: Find out why the pointer to the TangoDevicePointCloud is failing
	TangoDevicePointCloud* TangoDevicePointCloud = UTangoDevice::Get().getTangoDevicePointCloudPointer();
	if (TangoDevicePointCloud == nullptr)
	{
		return DummyPointCloud;

	}
	else
	{
		return TangoDevicePointCloud->GetPointCloud();
	}
}
