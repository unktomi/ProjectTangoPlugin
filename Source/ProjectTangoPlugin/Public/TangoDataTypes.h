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

#include "TangoDataTypes.generated.h"


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PROJECT TANGO PLUGIN BLUEPRINT-FRIENDLY ENUMERATIONS BEGIN HERE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
namespace ETangoPoseStatus
{
	enum Type
	{
		INITIALIZING	UMETA(DisplayName = "Tango pose initializing"),
		VALID			UMETA(DisplayName = "Tango pose valid"),
		INVALID			UMETA(DisplayName = "Tango pose invalid"),
		UNKNOWN			UMETA(DisplayName = "Tango pose unknown")
	};
}

UENUM(BlueprintType)
namespace ETangoPositionContext
{
	enum Type
	{
		START_TO_DEVICE UMETA(DisplayName = "Device from Service Start"),
		ADF_TO_DEVICE	UMETA(DisplayName = "Device from Area Description"),
		ADF_TO_START	UMETA(DisplayName = "Service Start from Area Description"),
		PREV_TO_DEVICE	UMETA(DisplayName = "Device from Previous Frame")
	};
}

/*
	ETangoCoordinateFrameType
	Allows users to specify from the available coordinate systems the Tango service can use.
	Can be grouped into pairs using the FTangoCoordinateFramePair structure;
*/
UENUM(BlueprintType)
namespace ETangoCoordinateFrameType
{

	enum Type
	{
		GLOBAL_WGS84			UMETA(DisplayName = "Global: WGS84"),
		AREA_DESCRIPTION		UMETA(DisplayName = "Area Description"),
		START_OF_SERVICE		UMETA(DisplayName = "Start of Service"),
		PREVIOUS_DEVICE_POSE	UMETA(DisplayName = "Previous Device Pose"),
		DEVICE					UMETA(DisplayName = "Device"),
		IMU						UMETA(DisplayName = "IMU"),
		DISPLAY					UMETA(DisplayName = "Display"), 
		CAMERA_COLOR			UMETA(DisplayName = "Camera: Colour"),
		CAMERA_DEPTH			UMETA(DisplayName = "Camera: Depth"),
		CAMERA_FISHEYE			UMETA(DisplayName = "Camera: Fisheye")
	};
}

/*
	ETangoPermissionType
*/
UENUM(BlueprintType)
namespace ETangoPermissionType
{
	enum Type
	{
		AREA_LEARNING	UMETA(DisplayName = "Area Learning"),
		ADF_LOAD_SAVE	UMETA(DisplayName = "Area Description File Load/Save"),
		MOTION_TRACKING	UMETA(DisplayName = "IMU")
	};
}

/*
	ETangoCalibrationType
*/
UENUM(BlueprintType)
namespace ETangoCalibrationType
{
	enum Type
	{
		EQUIDISTANT				UMETA(DisplayName = "Equidistant"),
		POLYNOMIAL_2_PARAMETERS	UMETA(DisplayName = "Polynomial; 2 parameters"),
		POLYNOMIAL_3_PARAMETERS	UMETA(DisplayName = "Polynomial; 3 parameters"),
		POLYNOMIAL_5_PARAMETERS	UMETA(DisplayName = "Polynomial; 5 parameters"),
		UNKNOWN					UMETA(DisplayName = "Unknown")
	};
}

/*
	ETangoDatasetRecordingMode
*/
UENUM(BlueprintType)
namespace ETangoDatasetRecordingMode
{
	enum Type
	{
		MOTION_TRACKING			UMETA(DisplayName = "Motion Tracking"),
		SCENE_RECONSTRUCTION	UMETA(DisplayName = "Scene Reconstruction")
	};
}

/*
	ETangoCameraType
*/
UENUM(BlueprintType)
namespace ETangoCameraType
{
	enum Type
	{
		COLOR	UMETA(DisplayName = "Color Camera"),
		DEPTH	UMETA(DisplayName = "Depth Camera"),
		FISHEYE	UMETA(DisplayName = "Fisheye Camera"),
		RGBR	UMETA(DisplayName = "RGBR Camera"),
	};
}

/*
	ETangoConfigType
*/
UENUM(BlueprintType)
namespace ETangoConfigType
{
	enum Type
	{
		AREA_DESCRIPTION	UMETA(DisplayName = "Area Description"),
		CURRENT				UMETA(DisplayName = "Current"),
		DEFAULT				UMETA(DisplayName = "Default"),
		MOTION_TRACKING		UMETA(DisplayName = "Motion Tracking"),
		RUNTIME				UMETA(DisplayName = "Runtime")
	};
}

