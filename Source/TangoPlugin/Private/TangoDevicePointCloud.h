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

#include "Object.h"

#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

class TangoDevicePointCloud
{
	
public:
	int32 GetMaxVertexCapacity();
	TArray<FVector>& GetPointCloud();
	float GetPointCloudTimestamp();
	void TickByDevice();

	TangoDevicePointCloud(
#if PLATFORM_ANDROID
		TangoConfig Config_
#endif
		);
	void ConnectCallback();
	~TangoDevicePointCloud();
	
	int32* GetIJData(uint32& _RowCount, uint32& ColCount);

private:
#if PLATFORM_ANDROID
	void OnXYZijAvailable(const TangoXYZij* XYZ_ij);
#endif

	//Mutex to lock the buffer when being swapped or when b side is written from Tango Thread
	FCriticalSection XYZIJLock;

#if PLATFORM_ANDROID

	//Of course only Reading is threadsafe from side A - side B should only be touched from the Tango Thread and by the swap.
	//Raw Data
	float(*RawData)[3];
	float(*RawDataB)[3];
	uint32_t VertCount;
	double NewDataTimeStamp;

	uint32 RowCount;
	uint32 ColumnCount;
	int32* IJDataA;
	int32* IJDataB;
#endif
	uint32_t VertCapacity;
	//safe Data Timestamp
	double TimeStamp;
	//Depth Information
	TArray<FVector> PointCloudValues;
};
