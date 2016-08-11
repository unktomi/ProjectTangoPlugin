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
#pragma once
#include "TangoDataTypes.h"
#include "TangoPointCloudComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTangoXYZijDataAvailable, float, TimeStamp);


UCLASS(ClassGroup = Tango, Blueprintable)
class TANGOPLUGIN_API UPointCloudContainer : public UObject
{
	GENERATED_BODY()
public:
	const TArray<FVector>& GetPointCloudArray(float& Timestamp);

private:
	//The Dummy Point cloud is returned in the event that the Point Cloud pointer is null.
	TArray<FVector> DummyPointCloud;
};

UENUM(BlueprintType)
namespace ETangoPointSpace
{
	enum Type
	{
		LOCAL UMETA(DisplayName = "Depth Space"),
		ADF_DEPTH UMETA(DisplayName = "ADF Space"),
		STARTOFSERVICE_DEPTH UMETA(DisplayName = "Start of Service Space")
	};
}

UCLASS(ClassGroup = Tango, Blueprintable, meta = (BlueprintSpawnableComponent))
class TANGOPLUGIN_API UTangoPointCloudComponent : public UActorComponent
{
	GENERATED_BODY()
	UTangoPointCloudComponent();

public:
	virtual void BeginPlay() override;

	//The BlueprintAssignable function delegate, called every time the Tango's 'OnPoseAvailable' callback occurs
	UPROPERTY(BlueprintAssignable, meta = (ToolTip = "Fires when new point cloud data was recieved"))
		FOnTangoXYZijDataAvailable OnTangoXYZijAvailable;

	/*
	* Get the max number of points a single frame from the point cloud can contain.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @return The maximum number of points the point cloud can contain.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get max number of points the point cloud can contain.", keyword = "depth, point cloud, max, number, buffer"), BlueprintPure)
		int32 GetMaxPointCount();

	/*
	* Retrieve a single point from the current point cloud.
	* @param Target The Unreal Engine / Tango Point Cloud Component interface object.
	* @param Index The index of the point within the point cloud.
	* @param Timestamp The seconds since tango service was started, when this point cloud was received.
	* @param ValidValue True when the point was successfully retrieved.
	* @return Point from the point cloud in Unreal co-ordinates.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get single point from cloud buffer.", keyword = "depth, point cloud, buffer"), BlueprintPure)
		FVector GetSinglePoint(int32 Index, ETangoPointSpace::Type OutputSpace,float& Timestamp,bool& bIsValidValue);

	/*
	* Retrieves the number of points contained within the latest frame from the point cloud.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @param Timestamp The seconds since tango service was started, when the last point cloud was received.
	* @return Amount of points in the current point cloud.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get current amount of points in cloud buffer.", keyword = "depth, point cloud, buffer"), BlueprintPure)
		int32 GetCurrentPointCount(float& Timestamp);

	/*
	* Retrieves the closest point to a specified position on the screen from within the point cloud buffer.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @param ScreenPoint Screen position in Pixels where the from the nearest depth point should be searched.
	* @param MaxDistancefromPoint Cutoff distance in Unreal units, points with a higher depth value are being ignored
	* @param Timestamp Timestamp of the returned point. Measured in seconds after Tango startup.
	* @return Returns closest depth point from the point cloud.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Retrieve the closest value to the input screen point from within the point cloud buffer.", keyword = "depth, point cloud, buffer, closest"), BlueprintPure)
		//FVector FindClosestDepthPoint(FVector2D ScreenPoint, float& Timestamp, float MaxDistanceFromPoint = 30);
		bool FindClosestDepthPoint(UCameraComponent* ViewPoint,FVector2D ScreenPoint, ETangoPointSpace::Type OutputSpace,FVector& Result, float& Timestamp, float MaxDistanceFromPoint = 30);

	/*
	* Get all points from the latest frame of the point cloud within the specified range of the input screen point.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @param ScreenPoint Point on the screen in pixels.
	*	@param Range Range in pixels around the screen point to return depth points from.
	* @param Timestamp The seconds since Tango Service was started, when this structure was generated.
	* @return Returns all points from the latest frame of the depth buffer within range of the designated screen point.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Get all the points in the point cloud buffer within the range of the input screen point.", keyword = "depth, point cloud, buffer"), BlueprintPure)
		TArray<FVector> GetAllDepthPointsInArea(UCameraComponent* ViewPoint, FVector2D ScreenPoint, float Range, ETangoPointSpace::Type OutputSpace, float& Timestamp);

	/*
	*Retrieve the plane average of the points within range of the input screen point.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @param ScreenPoint Point in screen-space pixels around which the attempt to find a plane will take place..
	*	@param PointAreaRadius Radius around the screen point in pixels in which to search for a plane.
	* @param MinPercentage Controls the sensitivity of the plane discorery algorithm.
	* @param PointDistanceThreshold Set the point distance threshold for the plane discovery algorithm.
	* @param PlaneCenter The center point on the plane in world space.
	* @param Plane A Plane structure in the format Ax + By + Cy + D
	* @param Timestamp The seconds since Tango Service was started, when this structure was generated.
	* @return Returns true if a plane was successfully retrieved.
	*/
	UFUNCTION(Category = "Tango|Depth", meta = (ToolTip = "Retrieve the plane average of the points within range of the input screen point."), BlueprintPure)
		bool GetPlaneAtScreenCoordinates(UCameraComponent* ViewPoint, FVector2D ScreenPoint, float PointAreaRadius, float MinPercentage, float PointDistanceThreshold, ETangoPointSpace::Type OutputSpace, FVector& PlaneCenter, FPlane& Plane, float& Timestamp);

	/*
	* Passes a container object for the point cloud data around. Useful to provide other C++ scripts access to the point cloud without any additional copies.
	* The reason this is useful is that all variable types except for UObjects are exposed to Blueprint by value-
	* to avoid additional deep copies of the entire point cloud, this instead passes an object which contains a pointer which can be accessed in C++ code.
	* @param Target The Unreal Engine / Tango Point Cloud interface object.
	* @return Container class for the point cloud.
	*/
	UFUNCTION(Category = "Tango|Depth", BlueprintPure, meta = (ToolTip = "Renders an debug point cloud to the screen.", keyword = "depth, point cloud, buffer, debug"))
		UPointCloudContainer* PassPointCloudReferenceContainer();

	/*
	* Returns the current scale factor to convert Tango distance units to Unreal distance units.
	* @return The current scale factor.
	*/
	UFUNCTION(Category = "Tango|Depth", BlueprintPure, meta = (ToolTip = "Returns the current scale factor to convert Tango distance units to Unreal distance units.", keyword = "depth, scale, factor, world"))
		float GetCurrentWorldScaleFactor();
private:
	float LatestDepthTimeStamp;

	//This point cloud container is used to transfer the depth array pointer through blueprint.
	UPROPERTY(transient)
		UPointCloudContainer* InternalPointCloudContainer;
	
	bool ConvertPointSpace(FVector& Point, ETangoPointSpace::Type Space,bool bIsNormal);
	FVector2D ProjectVectorToScreen(UCameraComponent* ViewPoint, FVector Location);
	FVector GetVectorArrayAverage(const TArray<FVector>& Vectors);
	FPlane MakeRandomPlane(FVector CameraForward, TArray<FVector> Points);
};

