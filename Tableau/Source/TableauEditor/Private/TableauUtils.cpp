#include "TableauUtils.h"

// Engine Includes
#include "Editor.h"
#include "Engine/Selection.h"
#include "BusyCursor.h"
#include "Factories/LevelFactory.h"
#include "Layers/ILayers.h"
#include "AssetSelection.h"
#include "ScopedTransaction.h"
#include "Components/InstancedStaticMeshComponent.h"

// Local Includes
#include "TableauEditorModule.h"
#include "TableauActorManager.h"
#include "TableauActorFactory.h"

ATableauActor* FTableauUtils::GetFirstTableauParentActor(AActor* InActor)
{
	ATableauActor* FirstTableauParentActor = nullptr;
	if (InActor && !InActor->IsPendingKillPending())
	{
		for (AActor* ParentActor = InActor->GetAttachParentActor(); ParentActor && !ParentActor->IsPendingKillPending(); ParentActor = ParentActor->GetAttachParentActor())
		{
			if (ATableauActor * ParentTableauActor = Cast<ATableauActor>(ParentActor))
			{
				FirstTableauParentActor = ParentTableauActor;
				break;
			}
		}
	}

	return FirstTableauParentActor;
}

TArray<TWeakObjectPtr<AActor>> FTableauUtils::SpawnInstances(ATableauActor* OwningActor, const FTransform& TableauSpace, const TArray<FTableauRecipeNode>& Recipe, UTableauComponent* TableauComponent, FTableauInstanceTracker* CurrTracker, bool bIsPreview)
{

	TArray<TWeakObjectPtr<AActor>> SpawnedPreviewActors;
	
	for (const FTableauRecipeNode& RecipeNode : Recipe)
	{
	
		AActor* SpawnedActor = nullptr;

		if (RecipeNode.bUseConfig)
		{
			SpawnedActor = PasteActor(OwningActor->GetWorld(), RecipeNode.AssetConfig, RecipeNode.LocalTransform * TableauSpace, RecipeNode.Name);
		}
		else
		{
			if (RecipeNode.AssetReference.IsValid())
			{
				SpawnComponent(OwningActor, RecipeNode.AssetReference.Get(), RecipeNode.LocalTransform, RecipeNode.Name);
			}
		}
		
		if (SpawnedActor)
		{

			// Add a tag informing the editor that this actor is a Tableau element.
			SpawnedActor->Tags.Add(TableauActorConstants::TABLEAU_ELEMENT_TAG);
			
			// Add a tag to signify the actors inclusion in snap operations.
			if (RecipeNode.bSnapToFloor)
			{
				SpawnedActor->Tags.Add(TableauActorConstants::TABLEAU_SNAPTOFLOOR_TAG);
			}

			if (bIsPreview)
			{
				SpawnedActor->Tags.Add(TableauActorConstants::TABLEAU_PREVIEW_TAG);
				SpawnedActor->SetActorEnableCollision(false);
			}

			TWeakObjectPtr<AActor> SpawnedActorWeakPtr(SpawnedActor);
			SpawnedPreviewActors.Add(SpawnedActorWeakPtr);
			TableauComponent->RegisterInstance(SpawnedActor, CurrTracker);

			// Spawn subordinate actors
			if (RecipeNode.SubRecipe.Num() > 0)
			{
				SpawnedPreviewActors.Append(SpawnInstances(OwningActor, TableauSpace, RecipeNode.SubRecipe, TableauComponent, TableauComponent->GetLastInstanceTracker(), bIsPreview));
			}

		}

	}

	return SpawnedPreviewActors;

}

AActor* FTableauUtils::PasteActor(UWorld* InWorld, const FString& Config, const FTransform& Xform, const FName& Name)
{

	TArray<AActor*> PastedActors;
	FClipboard::edactPasteSelected(InWorld, &Config, PastedActors);

	if (PastedActors.Num() > 0 && PastedActors.Last() != NULL)
	{
		AActor* PastedActor = PastedActors.Last();

		USceneComponent* Root = PastedActor->GetRootComponent();
		if (Root)
		{
			Root->SetRelativeTransform(Xform);
		}

		// Rename the actor
		PastedActor->SetActorLabel(Name.ToString());

		return PastedActor;
	}
	else
	{
		return nullptr;
	}

}

