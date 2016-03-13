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
#include "TangoDeviceImage.h"
#include "TangoDevice.h"

void UTangoDeviceImage::Init(
#if PLATFORM_ANDROID
	TangoConfig config_
#endif
	)
{
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage: Constructer called"));

	/*bIsImageBufferSet = false;
	ImageBufferWidth = 0;
	ImageBufferHeight = 0;*/

	ImageBufferTimestamp = 0;
	bTexturesHaveDataInThem = false;
	CallbackConnected = false;
	WantToConnectCallbacks = false;
	NewDataAvailable = false;
#if PLATFORM_ANDROID
	CreateYUVTextures(config_);
#endif
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage: Constructer finished"));
}

bool UTangoDeviceImage::CreateYUVTextures(
#if PLATFORM_ANDROID
	TangoConfig config_
#endif
	)
{
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage: CreateYUVTextures called"));
#if PLATFORM_ANDROID
	int32 YTextureWidth = 0;
	int32 YTextureHeight = 0;
	int32 uvTextureWidth = 0;
	int32 uvTextureHeight = 0;

	bool success = true;
	success = TangoConfig_getInt32(config_,"experimental_color_y_tex_data_width", &YTextureWidth) == TANGO_SUCCESS && success;
	success = TangoConfig_getInt32(config_, "experimental_color_y_tex_data_height", &YTextureHeight) == TANGO_SUCCESS && success;
	success = TangoConfig_getInt32(config_, "experimental_color_uv_tex_data_width", &uvTextureWidth) == TANGO_SUCCESS && success;
	success = TangoConfig_getInt32(config_, "experimental_color_uv_tex_data_height", &uvTextureHeight) == TANGO_SUCCESS && success;

	if (!success || YTextureWidth == 0 || YTextureHeight == 0 || uvTextureWidth == 0 || uvTextureHeight == 0)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("Fetched camera texture sizes are invalid"));
		return false;
	}
	else
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDevice::Get().YTextureWidth %d UTangoDevice::Get().YTextureHeight %d uvTextureWidth %d uvTextureHeight %d"), YTextureWidth, YTextureHeight, uvTextureWidth, uvTextureHeight);

	if (UTangoDevice::Get().YTexture == nullptr)
	{
		UTangoDevice::Get().YTexture = UTexture2D::CreateTransient(YTextureWidth, YTextureHeight, PF_R8G8B8A8);
		UTangoDevice::Get().CrTexture = UTexture2D::CreateTransient(uvTextureWidth, uvTextureHeight, PF_R8G8B8A8);
		UTangoDevice::Get().CbTexture = UTexture2D::CreateTransient(uvTextureWidth, uvTextureHeight, PF_R8G8B8A8);

		UTangoDevice::Get().YTexture->Filter = TF_Nearest;
		UTangoDevice::Get().CrTexture->Filter = TF_Nearest;
		UTangoDevice::Get().CbTexture->Filter = TF_Nearest;

		UTangoDevice::Get().YTexture->CompressionSettings = TC_Masks;
		UTangoDevice::Get().CrTexture->CompressionSettings = TC_Masks;
		UTangoDevice::Get().CbTexture->CompressionSettings = TC_Masks;

		UTangoDevice::Get().YTexture->SRGB = 0;
		UTangoDevice::Get().CrTexture->SRGB = 0;
		UTangoDevice::Get().CbTexture->SRGB = 0;

		UTangoDevice::Get().YTexture->UpdateResource();
		UTangoDevice::Get().CrTexture->UpdateResource();
		UTangoDevice::Get().CbTexture->UpdateResource();
	}


#endif
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage: CreateYUVTextures FINISHED"));
	return true;
}

bool UTangoDeviceImage::setRuntimeConfig(FTangoRuntimeConfig & runtimeConfig)
{
	if (runtimeConfig.EnableColorCamera)
	{
		if (!CallbackConnected)
		{
			ConnectCallback();
		}
		return true;
	}
	else if(CallbackConnected)
	{
		return DisconnectCallback();
	}
	else if (WantToConnectCallbacks)
	{
		WantToConnectCallbacks = false;
		return true;
	}
	else
	{
		return true;
	}
}

bool UTangoDeviceImage::DisconnectCallback()
{
	if (!CallbackConnected)
	{
		return true;
	}
	else
	{
		bool success = true;
#if PLATFORM_ANDROID
		success = TangoService_disconnectCamera(TANGO_CAMERA_COLOR) == TANGO_SUCCESS;
#endif
		if (success == true)
		{
			CallbackConnected = false;
		}
		return success;
	}
}

void UTangoDeviceImage::OnNewDataAvailable()
{
	NewDataAvailable = true;
}

void UTangoDeviceImage::ConnectCallback()
{
	WantToConnectCallbacks = true;
}