/*
	ETangoEventType
*/
UENUM(BlueprintType)
namespace ETangoEventType
{
	enum Type 
	{
		AREA_LEARNING		UMETA(DisplayName = "Area Learning Event"),
		COLOR_CAMERA		UMETA(DisplayName = "Color Camera Event"),
		FEATURE_TRACKING	UMETA(DisplayName = "Feature Tracking Event"),
		FISHEYE_CAMERA		UMETA(DisplayName = "Fisheye Camera Event"),
		GENERAL				UMETA(DisplayName = "General Event"),
		IMU					UMETA(DisplayName = "IMU Event"),
		UNKNOWN				UMETA(DisplayName = "Unknown Event")
	};
}

/*
	ETangoEventValue
*/
UENUM(BlueprintType)
namespace ETangoEventValue
{
	enum Type
	{
		SERVICE_FAULT		UMETA(DisplayName = "Tango Service Fault")
	};
}
/*
	ETangoEventKeyType
*/
UENUM(BlueprintType)
namespace ETangoEventKeyType
{
	enum Type
	{
		KEY_AREA_DESCRIPTION_SAVE_PROGRESS		UMETA(DisplayName = "Area Description Save Progress"),
		KEY_SERVICE_EXCEPTION					UMETA(DisplayName = "Service Exception"),
		DESCRIPTION_COLOR_OVER_EXPOSED			UMETA(DisplayName = "Colour Over Exposed"),
		DESCRIPTION_COLOR_UNDER_EXPOSED			UMETA(DisplayName = "Colour Under Exposed"),
		DESCRIPTION_FISHEYE_OVER_EXPOSED		UMETA(DisplayName = "Fisheye Over Exposed"),
		DESCRIPTION_FISHEYE_UNDER_EXPOSED		UMETA(DisplayName = "Fisheye Under Exposed"),
		DESCRIPTION_TOO_FEW_FEATURES			UMETA(DisplayName = "Too Few Features Tracked"),
		UNKOWN									UMETA(DisplayName = "Unkown Event")
	};
}
	
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//PROJECT TANGO PLUGIN DATA STRUCTURES BEGIN HERE
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
	FTangoEvent
	Unreal wrapper for the C TangoEvent object.
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Tango event type"))
		TEnumAsByte<ETangoEventKeyType::Type> Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information on which Tango subsystem fired the event"))
		TEnumAsByte<ETangoEventType::Type> Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Message from the Tango device"))
		FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Event time in seconds since the Tango device started"))
		float TimeStamp;
};

/*
	FTangoCoordinateFramePair
	Unreal wrapper for the C TangoCoordinateFramePair object.
*/

USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoCoordinateFramePair
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Source frame of reference"))
		TEnumAsByte<ETangoCoordinateFrameType::Type> BaseFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Target frame of reference"))
		TEnumAsByte<ETangoCoordinateFrameType::Type> TargetFrame;

	//Assume a valid pairing- the frame pair should not default to the first elements of the enumeration
	FTangoCoordinateFramePair(const TEnumAsByte<ETangoCoordinateFrameType::Type> NewBaseFrame = (ETangoCoordinateFrameType::START_OF_SERVICE),
		const TEnumAsByte<ETangoCoordinateFrameType::Type> NewTargetFrame = (ETangoCoordinateFrameType::DEVICE));

	friend bool operator== (const FTangoCoordinateFramePair& A,const FTangoCoordinateFramePair& B)
	{
		return A.BaseFrame == B.BaseFrame && A.TargetFrame == B.TargetFrame;
	}

	friend uint32 GetTypeHash(const FTangoCoordinateFramePair& Other)
	{
		return (static_cast<uint32>(Other.BaseFrame) << 16) + static_cast<uint32>(Other.TargetFrame);
	}
};