USceneComponent* FTableauUtils::BuildComponent(UObject* TargetAsset)
{
	// If the asset to be instatiated is a Static Mesh, we provide a Static Mesh Component.
	if (UStaticMesh* StaticMeshAsset = Cast<UStaticMesh>(TargetAsset))
	{
		UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(GetTransientPackage(), NAME_None, RF_Transient);

		// Specify the Static Mesh configuration
		StaticMeshComponent->SetStaticMesh(StaticMeshAsset);
		StaticMeshComponent->SetMobility(EComponentMobility::Static);

		return StaticMeshComponent;
	}
	else
	{
		// We spawn a child actor component otherwise.
		UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(GetTransientPackage(), NAME_None, RF_Transient);

		// Determine the actor class implied by the target asset
		if (UActorFactory* ActorFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(TargetAsset))
		{
			FAssetData AssetData(TargetAsset, true);
			AActor* DefaultActor = ActorFactory->GetDefaultActor(AssetData);
			if (DefaultActor)
			{
				ChildActorComponent->SetChildActorClass(DefaultActor->GetClass());
			}
		}

		return ChildActorComponent;
	}

}

void FTableauUtils::SpawnComponent(ATableauActor* OwningActor, UObject* TargetAsset, const FTransform& Xform, const FName& Name)
{
	// If the asset to be instatiated is a Static Mesh, we provide a Static Mesh Component.
	if (UStaticMesh* StaticMeshAsset = Cast<UStaticMesh>(TargetAsset))
	{
		UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(OwningActor, UStaticMeshComponent::StaticClass(), NAME_None, RF_Transactional);

		StaticMeshComponent->SetupAttachment(OwningActor->GetRootComponent());

		if (OwningActor->GetRootComponent()->IsRegistered())
		{
			StaticMeshComponent->RegisterComponent();
			OwningActor->StaticMeshComponents.Add(StaticMeshComponent);
			OwningActor->AddInstanceComponent(StaticMeshComponent);
		}

		// Set transform local to owning actor
		StaticMeshComponent->SetRelativeTransform(Xform);

		// Specify the Static Mesh configuration
		StaticMeshComponent->SetStaticMesh(StaticMeshAsset);
		StaticMeshComponent->SetMobility(EComponentMobility::Static);

		// Add the new component to the transaction buffer so it will get destroyed on undo
		StaticMeshComponent->Modify();
		// We don't want to track changes to to the static mesh later so we mark it as non-transactional
		StaticMeshComponent->ClearFlags(RF_Transactional);
	}
	else
	{
		// We spawn a child actor component otherwise.
		UChildActorComponent* ChildActorComponent = NewObject<UChildActorComponent>(OwningActor, UChildActorComponent::StaticClass(), NAME_None, RF_Transactional);

		ChildActorComponent->SetupAttachment(OwningActor->GetRootComponent());

		if (OwningActor->GetRootComponent()->IsRegistered())
		{
			ChildActorComponent->RegisterComponent();
			OwningActor->ChildActorComponents.Add(ChildActorComponent);
			OwningActor->AddInstanceComponent(ChildActorComponent);
		}

		// Set transform local to owning actor
		ChildActorComponent->SetRelativeTransform(Xform);
		
		// Determine the actor class implied by the target asset
		if (UActorFactory* ActorFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(TargetAsset))
		{
			FAssetData AssetData(TargetAsset, true);
			AActor* DefaultActor = ActorFactory->GetDefaultActor(AssetData);
			if (DefaultActor)
			{
				ChildActorComponent->SetChildActorClass(DefaultActor->GetClass());
			}
		}
		
		// Add the new component to the transaction buffer so it will get destroyed on undo
		ChildActorComponent->Modify();
		// We don't want to track changes to to the child actor later so we mark it as non-transactional
		ChildActorComponent->ClearFlags(RF_Transactional);
	}

}


