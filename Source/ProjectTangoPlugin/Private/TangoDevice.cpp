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
#include "TangoFromToCObject.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

UTangoDevice* UTangoDevice::Instance;

UTangoDevice::UTangoDevice() : FTickableGameObject(), UObject()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: Instantiating TangoDevice"));
	ConnectionState = DISCONNECTED;
	pointCloudHelper = nullptr;
	motionHelper = nullptr;
	imageHelper = nullptr;
	areaHelper = nullptr;
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: Instantiating TangoDevice FINISHED"));
}

void UTangoDevice::ProperInitialize()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ProperInitialize: called"));
	ConnectionState = DISCONNECTED;
	pointCloudHelper = nullptr;
	motionHelper = nullptr;
	imageHelper = nullptr;
	areaHelper = nullptr;

#if PLATFORM_ANDROID
	FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this, &UTangoDevice::AppServiceResume);
	FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this, &UTangoDevice::AppServicePause);
	config_ = nullptr;
	InitEventresources();
#endif

	bHasBeenPropelyInitialized = true;
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ProperInitialize: FINISHED"));
}

void UTangoDevice::DeallocateResources()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::DeallocateResources: Called"));
#if PLATFORM_ANDROID
	if (config_ != nullptr)
	{
		TangoConfig_free(config_);
		config_ = nullptr;
	}
#endif
	if (getTangoDevicePointCloudPointer() != nullptr)
	{
		delete getTangoDevicePointCloudPointer();
		pointCloudHelper = nullptr;
	}
	if (getTangoDeviceMotionPointer() != nullptr)
	{
		getTangoDeviceMotionPointer()->ConditionalBeginDestroy();
		motionHelper = nullptr;
	}
	if (getTangoDeviceImagePointer() != nullptr)
	{
		getTangoDeviceImagePointer()->ConditionalBeginDestroy();
		imageHelper = nullptr;
	}
}

UTangoDevice::~UTangoDevice()
{
	if (bHasBeenPropelyInitialized)
	{
		DeallocateResources();
		DestroyEventresources();
		if (GEngine)
		{
			GEngine->ViewExtensions.Remove(ViewExtension);
		}
	}
}



void UTangoDevice::BeginDestroy()
{
	Super::BeginDestroy();
	DeallocateResources();
	DestroyEventresources();
	if (GEngine && bFTangoViewExtensionRegistered)
	{
		bFTangoViewExtensionRegistered = false;
		GEngine->ViewExtensions.Remove(ViewExtension);
	}
}

bool UTangoDevice::IsTickable() const
{
	return bHasBeenPropelyInitialized;
}

void UTangoDevice::Tick(float DeltaTime)
{
	if (getTangoDeviceImagePointer())
	{
		getTangoDeviceImagePointer()->TickByDevice();
	}
	if (getTangoDevicePointCloudPointer())
	{
		getTangoDevicePointCloudPointer()->TickByDevice();
	}
	BroadCastEvents();
	//Since the GEngine takes a while to initialize we will have to do this here....
	if (GEngine && !bFTangoViewExtensionRegistered)
	{
		//UE_LOG(GoogleTangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: GEngine valid. Late Update working. %d"),reinterpret_cast<int32>(this));
		GEngine->ViewExtensions.Add(ViewExtension);
		bFTangoViewExtensionRegistered = true;
	}
}

TStatId UTangoDevice::GetStatId() const
{
	return TStatId();
}

UTangoDevice& UTangoDevice::Get()
{
	if (!Instance)
	{
		Instance = NewObject<UTangoDevice>(UTangoDevice::StaticClass());
		Instance->ProperInitialize();
		//This flag ensures the singleton is never garbage collected.
		Instance->SetFlags(RF_RootSet);
		Instance->SetFlags(RF_Transient);
	}

	return *Instance;
}

TangoDevicePointCloud * UTangoDevice::getTangoDevicePointCloudPointer()
{
	return pointCloudHelper;
}

