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

UTangoDevice::UTangoDevice() : UObject(), FTickableGameObject()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::UTangoDevice: Instantiating TangoDevice"));
	ConnectionState = DISCONNECTED;
	pointCloudHelper = nullptr;
	motionHelper = nullptr;
	imageHelper = nullptr;
	areaHelper = nullptr;
#if PLATFORM_ANDROID
    AppContextReference = nullptr;
#endif
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

#if PLATFORM_ANDROID
/*
 *  This function will populate the AppContextReference object with the Application context using a call to Java code via the JNI.
 *  @TODO: Investigate whether we need to call this on every connection or whether we can get away with only
 * triggering it once on app startup (i.e. will the application context switch? Is the bad global reference we
 * experienced when calling only at TangoDevice startup due to local references from the Unreal JNI pipeline being
 * converted into globals incorrectly, or because the Application context which is being pointed to has changed?)
 */
void UTangoDevice::PopulateAppContext()
{
    jobject AppContext = NULL;

    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        //We identify the method we need to call, by passing the name of the function.
        static jmethodID Method = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetAppContext", "()Landroid/content/Context;", false);
        //Once jmethodID has been identified, we supply it as an argument to CallObjectMethod.
        //Use CallObjectMethod for functions which return a jobject, CallIntMethod for functions which return an int, etc.
        AppContext = FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis, Method);
        
        if (AppContext == NULL)
        {
            UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Error - app context is still NULL after retrieval call!"));
            AppContextReference = nullptr;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Success - appcontext jobject is not null!!"));
            //Now we cache the activity for later use to prevent garbage collection
            AppContextReference = Env->NewGlobalRef(AppContext);
        }
    }
    
    return;
}

void UTangoDevice::DePopulateAppContext()
{
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        Env->DeleteGlobalRef(AppContextReference);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::PopulateAppContext: Success - appcontext jobject is not null!!"));
    }
    
}
#endif

void UTangoDevice::DeallocateResources()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::DeallocateResources: Called"));
#if PLATFORM_ANDROID
	if (config_ != nullptr)
	{
		TangoConfig_free(config_);
		config_ = nullptr;
        DePopulateAppContext();
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
	}
}



void UTangoDevice::BeginDestroy()
{
	Super::BeginDestroy();
	DeallocateResources();
	DestroyEventresources();
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
		Instance->SetFlags(RF_Transient);
		//This flag ensures the singleton is never garbage collected.
		if (Instance->IsSafeForRootSet())
		{
			Instance->SetInternalFlags(Instance->GetInternalFlags() | EInternalObjectFlags::RootSet);
		}
		else
		{
			UE_LOG(ProjectTangoPlugin, Error, TEXT("TangoDevice is not save for RootSet!"));
		}
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
	ConnectTangoService();
#endif
}


bool UTangoDevice::SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration, bool bPreRuntime)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: Called."));
#if PLATFORM_ANDROID
	if (config_ == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime config Tango config pointer is nullptr."));
		return false;
	}
	if(!bPreRuntime && !IsTangoServiceRunning() )
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set runtime config since Tango Service is not running."));
		return false;
	}

#endif
	bool bSuccess = true;
	if (Configuration.bEnableDepth && !CurrentConfig.bEnableDepthCapabilities)
	{
		Configuration.bEnableDepth = false;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableDepth to Runtimeconfig since Config has no DepthCapabilites enabled"));
	}
#if PLATFORM_ANDROID
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: Rate is: %d "),(Configuration.bEnableDepth ? Configuration.RuntimeDepthFramerate : 0));
	bSuccess = TangoConfig_setInt32(config_, "config_runtime_depth_framerate", Configuration.bEnableDepth ? Configuration.RuntimeDepthFramerate : 0) == TANGO_SUCCESS && bSuccess;
	if (!bPreRuntime)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::SetTangoRuntimeConfig: TangoService_setRuntimeConfig"));
		bSuccess = TangoService_setRuntimeConfig(config_) && bSuccess;
	}
