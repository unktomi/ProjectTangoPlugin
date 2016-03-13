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

#include "SceneViewExtension.h"
#include "Delegates/Delegate.h"
//#include "CriticalSection.h"
/** View extension object that can persist on the render thread without the components */
class FTangoViewExtension : public ISceneViewExtension, public TSharedFromThis<FTangoViewExtension, ESPMode::ThreadSafe>
{
public:
	FTangoViewExtension() {}
	virtual ~FTangoViewExtension() {}

	/** ISceneViewExtension interface */
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override { PreRenderEvent.Broadcast(); };
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) override {}

	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) override 
	{
		CriticalSection.Lock();
		PreRenderEventRenderThread.Broadcast();
		CriticalSection.Unlock();
	};


	/** Broadcasts on the rendering thread on prerender */
	DECLARE_EVENT(FTangoViewExtension, FPreRender)
	FPreRender& OnPreRender() { return PreRenderEvent; }

	DECLARE_EVENT(FTangoViewExtension, FPreRenderRenderThread)
	FDelegateHandle AddOnPreRenderRenderThread(UObject* InUserObject, FName Name)
	{
		CriticalSection.Lock();
		FDelegateHandle handle = PreRenderEventRenderThread.AddUFunction(InUserObject, Name);
		CriticalSection.Unlock();
		return handle;
	}

	void RemoveOnPreRenderRenderThread(FDelegateHandle Method) 
	{
		CriticalSection.Lock();
		PreRenderEventRenderThread.Remove(Method);
		CriticalSection.Unlock();
	}
private:
	FPreRender PreRenderEvent;
	FPreRenderRenderThread PreRenderEventRenderThread;
	FCriticalSection CriticalSection;
};