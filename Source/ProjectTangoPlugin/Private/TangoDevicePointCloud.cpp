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
#include "TangoPointCloudComponent.h"

#include "TangoDevice.h"

#include <UnrealTemplate.h>

#if PLATFORM_ANDROID
#include "Android/AndroidJNI.h"
#include "AndroidApplication.h"


/** Static function called by the Tango Library with depth cloud point data.
*/
void TangoDevicePointCloud::OnXYZijAvailable(const TangoXYZij* XYZ_ij)
{
	if (XYZ_ij->xyz_count <= VertCapacity)
	{
		FScopeLock ScopeLock(&XYZIJLock);
		if (RawDataB != nullptr && XYZ_ij->xyz != nullptr)
		{
			memcpy(IJDataB, XYZ_ij->ij, sizeof(uint32_t) * XYZ_ij->ij_cols * XYZ_ij->ij_rows);
			memcpy(RawDataB, XYZ_ij->xyz, sizeof(float) * 3 * VertCount);
			NewDataTimeStamp = XYZ_ij->timestamp;
			VertCount = XYZ_ij->xyz_count;
			ColumnCount = XYZ_ij->ij_cols;
			RowCount = XYZ_ij->ij_rows;
		}
	}
}
#endif

int32 TangoDevicePointCloud::GetMaxVertexCapacity()
{
	return VertCapacity;
}

TArray<FVector>& TangoDevicePointCloud::GetPointCloud()
{
	return PointCloudValues;
}

float TangoDevicePointCloud::GetPointCloudTimestamp()
{
	return TimeStamp;
}
void TangoDevicePointCloud::TickByDevice()
{
#if PLATFORM_ANDROID

	bool bIsNewDataAvailable = false;
	int count = 0;

	if (TimeStamp != NewDataTimeStamp)
	{
		FScopeLock ScopeLock(&XYZIJLock);
		TimeStamp = NewDataTimeStamp;
		bIsNewDataAvailable = true;
		count = VertCount;
		//Swap buffers
		float(*temp)[3] = RawData;
		RawData = RawDataB;
		RawDataB = temp;

		int32* ITemp = IJDataA;
		IJDataA = IJDataB;
		IJDataB = ITemp;

	}

	if (bIsNewDataAvailable)
	{
		float WorldScale = UTangoDevice::Get().GetMetersToWorldScale();
		PointCloudValues.SetNum(count, false);
		for (int i = 0; i < count; ++i)
		{
			PointCloudValues[i].X = RawData[i][2] * WorldScale;
			PointCloudValues[i].Y = RawData[i][0] * WorldScale;
			PointCloudValues[i].Z = -RawData[i][1] * WorldScale;
		}
		for (int i = 0; i < UTangoDevice::Get().PointCloudComponents.Num(); ++i)
		{
			if (UTangoDevice::Get().PointCloudComponents[i] != nullptr)
			{
				UTangoDevice::Get().PointCloudComponents[i]->OnTangoXYZijAvailable.Broadcast(TimeStamp);
			}
			else
			{
				UTangoDevice::Get().PointCloudComponents.RemoveAt(i);
				i--;
			}
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
	TimeStamp = 0;
#if PLATFORM_ANDROID

	NewDataTimeStamp = 0;
	int max_point_cloud_elements = 0;
	bool bSuccess = TangoConfig_getInt32(config_, "max_point_cloud_elements", &max_point_cloud_elements) == TANGO_SUCCESS;
	uint32_t MaxPointCloudVertexCount = static_cast<uint32_t>(max_point_cloud_elements);

	if (bSuccess)
	{
		UE_LOG(ProjectTangoPlugin, Log, TEXT("TangoDevicePointCloud::TangoDevicePointCloud: allocations. Max point count: %d"),MaxPointCloudVertexCount);
		FScopeLock ScopeLock(&XYZIJLock);
		VertCount = 0;
		VertCapacity = MaxPointCloudVertexCount;
		PointCloudValues.Reserve(static_cast<int32_t>(max_point_cloud_elements));

		RawData = new float[MaxPointCloudVertexCount][3];
		RawDataB = new float[MaxPointCloudVertexCount][3];

		IJDataA = new int32[MaxPointCloudVertexCount];
		IJDataB = new int32[MaxPointCloudVertexCount];

		ColumnCount = 0;
		RowCount = 0;
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
	FScopeLock ScopeLock(&XYZIJLock);
	delete[] RawData;
	delete[] RawDataB;
	delete[] IJDataA;
	delete[] IJDataB;
	RawData = nullptr;
	RawDataB = nullptr;
	IJDataA = nullptr;
	IJDataB = nullptr;
#endif
}

int32 * TangoDevicePointCloud::GetIJData(uint32 & _RowCount, uint32 & ColCount)
{
#if PLATFORM_ANDROID
	_RowCount = RowCount;
	ColCount = ColumnCount;
	return IJDataA;
#else
	_RowCount = 0;
	ColCount = 0;
	return nullptr;
#endif
}
