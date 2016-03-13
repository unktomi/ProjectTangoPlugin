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
#include "TangoDeviceMotion.h"
#include "TangoFromToCObject.h"

#include "TangoDevice.h"


#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"
#endif

UTangoDeviceMotion::UTangoDeviceMotion() : FTickableGameObject(), UObject()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::UTangoDeviceMotion: called"));
}

void UTangoDeviceMotion::ProperInitialize()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::Initialize: called"));
#if PLATFORM_ANDROID
#endif
	CheckForChangeInRequests();
	bIsProperlyInitialized = true;
}

void UTangoDeviceMotion::ConnectCallback()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::ConnectCallback: called"));
#if PLATFORM_ANDROID
	TangoCoordinateFramePair Pairs[CurrentlyRequestedPairs.Num()];// = { { TANGO_COORDINATE_FRAME_START_OF_SERVICE, TANGO_COORDINATE_FRAME_DEVICE } };
	int i = 0;
	for (auto& Elem : CurrentlyRequestedPairs)
	{
		Pairs[i].base = static_cast<TangoCoordinateFrameType>(static_cast<int32>(Elem.BaseFrame));
		Pairs[i].target = static_cast<TangoCoordinateFrameType>(static_cast<int32>(Elem.TargetFrame));
		i++;
	}
	if (TangoService_connectOnPoseAvailable(CurrentlyRequestedPairs.Num(), Pairs, [](void*, const TangoPoseData* Pose) {if (UTangoDevice::Get().getTangoDeviceMotionPointer() != nullptr)UTangoDevice::Get().getTangoDeviceMotionPointer()->OnPoseAvailable(Pose); }) != TANGO_SUCCESS)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceMotion::ConnectCallback: Was unsuccessfull"));
	}
#endif
	bCallbackIsConnected = true;
}

void UTangoDeviceMotion::BeginDestroy()
{
	Super::BeginDestroy();
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::UTangoDeviceMotion: Destructor called"));
}


/** Function called by the Tango Library with head pose data.
*/
#if PLATFORM_ANDROID
void UTangoDeviceMotion::OnPoseAvailable(const TangoPoseData* Pose)
{
	FTangoPoseData Data = FromCObject(*Pose, UTangoDevice::Get().GetMetersToWorldScale());
	PoseMutex.Lock();
	BroadcastTangoPoseData.FindOrAdd(Data.FrameOfReference) = Data; 
	PoseMutex.Unlock();
}
#endif

//START - Tango Motion functions

FTangoPoseData UTangoDeviceMotion::GetPoseAtTime(FTangoCoordinateFramePair FrameOfReference, float Timestamp)
{

	//@TODO: See if there's a way to remove the need for this data structure here
	FTangoPoseData BlueprintFriendlyPoseData;

#if PLATFORM_ANDROID
	TangoPoseData Result;

	////Remember to observe the Tango status in case the system isn't ready yet
	TangoErrorType ResultOfServiceCall;

	ResultOfServiceCall = TangoService_getPoseAtTime(static_cast<double> (Timestamp), ToCObject(FrameOfReference), &Result);

	if (ResultOfServiceCall != TANGO_SUCCESS)
	{
		//return a generic object
		return FTangoPoseData();
	}

	BlueprintFriendlyPoseData = FromCObject(Result, UTangoDevice::Get().GetMetersToWorldScale());
#endif
	return BlueprintFriendlyPoseData;
}

bool UTangoDeviceMotion::IsTickable() const
{
	return bIsProperlyInitialized;
}

TStatId UTangoDeviceMotion::GetStatId() const
{
	return TStatId();
}


void UTangoDeviceMotion::Tick(float DeltaTime)
{
	PoseMutex.Lock();
	TMap<FTangoCoordinateFramePair, FTangoPoseData> BroadcastTangoPoseDataCopy = BroadcastTangoPoseData;
	BroadcastTangoPoseData.Empty(CurrentlyRequestedPairs.Num());
	PoseMutex.Unlock();
	
	for (auto& Elem : BroadcastTangoPoseDataCopy)
	{
		auto& Components = FramePairToComponents[Elem.Key];
		for (int32 ID : Components)
		{
			if (UTangoDevice::Get().MotionComponents[ID] != nullptr)
			{
				UTangoDevice::Get().MotionComponents[ID]->OnTangoPoseAvailable.Broadcast(Elem.Value,Elem.Key);
			}
		}
	}
}