AActor* FTableauUtils::SpawnActor(UWorld * World, UObject * TargetAsset, const FTransform & Xform, const FName & Name)
{
	// If we're trying to spawn a foliage type, spawn the static mesh instead
	if (UFoliageType* FoliageType = Cast<UFoliageType>(TargetAsset))
	{
		const UFoliageType_InstancedStaticMesh* FoliageType_InstancedStaticMesh = Cast<UFoliageType_InstancedStaticMesh>(FoliageType);
		TargetAsset = FoliageType_InstancedStaticMesh->GetStaticMesh();
	}
	
	if (UActorFactory* ActorFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(TargetAsset))
	{
		if (AActor* SpawnedActor = ActorFactory->CreateActor(TargetAsset, World->GetCurrentLevel(), Xform, RF_Transactional, Name))
		{
			// Rename the actor.
			SpawnedActor->SetActorLabel(Name.ToString());
			
			return SpawnedActor;
		}
	}

	return nullptr;
}


AActor* FTableauUtils::SpawnTableauActor(UWorld* World, UTableauAsset* TargetTableauAsset, const FTransform& Xform, const FName& Name, const int32 Seed)
{
	
	FActorSpawnParameters SpawnParameters;
	AActor* SpawnedActor = World->SpawnActor(ATableauActor::StaticClass(), &Xform, SpawnParameters);
	if (ATableauActor* SpawnedTableauActor = Cast<ATableauActor>(SpawnedActor))
	{
		// Rename the actor.
		SpawnedTableauActor->SetActorLabel(Name.ToString());

		// Set the Asset and Seed
		UTableauComponent* TableauComponent = SpawnedTableauActor->GetTableauComponent();
		TableauComponent->SetTableau(TargetTableauAsset);
		TableauComponent->SetSeed(Seed);

		// And generate the Tableau
		FTableauActorManager Manager(SpawnedTableauActor);
		Manager.UpdateInstances(false);


		return SpawnedTableauActor;
	}

	return nullptr;
}

AActor* FTableauUtils::ReplaceActorsWithTableau(UTableauAsset* TableauAsset, const TArray<AActor*>& SelectedActors)
{
	// Following the capture of a set of selected actors to a Tableau asset, swap the newly
	// created Tableau in for the selection set.
	
	const FScopedTransaction Transaction(FText::FromString(TEXT("Replace Actors With Tableau")));

	// Swap in the newly created Tableau. We only use the original actor's location as the Tableau presumably
	// has scale / rotation built in.
	const FTransform TableauXform(SelectedActors[0]->GetActorLocation());
	
	UWorld* TargetWorld = SelectedActors[0]->GetLevel()->OwningWorld;
	const FName TableauName = TableauAsset->GetFName();
	AActor* SpawnedTableau = FTableauUtils::SpawnTableauActor(TargetWorld, TableauAsset, TableauXform, TableauName);

	// Delete selected actors and replace with new Tableau asset.
	for (AActor* SelectedActor : SelectedActors)
	{
		SelectedActor->Destroy();
	}

	// And select the new Tableau
	GEditor->SelectNone(true, true, false);
	GEditor->SelectActor(SpawnedTableau, true, true, true, false);

	return SpawnedTableau;
}