bool UTangoDeviceImage::TexturesReady()
{
	if (!(UTangoDevice::Get().YTexture || UTangoDevice::Get().CrTexture || UTangoDevice::Get().CbTexture))
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: One of the Textures is null"));
		return false;
	}
	if (!(UTangoDevice::Get().YTexture->IsValidLowLevel() || UTangoDevice::Get().CrTexture->IsValidLowLevel() || UTangoDevice::Get().CbTexture->IsValidLowLevel()))
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: RegisterCallbacks Not IsValidLowLevel"));
		return false;
	}
	if (!(UTangoDevice::Get().YTexture->Resource || UTangoDevice::Get().CrTexture->Resource || UTangoDevice::Get().CbTexture->Resource))//@TODO: Really neccessary?
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: RegisterCallbacks Not Resource"));
		return false;
	}

	if (!UTangoDevice::Get().YTexture->TextureReference.IsInitialized_GameThread())
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: UTangoDevice::Get().YTexture not ready yet"));
		return false;
	}
	if (!UTangoDevice::Get().CrTexture->TextureReference.IsInitialized_GameThread())
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: UTangoDevice::Get().CrTexture not ready yet"));
		return false;
	}
	if (!UTangoDevice::Get().CbTexture->TextureReference.IsInitialized_GameThread())
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::TexturesReady: UTangoDeviceImage: UTangoDevice::Get().CbTexture not ready yet"));
		return false;
	}
	return true;
}

void UTangoDeviceImage::CheckConnectCallback()
{
	if (!WantToConnectCallbacks)
	{
		return;
	}
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks called"))
#if PLATFORM_ANDROID
	if (!TexturesReady())
	{
		return;
	}

	//FTextureRHIRef YTex = UTangoDevice::Get().YTexture->Resource->TextureRHI;
	//FTextureRHIRef CrTex = UTangoDevice::Get().CrTexture->Resource->TextureRHI;
	//FTextureRHIRef CbTex = UTangoDevice::Get().CbTexture->Resource->TextureRHI;

	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(ConnectTangoCallback,
		FTextureRHIRef, YTex, UTangoDevice::Get().YTexture->Resource->TextureRHI,
		FTextureRHIRef, CrTex, UTangoDevice::Get().CrTexture->Resource->TextureRHI,
		FTextureRHIRef, CbTex, UTangoDevice::Get().CbTexture->Resource->TextureRHI,
		{
			if (!(YTex || CrTex || CbTex))//@TODO: Really neccessary?
			{
				UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks Not TextureRHI"));
				return;
			}

			UE_LOG(LogTemp, Log, TEXT("UTangoDeviceImage::CheckConnectCallback: Fetching Texture Pointers"));
			void* YRes = YTex->GetNativeResource();
			void* CrRes = CrTex->GetNativeResource();
			void* CbRes = CbTex->GetNativeResource();
			if (YRes == nullptr || CrRes == nullptr || CbRes == nullptr)//@TODO: Really neccessary?
			{
				UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks Not GetNativeResource"));
				return;
			}
			uint32  YOpenGLPointer = static_cast<uint32>(*reinterpret_cast<int32*>(YRes));
			uint32 CrOpenGLPointer = static_cast<uint32>(*reinterpret_cast<int32*>(CrRes));
			uint32 CbOpenGLPointer = static_cast<uint32>(*reinterpret_cast<int32*>(CbRes));
			UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceImage::CheckConnectCallback: Registering Callback"));

			//TangoService_connectTextureId(TANGO_CAMERA_COLOR, YOpenGLPointer, this,
			//	[](void*, TangoCameraId id) {if (id == TANGO_CAMERA_COLOR && UTangoDevice::Get().getTangoDeviceImagePointer() != nullptr) UTangoDevice::Get().getTangoDeviceImagePointer()->OnNewDataAvailable(); }
			//);
			TangoService_Experimental_connectTextureIdUnity(TANGO_CAMERA_COLOR, YOpenGLPointer, CbOpenGLPointer, CrOpenGLPointer, this,
				[](void*, TangoCameraId id) {if (id == TANGO_CAMERA_COLOR && UTangoDevice::Get().getTangoDeviceImagePointer() != nullptr) UTangoDevice::Get().getTangoDeviceImagePointer()->OnNewDataAvailable(); }
			);
		});
#endif
	WantToConnectCallbacks = false;
	CallbackConnected = true;
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::CheckConnectCallback: UTangoDeviceImage: RegisterCallbacks FINISHED"));
}

void UTangoDeviceImage::TickByDevice()
{
	CheckConnectCallback();
#if PLATFORM_ANDROID
	if (NewDataAvailable)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND(TangoUpdateTexture,
		{
			if (TangoService_updateTexture(TANGO_CAMERA_COLOR, &ImageBufferTimestamp) != TANGO_SUCCESS)
			{
				UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoDeviceImage::TickByDevice: Tango Texture Update FAILED"));
			}
			else
			{
				bTexturesHaveDataInThem = true;
			}
		});
		NewDataAvailable = false;
	}
#endif
}

UTexture* UTangoDeviceImage::GetYTexture()
{
	if (!bTexturesHaveDataInThem)
	{
		return nullptr;
	}
	else
	{
		return UTangoDevice::Get().YTexture;
	}
}

UTexture* UTangoDeviceImage::GetCrTexture()
{
	if (!bTexturesHaveDataInThem)
	{
		return nullptr;
	}
	else
	{
		return UTangoDevice::Get().CrTexture;
	}
}

UTexture* UTangoDeviceImage::GetCbTexture()
{
	if (!bTexturesHaveDataInThem)
	{
		return nullptr;
	}
	else
	{
		return UTangoDevice::Get().CbTexture;
	}
}

void UTangoDeviceImage::BeginDestroy()
{
	Super::BeginDestroy();
	UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoDeviceImage::BeginDestroy: destructor called"));
	DisconnectCallback();
}

float UTangoDeviceImage::GetImageBufferTimestamp()
{
	float ReturnValue = 0;
#if PLATFORM_ANDROID
	ReturnValue = ImageBufferTimestamp;
#endif
	return ReturnValue;
}