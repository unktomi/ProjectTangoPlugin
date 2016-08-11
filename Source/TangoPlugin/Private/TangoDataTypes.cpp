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

#include "TangoPluginPrivatePCH.h"
#include "TangoDataTypes.h"
#if PLATFORM_ANDROID
#include "tango_client_api.h"
#endif

//Tango Data Types function definition begins here

//Assume a valid pairing- the frame pair should not default to the first elements of the enumeration
FTangoCoordinateFramePair::FTangoCoordinateFramePair(const TEnumAsByte<ETangoCoordinateFrameType::Type> NewBaseFrame, 
													 const TEnumAsByte<ETangoCoordinateFrameType::Type> NewTargetFrame)
{
	BaseFrame = NewBaseFrame;
	TargetFrame = NewTargetFrame;
}

FTangoAreaDescription::FTangoAreaDescription(const FString InUUID, const FString InFileName)
{
	UUID = InUUID;
	Filename = InFileName;
}

void FTangoAreaDescription::SetUUID(const FString NewValue)
{
	UUID = NewValue;
}

void FTangoAreaDescription::SetFilename(const FString NewValue)
{
	Filename = NewValue;
}

FTangoPoseData::FTangoPoseData(FVector NewPosition, FRotator NewRotation, FQuat NewQuatRotation, FTangoCoordinateFramePair NewFrameOfReference,
	TEnumAsByte<ETangoPoseStatus::Type> NewStatusCode, float NewTimestamp) {
	Position = NewPosition;
	Rotation = NewRotation;
	QuatRotation = NewQuatRotation;
	FrameOfReference = NewFrameOfReference;
	StatusCode = NewStatusCode;
	Timestamp = NewTimestamp;
}

FTangoAreaDescriptionMetaData::FTangoAreaDescriptionMetaData(const FString InFileName, const int32 InMillisecondsSinceUnixEpoch, const FTransform InTransformation)
{
	Filename = InFileName;
	MillisecondsSinceUnixEpoch = InMillisecondsSinceUnixEpoch;
	TransformationX		= 0;
	TransformationY		= 0;
	TransformationZ		= 0;
	TransformationQX	= 0;
	TransformationQY	= 0;
	TransformationQZ	= 0;
	TransformationQW	= 1;
}