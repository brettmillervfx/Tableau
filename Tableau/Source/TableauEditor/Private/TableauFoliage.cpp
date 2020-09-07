
#include "TableauFoliage.h"

// Engine Includes
#include "EngineUtils.h"
#include "FoliageType.h"
#include "InstancedFoliageActor.h"
#include "FoliageType_InstancedStaticMesh.h"

// Local Includes
#include "TableauUtils.h"

FTableauFoliage::FTableauFoliage(UFoliageType* InFoliageType, const FTransform& Xform, const FName& InName, bool bIsSnappable = true)
	: Name(InName)
	, bSnappable(bIsSnappable)
	, bAlignToSurfaceNormal(false)
{ 
	FoliageType = TWeakObjectPtr<UFoliageType>(InFoliageType);
	Instances.Empty();
	Instances.Add(Xform);
}

FTableauFoliage::~FTableauFoliage()
{
	Instances.Empty();
}

void FTableauFoliage::AddInstance(const FTransform& Transform)
{
	Instances.Add(Transform);
}

void FTableauFoliage::ConfigureAndAttachHISM(ATableauActor* Actor)
{
	check(Actor);
	UTableauHISMComponent* HISMComponent = NewObject<UTableauHISMComponent>(Actor, UTableauHISMComponent::StaticClass(), NAME_None, RF_Transactional);

	check(HISMComponent);

	if (FoliageType.IsValid())
	{
		HISMComponent->FoliageType = FSoftObjectPath(FoliageType.Get());
		UpdateComponentSettings(HISMComponent);
	}

	HISMComponent->SetupAttachment(Actor->GetRootComponent());

	if (Actor->GetRootComponent()->IsRegistered())
	{
		HISMComponent->RegisterComponent();
		Actor->AddInstanceComponent(HISMComponent);
		Actor->FoliageComponents.Add(HISMComponent);
	}

	// Use only instance translation as a component transform
	HISMComponent->SetWorldTransform(Actor->GetRootComponent()->GetComponentTransform());

	HISMComponent->bSnappable = bSnappable;

	// Exclude from HLOD Generation?
	if (Actor->GetTableauComponent()->bExcludeHISMsFromHLOD)
	{
		HISMComponent->ExcludeFromHLOD();
	}

	// Add the new component to the transaction buffer so it will get destroyed on undo
	HISMComponent->Modify();
	// We don't want to track changes to instances later so we mark it as non-transactional
	HISMComponent->ClearFlags(RF_Transactional);
}

void FTableauFoliage::UpdateComponentSettings(UTableauHISMComponent* HISMComponent)
{
	// Configure the HISM to the FoliageType
	RefreshComponentProperties(HISMComponent);

	// Add the instances
	HISMComponent->PreAllocateInstancesMemory(Instances.Num());
	for (const FTransform& Xform : Instances)
	{
		HISMComponent->AddInstance(Xform);
	}
}