UTangoDeviceMotion * UTangoDevice::getTangoDeviceMotionPointer()
{
	return motionHelper;
}

UTangoDeviceImage * UTangoDevice::getTangoDeviceImagePointer()
{
	return imageHelper;
}

TangoDeviceAreaLearning * UTangoDevice::getTangoDeviceAreaLearningPointer()
{
	return areaHelper;
}

//START - Core Tango functions
bool UTangoDevice::IsTangoServiceRunning()
{
	return getTangoServiceStatus() == CONNECTED;
}

UTangoDevice::ServiceStatus UTangoDevice::getTangoServiceStatus()
{
	return ConnectionState;
}

void UTangoDevice::RestartService(FTangoConfig & config, FTangoRuntimeConfig& runtimeConfig)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::RestartService: Called"));
	StopTangoService();
	StartTangoService(config, runtimeConfig);
}

void UTangoDevice::StartTangoService(FTangoConfig & config, FTangoRuntimeConfig& runtimeConfig)
{
	CurrentConfig = config;
	CurrentRuntimeConfig = runtimeConfig;
#if PLATFORM_ANDROID
	ApplyConfig();
	ConnectTangoService();
#endif
}


bool UTangoDevice::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration, bool PreRuntime)
{
#if PLATFORM_ANDROID
	if (config_ == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime config Tango config pointer is nullptr."));
		return false;
	}
	if(!PreRuntime && !IsTangoServiceRunning() )
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime config since Tango Service is not running."));
		return false;
	}
#endif
	bool success = true;
#if PLATFOR_ANDROID
	success = TangoConfig_setInt32(config_, "config_runtime_depth_framerate", Configuration.EnableDepth ? Configuration.RuntimeDepthFramerate : 0) == TANGO_SUCCESS	&& success;
#endif
	if (Configuration.EnableDepth && getTangoDevicePointCloudPointer() == nullptr)
	{
		Configuration.EnableDepth = false;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableDepth to Runtimeconfig since Config has no DepthCapabilites enabled"));
	}
	if (Configuration.EnableColorCamera && getTangoDeviceImagePointer() == nullptr)
	{
		Configuration.EnableColorCamera = false;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableColorCamera to Runtimeconfig since Config has no ColorCameraCapabilites enabled"));
	}
	else if (getTangoDeviceImagePointer())
	{
		getTangoDeviceImagePointer()->setRuntimeConfig(Configuration);
	}
	CurrentRuntimeConfig = Configuration;

#if PLATFORM_ANDROID
	if (!PreRuntime)
	{
		success = TangoService_setRuntimeConfig(config_) && success;
	}
#endif

	return success;
}

// Disconnect Tango Service.
void UTangoDevice::StopTangoService()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::StopTangoService: Called"));
#if PLATFORM_ANDROID
	DisconnectTangoService();
#endif
}

FTangoConfig& UTangoDevice::GetCurrentConfig()
{
	return CurrentConfig;
}

FTangoRuntimeConfig& UTangoDevice::GetCurrentRuntimeConfig()
{
	return CurrentRuntimeConfig;
}

float UTangoDevice::GetMetersToWorldScale()
{
	return CurrentConfig.MetersToWorldScale;
}

FTangoCameraIntrinsics UTangoDevice::GetCameraIntrinsics(TEnumAsByte<ETangoCameraType::Type> CameraID)
{
#if PLATFORM_ANDROID

	TangoCameraIntrinsics DeviceIntrinsics;
	TangoErrorType RequestResult = TangoService_getCameraIntrinsics((TangoCameraId)(int)CameraID, &DeviceIntrinsics);
	if (RequestResult == TANGO_SUCCESS)
	{
		return FromCObject(DeviceIntrinsics);
	}
#endif

	//If we're not on Android, or the request result was not successful, just return an empty struct
	return FTangoCameraIntrinsics();
}