#endif


	if (Configuration.bEnableColorCamera  && !CurrentConfig.bEnableColorCameraCapabilities)
	{
		Configuration.bEnableColorCamera = false;
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoDevice::SetTangoRuntimeConfig: Unable to set EnableColorCamera to Runtimeconfig since Config has no ColorCameraCapabilites enabled"));
	}
	else if (CurrentConfig.bEnableColorCameraCapabilities && getTangoDeviceImagePointer())
	{
		getTangoDeviceImagePointer()->setRuntimeConfig(Configuration);
	}
	CurrentRuntimeConfig = Configuration;


	return bSuccess;
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
	return CurrentConfig.bEnableLearningMode && IsTangoServiceRunning();
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

	bool bSuccess = true;
	bSuccess = TangoConfig_setBool(config_, "config_enable_auto_recovery", CurrentConfig.bEnableAutoRecovery) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_enable_color_camera", CurrentConfig.bEnableColorCameraCapabilities) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_color_mode_auto", CurrentConfig.bColorModeAuto) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_enable_depth", CurrentConfig.bEnableDepthCapabilities) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_high_rate_pose", CurrentConfig.bHighRatePose) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_enable_learning_mode", CurrentConfig.bEnableLearningMode) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_enable_low_latency_imu_integration", CurrentConfig.bLowLatencyIMUIntegration) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_enable_motion_tracking", CurrentConfig.bEnableMotionTracking) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setBool(config_, "config_smooth_pose", CurrentConfig.bSmoothPose) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setInt32(config_, "config_color_exp", CurrentConfig.ColorExposure) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setInt32(config_, "config_color_iso", CurrentConfig.ColorISO) == TANGO_SUCCESS	&& bSuccess;
	bSuccess = TangoConfig_setString(config_, "config_load_area_description_UUID", TCHAR_TO_ANSI(*CurrentConfig.AreaDescription.UUID)) == TANGO_SUCCESS	&& bSuccess;
	
	if (!bSuccess)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT(" UTangoDevice::ApplyConfig: ApplyConfig FAILED because the config parameters could not be set."));
	}

	bSuccess = SetTangoRuntimeConfig(CurrentRuntimeConfig,true) && bSuccess;

	return bSuccess;
}

/*
 * This funciton sends a request to the Java layer to start the Tango service
 */
void UTangoDevice::ConnectTangoService()
{
    
    //@TODO: Call the Java RequestConnectToService function
    UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: starting service binding request call."));
    //Make the call to Java
    
#if PLATFORM_ANDROID
    jobject AppContext = NULL;
    
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        
        static jmethodID RequestTangoServiceMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_RequestTangoService", "()V", false);
        
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, RequestTangoServiceMethod);
    }
    else
    {
        UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::ConnectTangoService: Could not get Java environment!"));
    }
#endif
    UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::ConnectTangoService: Finished sevice binding request call."));

}

/*
 * This funciton sends a request to the Java layer to start the Tango service
 */
void UTangoDevice::UnbindTangoService()
{
    
    //@TODO: Call the Java RequestConnectToService function
    UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::UnbindTangoService: starting service unbinding request call."));
    //Make the call to Java
    
#if PLATFORM_ANDROID
    jobject AppContext = NULL;
    
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        
        static jmethodID TestJavaRoundTripMethod = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnbindTangoService", "()V", false);
        
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, TestJavaRoundTripMethod);
    }
    else
    {
        UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::UnbindTangoService: Could not get Java environment!"));
    }
#endif
    UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::UnbindTangoService: Finished sevice unbinding request call."));
    
}


#if PLATFORM_ANDROID
extern "C"
{
    //JavaThunkCpp: Native function which is called by Java code. Completes connection to the Tango service.
    JNIEXPORT void JNICALL
    Java_com_projecttango_plugin_ProjectTangoInterface_OnJavaTangoServiceConnected(JNIEnv* env, jobject, jobject iBinder)
    {
        UE_LOG(LogTemp, Warning, TEXT("UTangoDevice::OnJavaServiceConnected: Success- C++ function called from Java!"));
        //Java service connection finished, complete C API connection
        UTangoDevice::Get().BindAndCompleteConnectionToService(env, iBinder);
    }
}
#endif

#if PLATFORM_ANDROID
/*
 *  This function should be called after the Tango service has been bound at the Java level.
 */