void UTangoDeviceMotion::ResetMotionTracking()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::ResetMotionTracking: Called"));
#if PLATFORM_ANDROID
	TangoService_resetMotionTracking();
#endif
}

bool UTangoDeviceMotion::IsLocalized()
{
	//@TODO: See if there's a cleaner way to poll the service than getting entire pose value and checking the validity.
#if PLATFORM_ANDROID
	TangoPoseData Result;

	////Remember to observe the Tango status in case the system isn't ready yet
	TangoErrorType ResultOfServiceCall;
	TangoCoordinateFramePair ADFFramePair = { TANGO_COORDINATE_FRAME_AREA_DESCRIPTION, TANGO_COORDINATE_FRAME_DEVICE };

	ResultOfServiceCall = TangoService_getPoseAtTime(0.0, ADFFramePair, &Result);

	if (ResultOfServiceCall == TANGO_SUCCESS)
	{
		if (Result.status_code == TANGO_POSE_VALID)
		{
			return true;
		}
		else
		{
			//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Pose obtained and is invalid.")));
			return false;
		}
	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDeviceAreaLearning::IsLocalized: Could not successfully call GetPoseAtTime!"));
		//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Pose not obtained!")));

		return false;
	}

#endif
	//We're not localized if we're not on Android
	return false;
}

void UTangoDeviceMotion::CheckForChangeInRequests()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::CheckForChangeInRequests: Called"));
	TArray<FTangoCoordinateFramePair> NewRequestedPairs;
	for (int32 mc = 0; mc < UTangoDevice::Get().RequestedPairs.Num(); mc++)
	{
		for (int32 i = 0; i < UTangoDevice::Get().RequestedPairs[mc].Num(); ++i)
		{
			NewRequestedPairs.AddUnique(UTangoDevice::Get().RequestedPairs[mc][i]);
		}
	}
	bool bAreDifferent = false;
	NewRequestedPairs.Sort([](const FTangoCoordinateFramePair& A,const FTangoCoordinateFramePair& B) {return GetTypeHash(A) < GetTypeHash(B); });
	if (NewRequestedPairs.Num() == CurrentlyRequestedPairs.Num())
	{
		for (int i = 0; i < NewRequestedPairs.Num();++i)
		{
			if (NewRequestedPairs[i].BaseFrame != CurrentlyRequestedPairs[i].BaseFrame || NewRequestedPairs[i].TargetFrame != CurrentlyRequestedPairs[i].TargetFrame)
			{
				bAreDifferent = true;
				break;
			}
		}
	}
	else
	{
		bAreDifferent = true;
	}
	if (bAreDifferent)
	{
		CurrentlyRequestedPairs = NewRequestedPairs;
		if (bCallbackIsConnected)
		{
			ConnectCallback();
		}
	}
	RebuildFramePairToComponents();
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::CheckForChangeInRequests: Finished"));
}

void UTangoDeviceMotion::RebuildFramePairToComponents()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::RebuildFramePairToComponents: Called"));
	FramePairToComponents.Empty(CurrentlyRequestedPairs.Num());
	for (auto& Elem : CurrentlyRequestedPairs)
	{
		TArray<int32> ComponentArray;
		for (int i = 0; i < UTangoDevice::Get().RequestedPairs.Num(); ++i)
		{
			for (int j = 0; j < UTangoDevice::Get().RequestedPairs[i].Num();++j)
			{
				if (UTangoDevice::Get().RequestedPairs[i][j].BaseFrame == Elem.BaseFrame && UTangoDevice::Get().RequestedPairs[i][j].TargetFrame == Elem.TargetFrame)
				{
					ComponentArray.Emplace(i);
					break;
				}
			}
		}
		FramePairToComponents.Emplace(Elem, ComponentArray);
	}
	UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceMotion::RebuildFramePairToComponents: Finished"));
}
//END - Tango Motion functions