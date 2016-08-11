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
#include "TangoDevice.h"
#include "TangoPointsComponent.h"
#include "TangoImageComponent.h"
#include "TangoARHelpers.h"

//The Component Implementation

UTangoPointsComponent::UTangoPointsComponent() : Super()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	bUseEditorCompositing = true;
	bGenerateOverlapEvents = false;
}

bool UTangoPointsComponent::PrepareMaterialForPointColoring(UTangoImageComponent * ImageComponent, UMaterial* Material, FName PackedYMaskTextureName, FName PackedUVMaskTextureName, FName MaterialVectorName, FName IntrinsicsName,FName DistortionName)
{
	if (ImageComponent == nullptr)
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: ImageComponent == nullptr"));
		return false;
	}
	if (Material == nullptr)
	{
		UE_LOG(TangoPlugin, Error, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Material == nullptr"));
		return false;
	}
	UMaterialInstanceDynamic* Instance = this->CreateDynamicMaterialInstance(0, Material);
	auto Intrin = UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR);
	if (!UTangoDevice::Get().IsTangoServiceRunning())
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare AR Material since TangoService is not connected"));
		return false;
	}
	else if (UTangoDevice::Get().GetTangoDeviceImagePointer() == nullptr)
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare Material since Color Camera is not enabled"));
		return false;
	}
	else if (UTangoDevice::Get().GetTangoDeviceImagePointer()->GetYTexture() == nullptr)
	{
		UE_LOG(TangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare Material since camera textures are not ready yet."));
		return false;
	}
	else
	{
		Instance->SetTextureParameterValue(PackedYMaskTextureName, UTangoDevice::Get().GetTangoDeviceImagePointer()->GetYTexture());
		Instance->SetTextureParameterValue(PackedUVMaskTextureName, UTangoDevice::Get().GetTangoDeviceImagePointer()->GetCrTexture());
		Instance->SetVectorParameterValue(MaterialVectorName, FLinearColor(0, 0, Intrin.Width, Intrin.Height));
		Instance->SetVectorParameterValue(IntrinsicsName, FLinearColor(Intrin.Cx, Intrin.Cy, Intrin.Fx, Intrin.Fy));
		if (Intrin.Distortion.Num() >= 3 || Intrin.CalibrationType != ETangoCalibrationType::POLYNOMIAL_3_PARAMETERS)
		{
			Instance->SetVectorParameterValue(DistortionName, FLinearColor(Intrin.Distortion[0], Intrin.Distortion[1], Intrin.Distortion[2]));
		}
		else
		{
			UE_LOG(TangoPlugin, Warning, TEXT("UTangoARScreenComponent::SetupMaterial: Unable to retrieve distortion values!"));
		}
		UE_LOG(TangoPlugin, Log, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Called and working!"));
		return true;
	}
}

void UTangoPointsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer()) {
		float LatestTimestamp = UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetPointCloudTimestamp();
		
		if (Timestamp < LatestTimestamp)
		{
			Timestamp = LatestTimestamp;
			MarkRenderStateDirty();
		}
	}
}

FPrimitiveSceneProxy * UTangoPointsComponent::CreateSceneProxy()
{
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer() && UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetPointCloud().Num() > 0)
	{
		return new FTangoPointCloudSceneProxy(this, MinBounds, MaxBounds, TangoARHelpers::GetUnadjustedProjectionMatrix(), bExperimentalMeshGeneration);
	}
	return nullptr;
}

FBoxSphereBounds UTangoPointsComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FVector Pos = LocalToWorld.GetLocation() + LocalToWorld.GetRotation() * ((MinBounds + MaxBounds)*0.5f) * LocalToWorld.GetScale3D();
	float r = FVector::Dist(MinBounds, MaxBounds) * 0.5f * LocalToWorld.GetScale3D().GetMax();
	return FBoxSphereBounds(FSphere(Pos, r));
}

//The Scene Proxy implementation

FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy(const UTangoPointsComponent * InComponent, FVector& MinBounds, FVector& MaxBounds, const FMatrix& ProjMat, const bool bCreateTriangles) :
	FPrimitiveSceneProxy(InComponent)
{
	bTriangles = bCreateTriangles;
	//Get all the information we are going to use ready for upload to the Graphics Card.
	PointSize = 3.0;
	Color = FLinearColor(1, 1, 1);
	DepthPriority = 1;
	//If we specify a material, use it, otherwise use default.
	Material = InComponent->GetMaterial(0) ? InComponent->GetMaterial(0) : UMaterial::GetDefaultMaterial(MD_Surface);
	
	//Double check we won't cause a seg fault
	if (UTangoDevice::Get().GetTangoDevicePointCloudPointer()) 
	{
		FColor initColor = Color.ToFColor(true);

		TArray<FVector> & Points = UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetPointCloud();
		const int32 vertexCount = Points.Num();

		TArray<int32> TrueIJData;
		uint32 RowCount = 0;
		uint32 ColCount = 0;

		VertexBuffer.Vertices.SetNumUninitialized(vertexCount);
		if (!bTriangles)
		{
			IndexBuffer.Indices.SetNumUninitialized(vertexCount);
		}
		else
		{
			RowCount = FMath::FloorToInt(FMath::Sqrt(1250.0f/3.0f) * 16.0f*0.5f);//Camera aspect is 16:9 and max point count is 60000 so sqrt(60000 / (16 *9)) = 20.412
			ColCount = FMath::FloorToInt(FMath::Sqrt(1250.0f / 3.0f) * 9.0f*0.5f);
			TrueIJData.SetNumUninitialized(RowCount * ColCount);
			for (uint32 i = 0; i < RowCount * ColCount; ++i)
			{
				TrueIJData[i] = -1;
			}
		}

		if (vertexCount > 0)
		{
			MinBounds = Points[0];
			MaxBounds = Points[0];
		}
		//Initialize the Vertex and Index buffers with their data.
		int32 Written = 0;
		int32 Unique = 0;
		FVector4 MinmaxUV = FVector4(0,0,0,0);
		for (int32 Index = 0; Index < vertexCount; Index++)
		{
			VertexBuffer.Vertices[Index].Position = Points[Index];
			VertexBuffer.Vertices[Index].Color = initColor;
			FVector2D Texcoord = FVector2D((ProjMat.M[0][0] * Points[Index].Y + Points[Index].X * ProjMat.M[2][0]),
										  (-ProjMat.M[1][1] * Points[Index].Z + Points[Index].X * ProjMat.M[2][1])) / Points[Index].X;
			Texcoord = Texcoord * 0.5f + 0.5f;
			VertexBuffer.Vertices[Index].TextureCoordinate = Texcoord;
			//UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Calculated %f %f"), Texcoord.X, Texcoord.Y);
			
			if (Texcoord.X != NAN && Texcoord.Y != NAN)
			{
				MinmaxUV.X = FMath::Max<float>(MinmaxUV.X, Texcoord.X);
				MinmaxUV.Y = FMath::Min<float>(MinmaxUV.Y, Texcoord.X);
				MinmaxUV.Z = FMath::Max<float>(MinmaxUV.Z, Texcoord.Y);
				MinmaxUV.W = FMath::Min<float>(MinmaxUV.W, Texcoord.Y);
			}

			MinBounds.X = FMath::Min<float>(MinBounds.X, Points[Index].X);
			MinBounds.Y = FMath::Min<float>(MinBounds.Y, Points[Index].Y);
			MinBounds.Z = FMath::Min<float>(MinBounds.Z, Points[Index].Z);
			MaxBounds.X = FMath::Max<float>(MaxBounds.X, Points[Index].X);
			MaxBounds.Y = FMath::Max<float>(MaxBounds.Y, Points[Index].Y);
			MaxBounds.Z = FMath::Max<float>(MaxBounds.Z, Points[Index].Z);

			if (!bTriangles)
			{
				FVector Normal = Points[Index].GetSafeNormal() * -1.0f;
				VertexBuffer.Vertices[Index].TangentX = FPackedNormal(Normal);
				VertexBuffer.Vertices[Index].TangentZ = FPackedNormal(FVector::CrossProduct(Normal, FVector(0.0f, 1.0f, 0.0f)));

				IndexBuffer.Indices[Index] = Index;
			}
			else
			{
				FVector2D Ind = Texcoord;
				Ind.X *= RowCount;
				Ind.Y *= ColCount;
				int32 RInd = FMath::RoundToInt(Ind.X), CInd = FMath::RoundToInt(Ind.Y);
				if (RInd >= 0 && CInd >= 0 && RInd < static_cast<int32>(RowCount)&& CInd < static_cast<int32>(ColCount))
				{
					bool bFound = false;
					if (TrueIJData[RInd + CInd*RowCount] == -1)
					{
						bFound = true;
						Unique++;
					}
					else
					{
						Ind.X -= RInd;
						Ind.Y -= CInd;
						if (Ind.X > 0.25f && RInd + 1 < static_cast<int32>(RowCount))
						{
							if (TrueIJData[RInd + 1 + CInd*RowCount] == -1)
							{
								bFound = true;
								RInd++;
							}
						}
						if (!bFound && Ind.Y > 0.25f && CInd + 1 < static_cast<int32>(ColCount))
						{
							if (TrueIJData[RInd + CInd*RowCount + RowCount] == -1)
							{
								bFound = true;
								CInd++;
							}
						}
						if (!bFound && Ind.X < -0.25f && RInd > 0)
						{
							if (TrueIJData[RInd + CInd*RowCount - 1] == -1)
							{
								bFound = true;
								RInd--;
							}
						}
						if (!bFound && Ind.Y < -0.25f && CInd > 0)
						{
							if (TrueIJData[RInd + CInd*RowCount - RowCount] == -1)
							{
								bFound = true;
								CInd--;
							}
						}
					}
					if (bFound)
					{
						TrueIJData[RInd + CInd*RowCount] = Index;
						Written++;
					}
				}
			}
		}
		UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: MinMaxUV %f %f %f %f"), MinmaxUV.X, MinmaxUV.Y, MinmaxUV.Z, MinmaxUV.W);

		if (bTriangles)
		{
			//int32* IJData;
			//IJData = UTangoDevice::Get().GetTangoDevicePointCloudPointer()->GetIJData(RowCount, ColCount);
			static int32 StepIndexes[8][2] = {{0,1},{ 1,1 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 }};
			UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Wrote %d Unique %d out of %d"), Written, Unique, vertexCount);
			UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: %d %d"),RowCount,ColCount);
			for (uint32 r = 0; r < RowCount; r++)
			{
				for (uint32 c = 0; c < ColCount; c++)
				{
					if (TrueIJData[r + c*RowCount] >= 0 && TrueIJData[r + c*RowCount] < vertexCount)
					{
						FVector point = Points[TrueIJData[r + c*RowCount]];
						int32 rB = r + StepIndexes[7][0];
						int32 cB = c + StepIndexes[7][1];
						FVector NormalHelper = FVector::ZeroVector;
						uint32 NormalHelperCount = 0;
						for (uint32 i = 0; i < 8; ++i)
						{
							int32 rA = r + StepIndexes[i][0];
							int32 cA = c + StepIndexes[i][1];
							if (rA >= 0 && rA < static_cast<int32>(RowCount) && cA >= 0 && cA < static_cast<int32>(ColCount) && rB >= 0 && rB < static_cast<int32>(RowCount) && cB >= 0 && cB < static_cast<int32>(ColCount))
							{
								if (TrueIJData[rB + cB*RowCount] >= 0 && TrueIJData[rA + cA*RowCount] >= 0 && TrueIJData[rA + cA*RowCount] < vertexCount && TrueIJData[rB + cB*RowCount] < vertexCount)
								{
									NormalHelper += FVector::CrossProduct(Points[TrueIJData[rB + cB*RowCount]] - point, Points[TrueIJData[rA + cA*RowCount]] - point).GetSafeNormal();
									if (i == 1 || i == 2)//In these cases we add a triangle!
									{
										//UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Adding a triangle! %d %d %d"), TrueIJData[r + c*RowCount], TrueIJData[rB + cB*RowCount], TrueIJData[rA + cA*RowCount]);
										IndexBuffer.Indices.Add(TrueIJData[r + c*RowCount]);
										IndexBuffer.Indices.Add(TrueIJData[rB + cB*RowCount]);
										IndexBuffer.Indices.Add(TrueIJData[rA + cA*RowCount]);
									}
								}
							}
							cB = cA;
							rB = rA;
						}
						NormalHelper = NormalHelper.GetSafeNormal();
						VertexBuffer.Vertices[TrueIJData[r + c*RowCount]].TangentX = FPackedNormal(NormalHelper);
						VertexBuffer.Vertices[TrueIJData[r + c*RowCount]].TangentZ = FPackedNormal(FVector::CrossProduct(NormalHelper, FVector(0.0f, 1.0f, 0.0f)));
					}
				}
			}
			UE_LOG(TangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Indexcount %d"), IndexBuffer.Indices.Num());
		}
		int32 count = 0;
		while(IndexBuffer.Indices.Num()<9)
			IndexBuffer.Indices.Add(count++);
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
			Mesh.Type = bTriangles ? PT_TriangleList : PT_PointList;

			//Tell the render object how to index our data.
			FMeshBatchElement& BatchElement = Mesh.Elements[0];
			BatchElement.IndexBuffer = &IndexBuffer;
			
			//Give all the "default" uniforms to help render. (Transform etc.)
			BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
			BatchElement.FirstIndex = 0;
			BatchElement.NumPrimitives = bTriangles ? IndexBuffer.Indices.Num() / 3 : IndexBuffer.Indices.Num();
			BatchElement.MinVertexIndex = 0;
			BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;

			//All done!
			Collector.AddMesh(ViewIndex, Mesh);
		}
	}
}