void UTangoDevice::BindAndCompleteConnectionToService(JNIEnv* env, jobject iBinder)
{
    
    //Bind to the Tango service
    if (TangoService_setBinder(env, iBinder) != TANGO_SUCCESS)
    {
        UE_LOG(ProjectTangoPlugin, Error, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: could not bind to Tango Service."));
    }
    else
    {
        UE_LOG(ProjectTangoPlugin, Log, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: successfully bound to Tango Service."));
    }
    
    //Apply the service configuration
    ApplyConfig();
    
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Called"));
	if (config_ == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT(" UTangoDevice::BindAndCompleteConnectionToService: failed because config is NULL."));
		return;
	}

    //Refresh Java global reference here
    /*@TODO: This is a little hacky, see if we can get a reference to a state which won't be garbage collected so we don't need to do this.*/
    PopulateAppContext();
    
    //Attempt to connect to the now bound service
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Connecting!"));
	ConnectionState = TangoService_connect(AppContextReference, config_) == TANGO_SUCCESS ? CONNECTED : FAILED_TO_CONNECT;

	//Creating and deleting of additional class components that register callbacks and do their own thing
	if (CurrentConfig.bEnableDepthCapabilities && getTangoDevicePointCloudPointer() == nullptr)
	{
		pointCloudHelper = new TangoDevicePointCloud(config_);
	}
	else if (!CurrentConfig.bEnableDepthCapabilities && getTangoDevicePointCloudPointer() != nullptr)
	{
		delete pointCloudHelper;
		pointCloudHelper = nullptr;
	}

	if (CurrentConfig.bEnableMotionTracking && getTangoDeviceMotionPointer() == nullptr)
	{
		motionHelper = NewObject<UTangoDeviceMotion>(UTangoDeviceMotion::StaticClass());
		motionHelper->ProperInitialize();
	}
	else if (!CurrentConfig.bEnableMotionTracking && getTangoDeviceMotionPointer() != nullptr)
	{
		motionHelper->ConditionalBeginDestroy();
		motionHelper = nullptr;
	}

	if (CurrentConfig.bEnableColorCameraCapabilities && getTangoDeviceImagePointer() == nullptr)
	{
		imageHelper = NewObject<UTangoDeviceImage>(UTangoDeviceImage::StaticClass());
		imageHelper->Init(config_);
	}
	else if (!CurrentConfig.bEnableColorCameraCapabilities && getTangoDeviceImagePointer() != nullptr)
	{
		imageHelper->ConditionalBeginDestroy();
		imageHelper = nullptr;
	}

	//@todo: Investigate if 'AreaHelper' is overloaded, or if we need to move 'IsLocalized' into the motion component
	if (CurrentConfig.bEnableLearningMode && getTangoDeviceAreaLearningPointer() == nullptr)
	{
		areaHelper = new TangoDeviceAreaLearning();
	}
	else if (!CurrentConfig.bEnableLearningMode && getTangoDeviceAreaLearningPointer() != nullptr)
	{
		delete areaHelper;
		areaHelper = nullptr;
	}

	if (getTangoServiceStatus() == FAILED_TO_CONNECT)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Unable to connect!"));
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: Connection succesfull! Now connecting callbacks!"));
		if (getTangoDeviceMotionPointer() != nullptr)
		{
			getTangoDeviceMotionPointer()->ConnectCallback();
		}
		if (getTangoDevicePointCloudPointer() != nullptr)
		{
			getTangoDevicePointCloudPointer()->ConnectCallback();
		}
		if (getTangoDeviceImagePointer() != nullptr && CurrentRuntimeConfig.bEnableColorCamera)
		{
			getTangoDeviceImagePointer()->ConnectCallback();
		}
	}
	ConnectEventCallback();//Activate Events
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::BindAndCompleteConnectionToService: FINISHED"));
	BroadCastConnect();
	//Call a second time to make depth disabling at startup work.
	if (!CurrentRuntimeConfig.bEnableDepth && CurrentConfig.bEnableDepthCapabilities)
	{
		SetTangoRuntimeConfig(CurrentRuntimeConfig);
	}
}
#endif

void UTangoDevice::DisconnectTangoService(bool bByAppServicePause)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::DisconnectTangoService: Called"));
	
    if (getTangoServiceStatus() == CONNECTED)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT(" UTangoDevice::DisconnectTangoService: will now disconnect!"));

        //Free the Tango config object
        TangoConfig_free(config_);
        config_ = NULL;
        
        //Disconnect from the C service
		TangoService_disconnect();
        
        //Unbind from the Java-level service after the TangoService_disconnect call
        UnbindTangoService();
        
        //Mark the current connection state and broadcast the disconnection
		ConnectionState = bByAppServicePause ? DISCONNECTED_BY_APPSERVICEPAUSE : DISCONNECTED;
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
		UE_LOG(ProjectTangoPlugin, Log, TEXT(" UTangoDevice::AppServiceResume: Connect service called"));
		ApplyConfig();
		ConnectTangoService();
	}
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServiceResume: FINISHED"));
}

void UTangoDevice::AppServicePause()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDevice::AppServicePause: Called"));
	if (IsTangoServiceRunning()) //We only disconnect when it is already running!
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