/*
	FTangoAreaDescription
	Data structure which holds information about Tango Area Description Files.
	Note that it does not contain an ADF, and can not be converted to an ADF-
	it simply holds the information needed to manipulate the ADFs in a convenient
	manner.
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoAreaDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Unique identifier of the ADF file"))
		FString UUID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Filename of the area description file"))
		FString Filename;

	FTangoAreaDescription();

	FTangoAreaDescription(const FString InUUID, const FString InFileName);

	void SetUUID(const FString NewValue);

	void SetFilename(const FString NewValue);
};

/*
FTangoRuntimeConfig
Structure for the Tango Runtime settings
Used by developers to set their desired configuration for the Tango service at runtime.
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoRuntimeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should the devices colour camera be enabled"))
		bool EnableColorCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should the devices depth sensor be enabled"))
		bool EnableDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "The frame rate of the depth sensor"))
		int32 RuntimeDepthFramerate;

	FTangoRuntimeConfig();
};

/*
	FTangoConfig
	Wrapper structure for the TangoConfig object
	Used by developers to set their desired configuration for the Tango service.
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		bool EnableAutoRecovery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Allow for activation of color camera"))
		bool EnableColorCameraCapabilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Automatically adjust ISO and Exposure"))
		bool ColorModeAuto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Allow for activation of depth camera"))
		bool EnableDepthCapabilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Wether the pose should be tracked in high frequency mode"))
		bool HighRatePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Wether ADF learning capabilities should be enabled"))
		bool EnableLearningMode;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Should low latency IMU be activated"))
		bool LowLatencyIMUIntegration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Wether motion tracking should be activated"))
		bool EnableMotionTracking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Wether the pose should be smoothed. Can result in a little bit of lag and inaccuracies"))
		bool SmoothPose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Sets exposure of the color camera"))
		int32 ColorExposure;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Sets ISO value of color camera"))
		int32 ColorISO;

	/* This struct will be used for setting only. So the getter parameters are not needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		int32 MaxPointCloudElements;
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		float DepthPeriodInSeconds;*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Determines which area description file should be loaded"))
		FTangoAreaDescription AreaDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "The scaling factor from Tango coordinates (meters) to Unreal coordiantes"))
		float MetersToWorldScale = 100.0f;

	FTangoConfig();
};

/*
	FTangoPose
	Used to represent information from a TangoPose object.
	Contains the representations of where the tango is within a particular co-ordinate system.
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoPoseData
{
	GENERATED_BODY()
	//@NOTE: Accuracy will be supported in a future update of the Tango API
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
	//	float Accuracy;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Position in the current frame of reference"))
		FVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Rotation in the current frame of reference"))
		FRotator Rotation;

	//@NOTE: This property is not exposed to Blueprint. Blueprint has no
	//representation of Quaternions, only FRotators.

	FQuat QuatRotation;

	//@NOTE: Confidence will be supported in a future update of the Tango API
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
	//	int32 Confidence;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Frame of reference of this pose"))
		FTangoCoordinateFramePair FrameOfReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Status code of this pose"))
		TEnumAsByte<ETangoPoseStatus::Type> StatusCode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Pose time in seconds since the device was started"))
		float Timestamp;


	FTangoPoseData();

	FTangoPoseData(FVector NewPosition, FRotator NewRotation, FQuat NewQuatRotation, FTangoCoordinateFramePair NewFrameOfReference,
		TEnumAsByte<ETangoPoseStatus::Type> NewStatusCode, float NewTimestamp);
};

/*
	FTangoXyzIjData
*/
//@TODO: Investigate if we should remove that
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoXYZijData
{
	GENERATED_BODY()

	//@NOTE: These properties are currently not supported in the current versions
	// of Google Tango, and will be available in a later release.
	/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		TArray<int32> ij;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		int32 ijCols;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		int32 ijRows;
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "XYZij receive time in seconds since the device was started"))
		float timestamp;

	//@NOTE: We are removing the xyz component from the Blueprint API in order to prevent mismanagement of device thermals,
	//and remove any potential deep copy of the point cloud array.
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango")
		//TArray<FVector> xyz;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Number of points in the point cloud"))
		int32 count;

	FTangoXYZijData();

};

/*
	FTangoCameraIntrinsics
*/

USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoCameraIntrinsics
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tango", meta = (ToolTip = "Calibration type of the camera"))
		TEnumAsByte<ETangoCalibrationType::Type> CalibrationType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Determines to which camera these intrinsics belong"))
		TEnumAsByte<ETangoCameraType::Type> CameraID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Horizontal center offset in pixels"))
		int32 Cx;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Vertical center offset in pixels"))
		int32 Cy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Distortion parameters"))
		TArray<float> Distortion;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Horizontal focal length in pixels"))
		float Fx;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Vertical focal length in pixels"))
		float Fy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Width in pixels"))
		int32 Width;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Height in pixels"))
		int32 Height;

	FTangoCameraIntrinsics();
	
	~FTangoCameraIntrinsics();

};


/*
	FTangoAreaDescriptionMetaData
*/
USTRUCT(BlueprintType)
struct PROJECTTANGOPLUGIN_API FTangoAreaDescriptionMetaData
{
	GENERATED_BODY()
		
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "File name of the ADF file"))
		FString Filename;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "When the ADF was created"))
		int32 MillisecondsSinceUnixEpoch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationQX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationQY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationQZ;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tango", meta = (ToolTip = "Information where on the world the ADF was created"))
		float TransformationQW;

	FTangoAreaDescriptionMetaData();

	FTangoAreaDescriptionMetaData(const FString InFileName, const int32 InMillisecondsSinceUnixEpoch, const FTransform InTransformation);

};