bool UTangoDevice::IsLearningModeEnabled()
{
	return CurrentConfig.EnableLearningMode && IsTangoServiceRunning();
}
//END - Core Tango functions

//BEGIN - Platform Only Functions
#if PLATFORM_ANDROID

bool UTangoDevice::ApplyConfig()
{
	if (IsTangoServiceRunning())
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT(" UTangoDevice::ApplyConfig: Cannot ApplyConfig while TangoService is running!"));
		return false;
	}
	//Setting Up the Config:
	if (config_ == nullptr)
	{
		config_ = TangoService_getConfig(TANGO_CONFIG_DEFAULT);
		if (config_ == nullptr)
		{
			UE_LOG(ProjectTangoPlugin, Error, TEXT(" UTangoDevice::ApplyConfig: SetConfig FAILED because TangoService_getConfig did return nullptr!"));
			return false;
		}
	}

	bool success = true;
	success = TangoConfig_setBool(config_, "config_enable_auto_recovery", CurrentConfig.EnableAutoRecovery) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_enable_color_camera", CurrentConfig.EnableColorCameraCapabilities) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_color_mode_auto", CurrentConfig.ColorModeAuto) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_enable_depth", CurrentConfig.EnableDepthCapabilities) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_high_rate_pose", CurrentConfig.HighRatePose) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_enable_learning_mode", CurrentConfig.EnableLearningMode) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_enable_low_latency_imu_integration", CurrentConfig.LowLatencyIMUIntegration) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_enable_motion_tracking", CurrentConfig.EnableMotionTracking) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setBool(config_, "config_smooth_pose", CurrentConfig.SmoothPose) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setInt32(config_, "config_color_exp", CurrentConfig.ColorExposure) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setInt32(config_, "config_color_iso", CurrentConfig.ColorISO) == TANGO_SUCCESS	&& success;
	success = TangoConfig_setString(config_, "config_load_area_description_UUID", TCHAR_TO_ANSI(*CurrentConfig.AreaDescription.UUID)) == TANGO_SUCCESS	&& success;
	
	if (!success)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" UTangoDevice::ApplyConfig: ApplyConfig FAILED because the config parameters could not be set."));
	}

	success = SetTangoRuntimeConfig(CurrentRuntimeConfig,true) && success;

	return success;
}

void UTangoDevice::ConnectTangoService()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: Called"));
	if (config_ == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT(" UTangoDevice::ConnectTangoService: failed because config is NULL."));
		return;
	}

	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: Connecting!"));
	ConnectionState = TangoService_connect(nullptr, config_) == TANGO_SUCCESS ? CONNECTED : FAILED_TO_CONNECT;

	//Creating and deleting of additional class components that register callbacks and do their own thing
	if (CurrentConfig.EnableDepthCapabilities && getTangoDevicePointCloudPointer() == nullptr)
	{
		pointCloudHelper = new TangoDevicePointCloud(config_);
	}
	else if (!CurrentConfig.EnableDepthCapabilities && getTangoDevicePointCloudPointer() != nullptr)
	{
		delete pointCloudHelper;
		pointCloudHelper = nullptr;
	}

	if (CurrentConfig.EnableMotionTracking && getTangoDeviceMotionPointer() == nullptr)
	{
		motionHelper = NewObject<UTangoDeviceMotion>(UTangoDeviceMotion::StaticClass());
		motionHelper->ProperInitialize();
	}
	else if (!CurrentConfig.EnableMotionTracking && getTangoDeviceMotionPointer() != nullptr)
	{
		motionHelper->ConditionalBeginDestroy();
		motionHelper = nullptr;
	}

	if (CurrentConfig.EnableColorCameraCapabilities && getTangoDeviceImagePointer() == nullptr)
	{
		imageHelper = NewObject<UTangoDeviceImage>(UTangoDeviceImage::StaticClass());
		imageHelper->Init(config_);
	}
	else if (!CurrentConfig.EnableColorCameraCapabilities && getTangoDeviceImagePointer() != nullptr)
	{
		imageHelper->ConditionalBeginDestroy();
		imageHelper = nullptr;
	}

	//@todo: Investigate if 'AreaHelper' is overloaded, or if we need to move 'IsLocalized' into the motion component
	if (CurrentConfig.EnableLearningMode && getTangoDeviceAreaLearningPointer() == nullptr)
	{
		areaHelper = new TangoDeviceAreaLearning();
	}
	else if (!CurrentConfig.EnableLearningMode && getTangoDeviceAreaLearningPointer() != nullptr)
	{
		delete areaHelper;
		areaHelper = nullptr;
	}

	if (getTangoServiceStatus() == FAILED_TO_CONNECT)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::ConnectTangoService: Unable to connect!"));
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: Connection succesfull! Now connecting callbacks!"));
		if (getTangoDeviceMotionPointer() != nullptr)
		{
			getTangoDeviceMotionPointer()->ConnectCallback();
		}
		if (getTangoDevicePointCloudPointer() != nullptr)
		{
			getTangoDevicePointCloudPointer()->ConnectCallback();
		}
		if (getTangoDeviceImagePointer() != nullptr)
		{
			getTangoDeviceImagePointer()->ConnectCallback();
		}
	}
	ConnectEventCallback();//Activate Events
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: FINISHED"));
	BroadCastConnect();
}

