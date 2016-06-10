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
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: ImageComponent == nullptr"));
		return false;
	}
	if (Material == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Error, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Material == nullptr"));
		return false;
	}
	UMaterialInstanceDynamic* Instance = this->CreateDynamicMaterialInstance(0, Material);
	auto Intrin = UTangoDevice::Get().GetCameraIntrinsics(ETangoCameraType::COLOR);
	if (!UTangoDevice::Get().IsTangoServiceRunning())
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare AR Material since TangoService is not connected"));
		return false;
	}
	else if (UTangoDevice::Get().getTangoDeviceImagePointer() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare Material since Color Camera is not enabled"));
		return false;
	}
	else if (UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture() == nullptr)
	{
		UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Could not prepare Material since camera textures are not ready yet."));
		return false;
	}
	else
	{
		Instance->SetTextureParameterValue(PackedYMaskTextureName, UTangoDevice::Get().getTangoDeviceImagePointer()->GetYTexture());
		Instance->SetTextureParameterValue(PackedUVMaskTextureName, UTangoDevice::Get().getTangoDeviceImagePointer()->GetCrTexture());
		Instance->SetVectorParameterValue(MaterialVectorName, FLinearColor(0, 0, Intrin.Width, Intrin.Height));
		Instance->SetVectorParameterValue(IntrinsicsName, FLinearColor(Intrin.Cx, Intrin.Cy, Intrin.Fx, Intrin.Fy));
		if (Intrin.Distortion.Num() >= 3 || Intrin.CalibrationType != ETangoCalibrationType::POLYNOMIAL_3_PARAMETERS)
		{
			Instance->SetVectorParameterValue(DistortionName, FLinearColor(Intrin.Distortion[0], Intrin.Distortion[1], Intrin.Distortion[2]));
		}
		else
		{
			UE_LOG(ProjectTangoPlugin, Warning, TEXT("UTangoARScreenComponent::SetupMaterial: Unable to retrieve distortion values!"));
		}
		UE_LOG(ProjectTangoPlugin, Log, TEXT("UTangoPointsComponent::PrepareMaterialForPointColoring: Called and working!"));
		return true;
	}
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
	if (UTangoDevice::Get().getTangoDevicePointCloudPointer()) 
	{
		FColor initColor = Color.ToFColor(true);

		TArray<FVector> & points = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetPointCloud();
		const int32 vertexCount = points.Num();

		TArray<int32> trueIJData;
		uint32 RowCount;
		uint32 ColCount;

		VertexBuffer.Vertices.SetNumUninitialized(vertexCount);
		if (!bTriangles)
		{
			IndexBuffer.Indices.SetNumUninitialized(vertexCount);
		}
		else
		{
			RowCount = FMath::FloorToInt(FMath::Sqrt(1250.0f/3.0f) * 16.0f*0.5f);//Camera aspect is 16:9 and max point count is 60000 so sqrt(60000 / (16 *9)) = 20.412
			ColCount = FMath::FloorToInt(FMath::Sqrt(1250.0f / 3.0f) * 9.0f*0.5f);
			trueIJData.SetNumUninitialized(RowCount * ColCount);
			for (uint32 i = 0; i < RowCount * ColCount; ++i)
			{
				trueIJData[i] = -1;
			}
		}

		if (vertexCount > 0)
		{
			MinBounds = points[0];
			MaxBounds = points[0];
		}
		//Initialize the Vertex and Index buffers with their data.
		int32 written = 0;
		int32 unique = 0;
		FVector4 minmaxUV = FVector4(0,0,0,0);
		for (int32 index = 0; index < vertexCount; index++)
		{
			VertexBuffer.Vertices[index].Position = points[index];
			VertexBuffer.Vertices[index].Color = initColor;
			FVector2D texcoord = FVector2D((ProjMat.M[0][0] * points[index].Y + points[index].X * ProjMat.M[2][0]),
										  (-ProjMat.M[1][1] * points[index].Z + points[index].X * ProjMat.M[2][1])) / points[index].X;
			texcoord = texcoord * 0.5f + 0.5f;
			VertexBuffer.Vertices[index].TextureCoordinate = texcoord;
			//UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Calculated %f %f"), texcoord.X, texcoord.Y);
			
			if (texcoord.X != NAN && texcoord.Y != NAN)
			{
				minmaxUV.X = FMath::Max<float>(minmaxUV.X, texcoord.X);
				minmaxUV.Y = FMath::Min<float>(minmaxUV.Y, texcoord.X);
				minmaxUV.Z = FMath::Max<float>(minmaxUV.Z, texcoord.Y);
				minmaxUV.W = FMath::Min<float>(minmaxUV.W, texcoord.Y);
			}

			MinBounds.X = FMath::Min<float>(MinBounds.X, points[index].X);
			MinBounds.Y = FMath::Min<float>(MinBounds.Y, points[index].Y);
			MinBounds.Z = FMath::Min<float>(MinBounds.Z, points[index].Z);
			MaxBounds.X = FMath::Max<float>(MaxBounds.X, points[index].X);
			MaxBounds.Y = FMath::Max<float>(MaxBounds.Y, points[index].Y);
			MaxBounds.Z = FMath::Max<float>(MaxBounds.Z, points[index].Z);

			if (!bTriangles)
			{
				FVector Normal = points[index].GetSafeNormal() * -1.0f;
				VertexBuffer.Vertices[index].TangentX = FPackedNormal(Normal);
				VertexBuffer.Vertices[index].TangentZ = FPackedNormal(FVector::CrossProduct(Normal, FVector(0.0f, 1.0f, 0.0f)));

				IndexBuffer.Indices[index] = index;
			}
			else
			{
				FVector2D ind = texcoord;
				ind.X *= RowCount;
				ind.Y *= ColCount;
				int32 RInd = FMath::RoundToInt(ind.X), CInd = FMath::RoundToInt(ind.Y);
				if (RInd >= 0 && CInd >= 0 && RInd < static_cast<int32>(RowCount)&& CInd < static_cast<int32>(ColCount))
				{
					bool found = false;
					if (trueIJData[RInd + CInd*RowCount] == -1)
					{
						found = true;
						unique++;
					}
					else
					{
						ind.X -= RInd;
						ind.Y -= CInd;
						if (ind.X > 0.25f && RInd + 1 < static_cast<int32>(RowCount))
						{
							if (trueIJData[RInd + 1 + CInd*RowCount] == -1)
							{
								found = true;
								RInd++;
							}
						}
						if (!found && ind.Y > 0.25f && CInd + 1 < static_cast<int32>(ColCount))
						{
							if (trueIJData[RInd + CInd*RowCount + RowCount] == -1)
							{
								found = true;
								CInd++;
							}
						}
						if (!found && ind.X < -0.25f && RInd > 0)
						{
							if (trueIJData[RInd + CInd*RowCount - 1] == -1)
							{
								found = true;
								RInd--;
							}
						}
						if (!found && ind.Y < -0.25f && CInd > 0)
						{
							if (trueIJData[RInd + CInd*RowCount - RowCount] == -1)
							{
								found = true;
								CInd--;
							}
						}
					}
					if (found)
					{
						trueIJData[RInd + CInd*RowCount] = index;
						written++;
					}
				}
			}
		}
		UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: MinMaxUV %f %f %f %f"), minmaxUV.X, minmaxUV.Y, minmaxUV.Z, minmaxUV.W);

		if (bTriangles)
		{
			//int32* IJData;
			//IJData = UTangoDevice::Get().getTangoDevicePointCloudPointer()->GetIJData(RowCount, ColCount);
			static int32 StepIndexes[8][2] = {{0,1},{ 1,1 },{ 1,0 },{ 1,-1 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 }};
			UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Wrote %d unique %d out of %d"), written, unique, vertexCount);
			UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: %d %d"),RowCount,ColCount);
			for (uint32 r = 0; r < RowCount; r++)
			{
				for (uint32 c = 0; c < ColCount; c++)
				{
					if (trueIJData[r + c*RowCount] >= 0 && trueIJData[r + c*RowCount] < vertexCount)
					{
						FVector point = points[trueIJData[r + c*RowCount]];
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
								if (trueIJData[rB + cB*RowCount] >= 0 && trueIJData[rA + cA*RowCount] >= 0 && trueIJData[rA + cA*RowCount] < vertexCount && trueIJData[rB + cB*RowCount] < vertexCount)
								{
									NormalHelper += FVector::CrossProduct(points[trueIJData[rB + cB*RowCount]] - point, points[trueIJData[rA + cA*RowCount]] - point).GetSafeNormal();
									if (i == 1 || i == 2)//In these cases we add a triangle!
									{
										//UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Adding a triangle! %d %d %d"), trueIJData[r + c*RowCount], trueIJData[rB + cB*RowCount], trueIJData[rA + cA*RowCount]);
										IndexBuffer.Indices.Add(trueIJData[r + c*RowCount]);
										IndexBuffer.Indices.Add(trueIJData[rB + cB*RowCount]);
										IndexBuffer.Indices.Add(trueIJData[rA + cA*RowCount]);
									}
								}
							}
							cB = cA;
							rB = rA;
						}
						NormalHelper = NormalHelper.GetSafeNormal();
						VertexBuffer.Vertices[trueIJData[r + c*RowCount]].TangentX = FPackedNormal(NormalHelper);
						VertexBuffer.Vertices[trueIJData[r + c*RowCount]].TangentZ = FPackedNormal(FVector::CrossProduct(NormalHelper, FVector(0.0f, 1.0f, 0.0f)));
					}
				}
			}
			UE_LOG(ProjectTangoPlugin, Log, TEXT("FTangoPointCloudSceneProxy::FTangoPointCloudSceneProxy: Indexcount %d"), IndexBuffer.Indices.Num());
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