void FTableauFoliage::RefreshComponentProperties(UTableauHISMComponent* Component)
{
	const UFoliageType_InstancedStaticMesh* FoliageType_InstancedStaticMesh = Cast<UFoliageType_InstancedStaticMesh>(FoliageType);
	
	if (Component)
	{
		bool bNeedsMarkRenderStateDirty = false;
		bool bNeedsInvalidateLightingCache = false;

		if (Component->GetStaticMesh() != FoliageType_InstancedStaticMesh->GetStaticMesh())
		{
			Component->SetStaticMesh(FoliageType_InstancedStaticMesh->GetStaticMesh());

			bNeedsInvalidateLightingCache = true;
			bNeedsMarkRenderStateDirty = true;
		}

		if (Component->Mobility != FoliageType_InstancedStaticMesh->Mobility)
		{
			Component->SetMobility(FoliageType_InstancedStaticMesh->Mobility);
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->InstanceStartCullDistance != FoliageType_InstancedStaticMesh->CullDistance.Min)
		{
			Component->InstanceStartCullDistance = FoliageType_InstancedStaticMesh->CullDistance.Min;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->InstanceEndCullDistance != FoliageType_InstancedStaticMesh->CullDistance.Max)
		{
			Component->InstanceEndCullDistance = FoliageType_InstancedStaticMesh->CullDistance.Max;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->CastShadow != FoliageType_InstancedStaticMesh->CastShadow)
		{
			Component->CastShadow = FoliageType_InstancedStaticMesh->CastShadow;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bCastDynamicShadow != FoliageType_InstancedStaticMesh->bCastDynamicShadow)
		{
			Component->bCastDynamicShadow = FoliageType_InstancedStaticMesh->bCastDynamicShadow;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bCastStaticShadow != FoliageType_InstancedStaticMesh->bCastStaticShadow)
		{
			Component->bCastStaticShadow = FoliageType_InstancedStaticMesh->bCastStaticShadow;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->RuntimeVirtualTextures != FoliageType_InstancedStaticMesh->RuntimeVirtualTextures)
		{
			Component->RuntimeVirtualTextures = FoliageType_InstancedStaticMesh->RuntimeVirtualTextures;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->VirtualTextureRenderPassType != FoliageType_InstancedStaticMesh->VirtualTextureRenderPassType)
		{
			Component->VirtualTextureRenderPassType = FoliageType_InstancedStaticMesh->VirtualTextureRenderPassType;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->VirtualTextureCullMips != FoliageType_InstancedStaticMesh->VirtualTextureCullMips)
		{
			Component->VirtualTextureCullMips = FoliageType_InstancedStaticMesh->VirtualTextureCullMips;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->TranslucencySortPriority != FoliageType_InstancedStaticMesh->TranslucencySortPriority)
		{
			Component->TranslucencySortPriority = FoliageType_InstancedStaticMesh->TranslucencySortPriority;
			bNeedsMarkRenderStateDirty = true;
		}
		if (Component->bAffectDynamicIndirectLighting != FoliageType_InstancedStaticMesh->bAffectDynamicIndirectLighting)
		{
			Component->bAffectDynamicIndirectLighting = FoliageType_InstancedStaticMesh->bAffectDynamicIndirectLighting;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bAffectDistanceFieldLighting != FoliageType_InstancedStaticMesh->bAffectDistanceFieldLighting)
		{
			Component->bAffectDistanceFieldLighting = FoliageType_InstancedStaticMesh->bAffectDistanceFieldLighting;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bCastShadowAsTwoSided != FoliageType_InstancedStaticMesh->bCastShadowAsTwoSided)
		{
			Component->bCastShadowAsTwoSided = FoliageType_InstancedStaticMesh->bCastShadowAsTwoSided;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bReceivesDecals != FoliageType_InstancedStaticMesh->bReceivesDecals)
		{
			Component->bReceivesDecals = FoliageType_InstancedStaticMesh->bReceivesDecals;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bOverrideLightMapRes != FoliageType_InstancedStaticMesh->bOverrideLightMapRes)
		{
			Component->bOverrideLightMapRes = FoliageType_InstancedStaticMesh->bOverrideLightMapRes;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->OverriddenLightMapRes != FoliageType_InstancedStaticMesh->OverriddenLightMapRes)
		{
			Component->OverriddenLightMapRes = FoliageType_InstancedStaticMesh->OverriddenLightMapRes;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->LightmapType != FoliageType_InstancedStaticMesh->LightmapType)
		{
			Component->LightmapType = FoliageType_InstancedStaticMesh->LightmapType;
			bNeedsMarkRenderStateDirty = true;
			bNeedsInvalidateLightingCache = true;
		}
		if (Component->bUseAsOccluder != FoliageType_InstancedStaticMesh->bUseAsOccluder)
		{
			Component->bUseAsOccluder = FoliageType_InstancedStaticMesh->bUseAsOccluder;
			bNeedsMarkRenderStateDirty = true;
		}

		if (Component->bEnableDensityScaling != FoliageType_InstancedStaticMesh->bEnableDensityScaling)
		{
			Component->bEnableDensityScaling = FoliageType_InstancedStaticMesh->bEnableDensityScaling;

			Component->UpdateDensityScaling();

			bNeedsMarkRenderStateDirty = true;
		}

		if (GetLightingChannelMaskForStruct(Component->LightingChannels) != GetLightingChannelMaskForStruct(FoliageType_InstancedStaticMesh->LightingChannels))
		{
			Component->LightingChannels = FoliageType_InstancedStaticMesh->LightingChannels;
			bNeedsMarkRenderStateDirty = true;
		}

		if (Component->bRenderCustomDepth != FoliageType_InstancedStaticMesh->bRenderCustomDepth)
		{
			Component->bRenderCustomDepth = FoliageType_InstancedStaticMesh->bRenderCustomDepth;
			bNeedsMarkRenderStateDirty = true;
		}

		if (Component->CustomDepthStencilValue != FoliageType_InstancedStaticMesh->CustomDepthStencilValue)
		{
			Component->CustomDepthStencilValue = FoliageType_InstancedStaticMesh->CustomDepthStencilValue;
			bNeedsMarkRenderStateDirty = true;
		}

		if (FoliageType_InstancedStaticMesh->AlignToNormal)
		{
			Component->bAlignToNormal = true;
		}

		Component->GroundSlopeAngle = FoliageType_InstancedStaticMesh->GroundSlopeAngle;

		Component->BodyInstance.CopyBodyInstancePropertiesFrom(&FoliageType_InstancedStaticMesh->BodyInstance);

		Component->SetCustomNavigableGeometry(FoliageType_InstancedStaticMesh->CustomNavigableGeometry);

		if (bNeedsInvalidateLightingCache)
		{
			Component->InvalidateLightingCache();
		}

		if (bNeedsMarkRenderStateDirty)
		{
			Component->MarkRenderStateDirty();
		}
	}

}