void FTableauUtils::FillTableauAssetElements(UTableauAsset* InAsset, const TArray<AActor*>& SelectedActors)
{
	check(InAsset);
	
	InAsset->ClearElements();

	if (SelectedActors.Num() > 0)
	{

		// Tableau pivots around first selected actor'slocation.
		FTransform LocalToWorld(SelectedActors[0]->GetActorLocation());

		const FTransform WorldToLocal = LocalToWorld.Inverse();

		UWorld* World = SelectedActors[0]->GetWorld();

		for (int32 index = 0; index < SelectedActors.Num(); ++index)
		{

			AActor* Sel = SelectedActors[index];

			TArray<UObject*> RefObjects;
			if (Sel->GetReferencedContentObjects(RefObjects))
			{
				if (RefObjects.Num() > 0)
				{
					// This is possibly a Tableau actor
					if (ATableauActor* TableauActor = Cast<ATableauActor>(Sel))
					{
						const UTableauComponent* TableauComp = TableauActor->GetTableauComponent();
						const FSoftObjectPath AssetReference(TableauComp->GetTableau());
						const FString AssetConfig; // We don't require config for a Tableau Asset
						const FName Name = FName(*Sel->GetActorLabel());
						const FTransform LocalTransform = Sel->GetActorTransform() * WorldToLocal;
						const uint32 Seed = TableauComp->Seed;

						InAsset->AddElement(Name, AssetReference, AssetConfig, LocalTransform, Seed);
					}
					else
					{
						const FSoftObjectPath AssetReference = RefObjects[0];
						const FName Name = FName(*Sel->GetActorLabel());
						const FTransform LocalTransform = Sel->GetActorTransform();

						Sel->SetActorTransform(FTransform::Identity);
						FString AssetConfig;
						StoreActorAsString(World, Sel, &AssetConfig);
						Sel->SetActorTransform(LocalTransform);

						InAsset->AddElement(Name, AssetReference, AssetConfig, LocalTransform * WorldToLocal);
					}	
				}
				else
				{
					// No referenced content, which means it's a primitive, like a light.
					const FSoftObjectPath AssetReference(Sel->GetClass());
					const FName Name = FName(*Sel->GetActorLabel());
					const FTransform LocalTransform = Sel->GetActorTransform();

					Sel->SetActorTransform(FTransform::Identity);
					FString AssetConfig;
					StoreActorAsString(World, Sel, &AssetConfig);
					Sel->SetActorTransform(LocalTransform);

					InAsset->AddElement(Name, AssetReference, AssetConfig, LocalTransform * WorldToLocal);
				}
			}
		}
	}

}

void FTableauUtils::StoreActorAsString(UWorld* InWorld, AActor* InActor, FString* DestinationData)
{
	// Exploit the clipboard to get a string version of the actor.
	GEditor->SelectNone(false, false, false);
	GEditor->SelectActor(InActor, true, true, true, true);
	if (GEditor->CanCopySelectedActorsToClipboard(InWorld, nullptr))
	{
		GEditor->edactCopySelected(InWorld, DestinationData);
	}
}

void FTableauUtils::JitterTransform(uint32 Seed, float MinScale, float MaxScale, bool bSpinZAxis, FTransform& Transform)
{
	
	// Set the transform to origin for application of jitter.
	const FVector Location = Transform.GetLocation();
	Transform.SetLocation(FVector::ZeroVector);

	// Apply scale jitter
	if (FMath::IsNearlyZero(abs(MinScale-MaxScale)))
	{
		Transform.MultiplyScale3D(FVector(MinScale,MinScale,MinScale));
	}
	else
	{
		static const int32 SeedOffsetForScale = FTableauUtils::NextSeed(Seed);
		FRandomStream Stream(Seed+ SeedOffsetForScale);

		const float Scale = Stream.FRand()*(MaxScale-MinScale) + MinScale;
		Transform.MultiplyScale3D(FVector(Scale, Scale, Scale));
	}

	// Rotation may be assigned a random spin about Z axis
	if (bSpinZAxis)
	{
		static const int32 SeedOffsetForRotation = FTableauUtils::NextSeed(FTableauUtils::NextSeed(Seed));
		FRandomStream Stream(Seed + SeedOffsetForRotation);

		const FQuat Rotate(FVector::UpVector, Stream.FRand() * 2.0 * PI);
		Transform *= Rotate;
	}

	// Bring transform back to position.
	Transform.AddToTranslation(Location);
}

