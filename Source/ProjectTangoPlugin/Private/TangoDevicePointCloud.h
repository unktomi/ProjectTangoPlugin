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

#include "Object.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#include <pthread.h>
#endif



/**
 * 
 */
class TangoDevicePointCloud
{
	
public:
	int32 GetMaxVertexCapacity();
	TArray<FVector>& GetPointCloud();
	FTangoXYZijData GetLatestXYZijData();
	float GetPointCloudTimestamp();
	void TickByDevice();

	TangoDevicePointCloud(
#if PLATFORM_ANDROID
		TangoConfig config_
#endif
		);
	void ConnectCallback();
	~TangoDevicePointCloud();
	

	//Mutex to lock the buffer when being swapped or when b side is written from Tango Thread
	//Of course only Reading is threadsafe from side A - side B should only be touched from the Tango Thread and by the swap.
private:
#if PLATFORM_ANDROID
	void OnXYZijAvailable(const TangoXYZij* XYZ_ij);
#endif


#if PLATFORM_ANDROID
	pthread_mutex_t xyzij_mutex;
	//Raw Data
	float(*RawData)[3];
	float(*RawDataB)[3];
	uint32_t VertCount;
	double NewDataTimeStamp;
#endif
	uint32_t VertCapacity;
	//safe Data Timestamp
	double TimeStamp;
	//Depth Information
	TArray<FVector> PointCloudValues;
};