void UTangoDevice::DisconnectTangoService(bool byAppServicePause)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::DisconnectTangoService: Called"));

	//TangoConfig_free(config_);
	//config_ = NULL;
	if (getTangoServiceStatus() == CONNECTED)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" UTangoDevice::DisconnectTangoService: will now disconnect!"));

		TangoService_disconnect();
		ConnectionState = byAppServicePause ? DISCONNECTED_BY_APPSERVICEPAUSE : DISCONNECTED;
		BroadCastDisconnect();
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" UTangoDevice::DisconnectTangoService: Tango is not registered as connected, and is therefore not disconnecting properly!!"));
	}
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::DisconnectTangoService: FINISHED"));
}

//END - Platform Only Functions

void UTangoDevice::AppServiceResume()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServiceResume: Called"));
	if (getTangoServiceStatus() == DISCONNECTED_BY_APPSERVICEPAUSE)//We only reconnect when we disconnected the Tango by AppServicePause first!
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" UTangoDevice::AppServiceResume: Connect service called"));
		ApplyConfig();
		ConnectTangoService();
	}
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServiceResume: FINISHED"));
}

void UTangoDevice::AppServicePause()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServicePause: Called"));
	if (IsTangoServiceRunning())//We only disconnect when it is already running!
	{
		DeallocateResources();
		DisconnectTangoService(true);
	}
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServicePause: FINISHED"));
}

#endif

//TangoDeviceMotion Helper

void UTangoDevice::AddTangoMotionComponent(UTangoMotionComponent* Component, TArray<FTangoCoordinateFramePair>& Requests)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Started"));
	//Check if component is already added and remove components that have been garbage collected
	bool bComponentFound = false;
	for (int i = 0; i < MotionComponents.Num(); ++i)
	{
		while (MotionComponents[i] == nullptr)
		{
			MotionComponents.RemoveAt(i);
			RequestedPairs.RemoveAt(i);
			if (i >= MotionComponents.Num())
			{
				break;
			}
		}
		if (MotionComponents[i] == Component)
		{
			bComponentFound = true;
			RequestedPairs[i] = Requests;
		}
	}
	if (!bComponentFound)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Not Found"));
		MotionComponents.Add(Component);
		RequestedPairs.Add(Requests);
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Found"));
	}
	if (getTangoDeviceMotionPointer() != nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AddTangoMotionComponent: Initiating Rebuild"));
		getTangoDeviceMotionPointer()->CheckForChangeInRequests();
	}
}