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
#include "TangoDataTypes.h"
#include "TangoDevicePointCloud.h"
#include "TangoDeviceMotion.h"
#include "TangoDeviceImage.h"
#include "TangoDeviceAreaLearning.h"
#include "TangoEventComponent.h"
#include "TangoViewExtension.h"

#include <sstream>
#include <stdlib.h>
#include <string>

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include <pthread.h>
#endif

#include "TangoDevice.generated.h"

UCLASS(NotBlueprintable, NotPlaceable, Transient)
class UTangoDevice : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

private:

	static UTangoDevice * Instance;
	UTangoDevice();
	void ProperInitialize();
	void DeallocateResources();
	~UTangoDevice();
	virtual void BeginDestroy() override;

	//Pointers to optional Submodules
	TangoDevicePointCloud* pointCloudHelper;
	UPROPERTY(transient)
		UTangoDeviceMotion* motionHelper;
	UPROPERTY(transient)
		UTangoDeviceImage* imageHelper;
	TangoDeviceAreaLearning* areaHelper;

	//FTickableGameObject interface
	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

public:
	static UTangoDevice& Get();

	//Getter functions for different submodules
	TangoDevicePointCloud* getTangoDevicePointCloudPointer();
	UTangoDeviceMotion* getTangoDeviceMotionPointer();
	UTangoDeviceImage* getTangoDeviceImagePointer();
	TangoDeviceAreaLearning* getTangoDeviceAreaLearningPointer();

	///////////////
	// Lifecycle //
	///////////////
public:
	
	//Tango Service Enums
	enum ServiceStatus
	{
		CONNECTED = 0,
		DISCONNECTED = 1,
		DISCONNECTED_BY_APPSERVICEPAUSE = 2,
		FAILED_TO_CONNECT = 3
	};

	//Core Tango functions
	bool IsTangoServiceRunning();
	bool IsLearningModeEnabled();
	ServiceStatus getTangoServiceStatus();
	void RestartService(FTangoConfig& config, FTangoRuntimeConfig& runtimeConfig);
	void StartTangoService(FTangoConfig& config, FTangoRuntimeConfig& runtimeConfig);
	bool SetTangoRuntimeConfig(FTangoRuntimeConfig Configuration,bool PreRuntime = false);
	void StopTangoService();
	
	FTangoConfig& GetCurrentConfig();
	FTangoRuntimeConfig& GetCurrentRuntimeConfig();

private:
	bool bHasBeenPropelyInitialized = false;
	//Service Status
	ServiceStatus ConnectionState;
	//Service Configuration
	FTangoConfig CurrentConfig;
	FTangoRuntimeConfig CurrentRuntimeConfig;

#if PLATFORM_ANDROID
	TangoConfig config_; //Internal Tango Pointer
	//Core service functions
	bool ApplyConfig();
	void ConnectTangoService();
	void DisconnectTangoService(bool byAppServicePause = false);
	//Delegate binding functions
	void AppServiceResume();
	void AppServicePause();
#endif

	///////////////////////////
	// General functionality //
	///////////////////////////
public:
	float GetMetersToWorldScale();
	//Tango Camera Intrinsics defined here because we need the intrinsics to start the ImageDevice!
	FTangoCameraIntrinsics GetCameraIntrinsics(TEnumAsByte<ETangoCameraType::Type> CameraID);

	//Area accessibility functions. Found in TangoDeviceADF.cpp
	FString GetLoadedAreaDescriptionUUID();
	TArray<FTangoAreaDescription> GetAreaDescriptions();
	TArray<FString> GetAllUUIDs();
	FTangoAreaDescriptionMetaData GetMetaData(FString UUID, bool& IsSuccessful);
	void ImportCurrentArea(FString Filepath, bool& IsSuccessful);
	void ExportCurrentArea(FString UUID, FString Filepath, bool& IsSuccessful);

	/////////////////
	// Tango Event //
	/////////////////
public:
	void AttachTangoEventComponent(UTangoEventComponent* Component);
private:
	void InitEventresources();
	void DestroyEventresources();
	void ConnectEventCallback();
	void BroadCastConnect();
	void BroadCastDisconnect();
	void BroadCastEvents();
#if PLATFORM_ANDROID
	void OnTangoEvent(const TangoEvent * Event);
#endif
	UPROPERTY(transient)
	TArray<UTangoEventComponent*> TangoEventComponents;
	TArray<FTangoEvent> CurrentEvents;
	//For less blocking :(
	TArray<FTangoEvent> CurrentEventsCopy;
#if PLATFORM_ANDROID
	pthread_mutex_t Event_mutex;
#endif

	/////////////////////
	// Persistent Data //
	/////////////////////
public:
	//@TODO: use friend classes instead of public
	bool bFTangoViewExtensionRegistered = false;
	TSharedRef< FTangoViewExtension, ESPMode::ThreadSafe > ViewExtension = TSharedRef< FTangoViewExtension, ESPMode::ThreadSafe >(new FTangoViewExtension());

	//ATTENTION: These properties are used by other classes.
	//They are here so persist even if the tango is being disconnected

	//TangoDeviceImage
	UPROPERTY(transient)
		UTexture2D * YTexture;
	UPROPERTY(transient)
		UTexture2D * CrTexture;
	UPROPERTY(transient)
		UTexture2D * CbTexture;
	//TangoDeviceMotion
	UPROPERTY(transient)
		TArray<UTangoMotionComponent*> MotionComponents;
	TArray<TArray<FTangoCoordinateFramePair>> RequestedPairs;
	void AddTangoMotionComponent(UTangoMotionComponent* Component, TArray<FTangoCoordinateFramePair>& Requests);
};