bool FTableauUtils::TraceToWorld(FTransform& Xform, const UWorld* InWorld, const FFloatInterval& GroundSlopeAngle, AActor* IgnoredActor=nullptr, bool bAlignToNormal = false)
{
	// We bracket our floor traces by +- 10m.
	const float SnapToFloorBound = 1000.0;

	// Establish vertical ray along which the trace will be conducted.
	FVector InitialLocation = Xform.GetTranslation();
	FVector TraceStart = InitialLocation + FVector(0.0, 0.0, SnapToFloorBound);
	FVector TraceEnd = InitialLocation - FVector(0.0, 0.0, SnapToFloorBound);

	FHitResult HitResult;

	FCollisionQueryParams CollisionQueryParams;
	if (IgnoredActor)
	{
		CollisionQueryParams.AddIgnoredActor(IgnoredActor);
	}
	

	FCollisionResponseParams CollisionResponseParams;
	
	if (!InWorld->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldStatic, CollisionQueryParams, CollisionResponseParams))
	{
		return false;
	}

	// We don't accept snapping to other HISMs. This tends to create floating instance issues.
	if (UHierarchicalInstancedStaticMeshComponent* HISMC = Cast<UHierarchicalInstancedStaticMeshComponent>(HitResult.Component.Get()))
	{
		return false;
	}

	// Determine if the hit result normal is within the allowed slope tolerance.
	float AngleFromHorizontal = FMath::RadiansToDegrees(FMath::Acos(HitResult.Normal.Z));
	if (!GroundSlopeAngle.Contains(AngleFromHorizontal))
	{
		return false;
	}
		
	Xform.SetTranslation(HitResult.Location);

	if (bAlignToNormal)
	{
		FQuat CurrentRot = Xform.GetRotation();
		FMatrix AlignedRotation = FRotationMatrix::MakeFromXZ(CurrentRot.GetAxisX(), HitResult.Normal);
		Xform.SetRotation(AlignedRotation.ToQuat());
	}

	return true;
}

int32 FTableauUtils::NextSeed(int32 Seed)
{
	FRandomStream Stream(Seed);

	// Advance the seed
	Stream.GetUnsignedInt();

	return Stream.GetCurrentSeed();
}




//////////////////////////////////////////////////
// FClipboard

void FClipboard::edactPasteSelected(UWorld* InWorld, const FString* SourceData, TArray<AActor*>& PastedActors)
{

	// Create a location offset.
	const FVector LocationOffset(0, 0, 0);

	FCachedActorLabels ActorLabels(InWorld);

	FString PasteString = *SourceData;
	const TCHAR* Paste = *PasteString;

	// Import the actors.
	ULevelFactory* Factory = NewObject<ULevelFactory>();
	Factory->FactoryCreateText(
		ULevel::StaticClass(),
		InWorld->GetCurrentLevel(),
		InWorld->GetCurrentLevel()->GetFName(),
		RF_Transactional,
		nullptr,
		TEXT("paste"),
		Paste,
		Paste + FCString::Strlen(Paste),
		GWarn);

	
	// Fire ULevel::LevelDirtiedEvent when falling out of scope.
	FScopedLevelDirtied	LevelDirtyCallback;

	// Update the actors' locations and update the global list of visible layers.
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		AActor* Actor = static_cast<AActor*>(*It);
		checkSlow(Actor->IsA(AActor::StaticClass()));

		// We only want to offset the location if this actor is the root of a selected attachment hierarchy
		// Offsetting children of an attachment hierarchy would cause them to drift away from the node they're attached to
		// as the offset would effectively get applied twice
		const AActor* const ParentActor = Actor->GetAttachParentActor();
		const FVector& ActorLocationOffset = (ParentActor && ParentActor->IsSelected()) ? FVector::ZeroVector : LocationOffset;

		// Offset the actor's location.
		Actor->TeleportTo(Actor->GetActorLocation() + ActorLocationOffset, Actor->GetActorRotation(), false, true);

		// Re-label duplicated actors so that labels become unique
		FActorLabelUtilities::SetActorLabelUnique(Actor, Actor->GetActorLabel(), &ActorLabels);
		ActorLabels.Add(Actor->GetActorLabel());

		// Call PostEditMove to update components, etc.
		Actor->PostEditMove(true);
		Actor->PostDuplicate(EDuplicateMode::Normal);
		Actor->CheckDefaultSubobjects();

		// Request saves/refreshes.
		Actor->MarkPackageDirty();
		LevelDirtyCallback.Request();

		// Return a pointer to this actor.
		PastedActors.Add(Actor);
	}
	
	GEditor->RedrawLevelEditingViewports();

}