FTableauFoliageManager::FTableauFoliageManager(ATableauActor* InActor) :
	TableauActor(InActor)
{
	TableauActor->FoliageComponents.Empty();
	
	if (TableauActor.IsValid())
	{
		// Restore Actor's Foliage Component list.
		TArray<UTableauHISMComponent*> Foliages;
		TableauActor->GetComponents(Foliages, false);
		for (UTableauHISMComponent* Foliage : Foliages)
		{
			TableauActor->FoliageComponents.Add(Foliage);
		}
	}

}

void FTableauFoliageManager::DestroyFoliageComponents()
{
	if (TableauActor.IsValid())
	{
		for (UTableauHISMComponent* Component : TableauActor->FoliageComponents)
		{
			TableauActor->RemoveInstanceComponent(Component);
			Component->DestroyComponent(false);
		}
		TableauActor->FoliageComponents.Empty();
	}
}

void FTableauFoliageManager::SnapToFloor(const UWorld* InWorld)
{
	if (TableauActor.IsValid())
	{
		for (UTableauHISMComponent* Component : TableauActor->FoliageComponents)
		{
			if (Component->bSnappable)
			{
				SnapInstancesToFloor(Component, InWorld);
			}
		}
	}
}

void FTableauFoliageManager::SnapInstancesToFloor(UTableauHISMComponent* Component, const UWorld* InWorld)
{
	
	const int32 InstanceCount = Component->GetInstanceCount();
	TArray<FTransform> InstanceWorldTransforms;
	InstanceWorldTransforms.Empty(InstanceCount);

	// Get the world space transform of each instance and trace it to the world.
	for (int32 i = 0; i < InstanceCount; ++i)
	{
		FTransform InstanceWorldTransform;
		Component->GetInstanceTransform(i, InstanceWorldTransform, true);
		if (FTableauUtils::TraceToWorld(InstanceWorldTransform, InWorld, Component->GroundSlopeAngle, TableauActor.Get(), Component->bAlignToNormal))
		{
			InstanceWorldTransforms.Add(InstanceWorldTransform);
		}
	}

	// Update the transforms.
	Component->ClearInstances();
	Component->PreAllocateInstancesMemory(InstanceWorldTransforms.Num());
	for (const FTransform& Xform : InstanceWorldTransforms)
	{
		Component->AddInstanceWorldSpace(Xform);
	}

}

void FTableauFoliageManager::ExtractFoliage(UWorld* InWorld)
{
	if (TableauActor.IsValid())
	{
		AInstancedFoliageActor* FoliageActor = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(TableauActor->GetLevel(), true);

		for (const UTableauHISMComponent* Foliage : TableauActor->FoliageComponents)
		{
			// Resolve the HISM's FoliageType
			FSoftObjectPtr SoftObjectPtr(Foliage->FoliageType);
			if (UFoliageType* FoliageType = Cast<UFoliageType>(SoftObjectPtr.LoadSynchronous()))
			{

				if (!FoliageActor->FoliageInfos.Contains(FoliageType))
				{
					FoliageActor->AddFoliageType(FoliageType, nullptr);
				}

				// Copy the instances from the HISM to the Foliage
				FFoliageInfo* FoliageInfo = FoliageActor->FindInfo(FoliageType);

				// Gather locations from Tableau HISM Component and add to Foliage Component
				int32 InstanceCount = Foliage->GetInstanceCount();
				for (int32 i = 0; i < InstanceCount; ++i)
				{
					FTransform TransferLocation;
					Foliage->GetInstanceTransform(i, TransferLocation, true);

					FFoliageInstance InstancePlacement;
					InstancePlacement.Location = TransferLocation.GetLocation();
					InstancePlacement.DrawScale3D = TransferLocation.GetScale3D();
					InstancePlacement.Rotation = TransferLocation.Rotator();

					FoliageInfo->AddInstance(FoliageActor, FoliageType, InstancePlacement);
				}
			}
		}
	}

	DestroyFoliageComponents();
}