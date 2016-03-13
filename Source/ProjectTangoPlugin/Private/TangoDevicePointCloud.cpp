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
#include "TangoDevicePointCloud.h"

#include "TangoDevice.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
//#include "Private/Android/AndroidJNI.h"
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"


/** Static function called by the Tango Library with depth cloud point data.
*/
void TangoDevicePointCloud::OnXYZijAvailable(const TangoXYZij* XYZ_ij)
{

	if (XYZ_ij->xyz_count <= VertCapacity)
	{
		pthread_mutex_lock(&xyzij_mutex);
		if (RawDataB != nullptr && XYZ_ij->xyz != nullptr)
		{


			memcpy(RawDataB, XYZ_ij->xyz, sizeof(float) * 3 * VertCount);

			/*float(*temp)[3] = RawData;
			RawData = XYZ_ij->xyz;
			XYZ_ij->xyz = temp;*/

			NewDataTimeStamp = XYZ_ij->timestamp;
			VertCount = XYZ_ij->xyz_count;

		}
		pthread_mutex_unlock(&xyzij_mutex);
	}
}
#endif

//START - Tango Point Cloud functions
int32 TangoDevicePointCloud::GetMaxVertexCapacity()
{
	return VertCapacity;
}

TArray<FVector>& TangoDevicePointCloud::GetPointCloud()
{
	return PointCloudValues;
}

//Returns the latest available XYZijData object.
FTangoXYZijData TangoDevicePointCloud::GetLatestXYZijData()
{
	FTangoXYZijData Result = FTangoXYZijData();
#if PLATFORM_ANDROID
	//Result.xyz = GetPointCloud();
	Result.timestamp = GetPointCloudTimestamp();
	Result.count = GetPointCloud().Num(); //TODO: This is kinda redundant, remove it?
#endif

	return Result;
}

float TangoDevicePointCloud::GetPointCloudTimestamp()
{
	return TimeStamp;
}
void TangoDevicePointCloud::TickByDevice()
{
#if PLATFORM_ANDROID

	bool newDataAvailable = false;
	int count = 0;

	if (TimeStamp != NewDataTimeStamp)
	{
		pthread_mutex_lock(&xyzij_mutex);
		TimeStamp = NewDataTimeStamp;
		newDataAvailable = true;
		count = VertCount;
		
		//Swap buffers
		float(*temp)[3] = RawData;
		RawData = RawDataB;
		RawDataB = temp;
		pthread_mutex_unlock(&xyzij_mutex);
	}

	if (newDataAvailable)
	{
		float WorldScale = UTangoDevice::Get().GetMetersToWorldScale();
		PointCloudValues.SetNum(count, false);
		for (int i = 0; i < count; ++i)
		{
			PointCloudValues[i].X = RawData[i][2] * WorldScale;
			PointCloudValues[i].Y = RawData[i][0] * WorldScale;
			PointCloudValues[i].Z = -RawData[i][1] * WorldScale;
		}
	}
#endif
}

TangoDevicePointCloud::TangoDevicePointCloud(
#if PLATFORM_ANDROID
	TangoConfig config_
#endif
	)
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: Creating TangoDevicePointCloud!"));
	//Setting up Point Cloud Buffers

#if PLATFORM_ANDROID

	pthread_mutex_init(&xyzij_mutex,NULL);

	int max_point_cloud_elements = 0;
	bool success = TangoConfig_getInt32(config_, "max_point_cloud_elements", &max_point_cloud_elements) == TANGO_SUCCESS;
	uint32_t MaxPointCloudVertexCount = static_cast<uint32_t>(max_point_cloud_elements);

	if (success)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: allocations"));
		pthread_mutex_lock(&xyzij_mutex); 
		VertCount = 0;
		VertCapacity = MaxPointCloudVertexCount;
		PointCloudValues.Reserve(static_cast<int32_t>(max_point_cloud_elements));

		RawData = new float[MaxPointCloudVertexCount][3];
		RawDataB = new float[MaxPointCloudVertexCount][3];
		pthread_mutex_unlock(&xyzij_mutex);

	}
	else
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: construction failed because read of max_point_cloud_elements was not successful."));
	}

#endif
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: Creating TangoDevicePointCloud FINISHED"));
}

void TangoDevicePointCloud::ConnectCallback()
{
	UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDevicePointCloud::ConnectCallback: RegisterCallBack!"));
#if PLATFORM_ANDROID
	TangoService_connectOnXYZijAvailable([](void*, const TangoXYZij* XYZ_ij) {UTangoDevice::Get().getTangoDevicePointCloudPointer()->OnXYZijAvailable(XYZ_ij); });
#endif
}

TangoDevicePointCloud::~TangoDevicePointCloud()
{
#if PLATFORM_ANDROID
	delete[] RawData;
	delete[] RawDataB;
	pthread_mutex_destroy(&xyzij_mutex);
#endif
}
//END - Tango Point Cloud functions