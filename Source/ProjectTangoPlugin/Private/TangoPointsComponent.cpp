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
#include "TangoPointsComponent.h"

//The Component Implementation

UTangoPointsComponent::UTangoPointsComponent() : Super()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	bUseEditorCompositing = true;
	bGenerateOverlapEvents = false;
}

void UTangoPointsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer()) {
		float LatestTimestamp = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloudTimestamp();
		
		if (Timestamp < LatestTimestamp)
		{
			Timestamp = LatestTimestamp;
			MarkRenderStateDirty();
		}
	}
}

FPrimitiveSceneProxy * UTangoPointsComponent::CreateSceneProxy()
{
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer() && UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud().Num() > 0)
	{
		return new FTangoPointCloudSceneProxy(this);
	}
	return nullptr;
}

FBoxSphereBounds UTangoPointsComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds(FSphere(LocalToWorld.GetLocation(), 100));
}

//The Scene Proxy implementation

FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy(const UTangoPointsComponent * InComponent) : 
	FPrimitiveSceneProxy(InComponent)
{
	//Get all the information we are going to use ready for upload to the Graphics Card.
	PointSize = 3.0;
	Color = FLinearColor(.75, .75, .75);
	DepthPriority = 1;
	//If we specify a material, use it, otherwise use default.
	Material = InComponent->Material ? InComponent->Material : UMaterial::GetDefaultMaterial(MD_Surface);
	
	//Double check we won't cause a seg fault
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer()) 
	{
		FColor initColor = Color.ToFColor(true);

		TArray<FVector> & points = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud();
		const int32 vertexCount = points.Num();
		VertexBuffer.Vertices.SetNumUninitialized(vertexCount);
		IndexBuffer.Indices.SetNumUninitialized(vertexCount);

		//Initialize the Vertex and Index buffers with their data.
		for (int index = 0; index < vertexCount; index++)
		{
			VertexBuffer.Vertices[index].Position = points[index];
			VertexBuffer.Vertices[index].Color = initColor;
			IndexBuffer.Indices[index] = index;
		}

		//Initialise the Vertex Factory with our Vertices.
		VertexFactory.Init(&VertexBuffer);

		//Tell the RHI to initialise the resources on the Graphics Card.
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);
	}

	bWillEverBeLit = true;
	ViewRelevance.bDrawRelevance = true;
	ViewRelevance.bDynamicRelevance = true;
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	ViewRelevance.bSeparateTranslucencyRelevance = ViewRelevance.bNormalTranslucencyRelevance = true;
}

FTangoPointCloudSceneProxy::~FTangoPointCloudSceneProxy()
{
	//Make sure we don't leak!
	VertexBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

void FTangoPointCloudSceneProxy::UpdatePoints_RenderThread()
{
	//@TODO: Figure out a neat way to reuse the existing vertex buffers, rather than
	//trigger a full rebuild of the SceneProxy.
}

void FTangoPointCloudSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily & ViewFamily, uint32 VisibilityMap, FMeshElementCollector & Collector) const
{
	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (VisibilityMap & (1 << ViewIndex))
		{
			//Essentially, which camera we are using
			const FSceneView* View = Views[ViewIndex];
			
			//Ask for a new render batch and link it to our data
			FMeshBatch& Mesh = Collector.AllocateMesh();
			Mesh.VertexFactory = &VertexFactory;
			Mesh.MaterialRenderProxy = Material->GetRenderProxy(IsSelected());
			Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
			Mesh.DepthPriorityGroup = SDPG_World;
			Mesh.bCanApplyViewModeOverrides = false;
			
			//GL_Points render, still need to work out how to set GL_PointSize
			Mesh.Type = PT_PointList;

			//Tell the render object how to index our data.
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = &IndexBuffer;
			
			//Give all the "default" uniforms to help render. (Transform etc.)
			BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = IndexBuffer.Indices.Num() / 3;
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;

			//All done!
			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}
