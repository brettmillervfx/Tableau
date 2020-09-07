
#include "TableauActorManager.h"

// Engine Includes
#include "Editor.h"
#include "Engine/Selection.h"
#include "LevelEditor.h"
#include "ISceneOutliner.h"
#include "ActorTreeItem.h"
#include "SceneOutlinerVisitorTypes.h"
#include "Layers/LayersSubsystem.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/STreeView.h"
#include "AssetSelection.h"
#include "ScopedTransaction.h"
#include "Components/SplineComponent.h"

// Local Includes
#include "TableauEditorModule.h"
#include "TableauActorFactory.h"
#include "TableauUtils.h"


// constants
static const FFloatInterval DefaultViableGroundSlopeAngleInterval(0.0, 90.0);

FTableauActorManager::FTableauActorManager(ATableauActor* InTableauActor, UWorld* InTargetWorld)
	: TableauActor(InTableauActor)
{
	TargetWorld = InTargetWorld;
	if (TargetWorld == nullptr)
	{
		TargetWorld = TableauActor->GetLevel()->OwningWorld;
		bAssetEditorWorkflow = false;
	}
	else
	{
		bAssetEditorWorkflow = true;
	}
}

FTableauActorManager::~FTableauActorManager()
{
}

void FTableauActorManager::UpdateInstances(bool bIsPreview)
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Regenerate Tableau")));

	UTableauComponent* TableauComponent = TableauActor->GetTableauComponent();
	TableauComponent->Modify(true);

	TSharedPtr<FTableauFilterSampler> Filter = GatherFilters(TableauComponent);

	const UTableauAsset* TableauAsset = TableauComponent->GetTableau();

	// If the Tableau Asset is not set, there is no point proceeding.
	if (TableauAsset == nullptr)
	{
		return;
	}

	if (!bIsPreview)
	{
		TableauComponent->RestoreInstances();

		// Remove child spawns
		DeleteInstances();
	}

	FTableauLatentTree TableauLatentTree(TableauAsset, Filter, !bAssetEditorWorkflow);
	TableauLatentTree.EvaluateLatentTree(TableauComponent->Seed);

	SpawnInstances(TableauLatentTree.GetRecipe(), bIsPreview);
	SpawnFoliage(TableauLatentTree.GetFoliages());

	if (!bIsPreview)
	{
		if (TableauComponent->bAutoSnap)
		{
			SnapToFloor();
		}
		
		MoveSelectionToTableauParent();

		TableauComponent->UpdateBounds();

		// Need to allow a moment for the Outliner to refresh...
		FTimerDelegate Delegate = FTimerDelegate::CreateLambda([]()
			{
				for (FSelectionIterator Iter(*GEditor->GetSelectedActors()); Iter; ++Iter)
				{
					if (ATableauActor* Actor = Cast<ATableauActor>(*Iter))
					{
						FTableauActorManager Manager(Actor);
						Manager.CollapseActorInOutliner();
					}
				}
			});

		static FTimerHandle DelayTimer;
		if (DelayTimer.IsValid())
		{
			GEditor->GetTimerManager()->ClearTimer(DelayTimer);
		}
		GEditor->GetTimerManager()->SetTimer(DelayTimer, Delegate, 0.1f, false);

	}

}

void FTableauActorManager::SpawnInstances(TArray<FTableauRecipeNode>& Recipe, bool bIsPreview)
{

	// Stash current level
	ULevel* OldCurrentLevel = TargetWorld->GetCurrentLevel();

	// (Temporarily) set to Actor's level to avoid spawning actors into some other streaming level.
	TargetWorld->SetCurrentLevel(TableauActor->GetLevel());

	// We will be transforming the spawned actors into the Tableau's local space.
	const FTransform TableauSpace = TableauActor->GetActorTransform();

	TArray<TWeakObjectPtr<AActor>> SpawnedActors = FTableauUtils::SpawnInstances(TableauActor, TableauSpace, Recipe, TableauActor->GetTableauComponent(), nullptr, bIsPreview);

	// Parent spawned actors to Tableau
	for (auto It = SpawnedActors.CreateConstIterator(); It; ++It)
	{
		AActor* SpawnedActor = It->Get();
		GEditor->ParentActors(TableauActor, SpawnedActor, TEXT("Name_NONE"), TableauActor->GetTableauComponent());
	}

	// We're done so revert the level
	TargetWorld->SetCurrentLevel(OldCurrentLevel);

	GEngine->ForceGarbageCollection(true);

}

AActor* FTableauActorManager::SpawnElement(ULevel* Level, const FTableauAssetElement& Element, int32 Seed, const FTransform& Space)
{
	AActor* ReturnElement = nullptr;

	// Place an actor described by the Tableau Element.
	const FTransform SpawnXform = Element.LocalTransform * Space;

	// Is the requested asset a Tableau?
	FSoftObjectPtr SoftObjectPtr(Element.AssetReference);
	UObject* TargetAsset = SoftObjectPtr.LoadSynchronous();

	if (UTableauAsset* TargetTableauAsset = Cast<UTableauAsset>(TargetAsset))
	{
		ReturnElement = FTableauUtils::SpawnTableauActor(Level->OwningWorld, TargetTableauAsset, SpawnXform, Element.Name, Seed);
	}
	else if (Element.bUseConfig & !bAssetEditorWorkflow)
	{
		FString Config = Element.AssetConfig;
		ReturnElement = FTableauUtils::PasteActor(TargetWorld, Config, SpawnXform, Element.Name);
	}
	else
	{
		ReturnElement = FTableauUtils::SpawnActor(TargetWorld, TargetAsset, SpawnXform, Element.Name);
	}

	return ReturnElement;

}

void FTableauActorManager::SpawnFoliage(TMap<UFoliageType*, TUniquePtr<FTableauFoliage>>& Foliages)
{
	// Call upon each Foliage type to configure, fill, and attach a HISM component for that type.
	for (const auto& Foliage : Foliages)
	{
		if (Foliage.Value.IsValid())
		{
			Foliage.Value->ConfigureAndAttachHISM(TableauActor);
		}
	}
}

void FTableauActorManager::DeleteInstances()
{
	// Get all of the actor's instances and delete them.
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	TArray<FTableauInstanceTracker> Instances = Component->GetInstances();

	for (FTableauInstanceTracker& Instance : Instances)
	{
		Instance.Apply([](AActor* InActor)
			{
				GEditor->GetEditorSubsystem<ULayersSubsystem>()->DisassociateActorFromLayers(InActor);
				InActor->GetLevel()->OwningWorld->DestroyActor(InActor, true);
			});

	}

	Component->ClearInstances();

	// Delete any HISMComponents attached to the TableauActor
	FTableauFoliageManager FoliageManager(TableauActor);
	FoliageManager.DestroyFoliageComponents();

	// Delete any StaticMeshComponents attached to the TableauActor
	for (UStaticMeshComponent* StaticMesh : TableauActor->StaticMeshComponents)
	{
		TableauActor->RemoveInstanceComponent(StaticMesh);
		StaticMesh->DestroyComponent(false);
	}
	TableauActor->StaticMeshComponents.Empty();

	// Delete any ChildActorComponents attached to the TableauActor
	for (UChildActorComponent* ChildActor : TableauActor->ChildActorComponents)
	{
		TableauActor->RemoveInstanceComponent(ChildActor);
		ChildActor->DestroyComponent(false);
	}
	TableauActor->ChildActorComponents.Empty();

}

void FTableauActorManager::Reseed()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Reseed Tableau")));

	// Set the actor's seed to a random value.
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	Component->Modify(true);
	Component->SetSeed(FMath::RandHelper(TNumericLimits<int32>::Max()));

	UpdateInstances(false);
}

void FTableauActorManager::CollapseActorInOutliner()
{
	struct FToggleExpansionVisitor : SceneOutliner::IMutableTreeItemVisitor
	{
		virtual void Visit(SceneOutliner::FActorTreeItem& ActorItem) const override
		{
			AActor* Actor = ActorItem.Actor.Get();
			if (Actor)
			{
				ActorItem.Flags.bIsExpanded = false;
			}
		}
	};

	auto& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	auto LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
	const FTabId SceneOutlinerTabId("LevelEditorSceneOutliner");
	auto SceneOutlinerTab = LevelEditorTabManager->FindExistingLiveTab(SceneOutlinerTabId);

	if (SceneOutlinerTab.IsValid())
	{
		auto BorderWidget = StaticCastSharedRef<SBorder>(SceneOutlinerTab->GetContent());
		auto SceneOutlinerWidget = StaticCastSharedRef<ISceneOutliner>(BorderWidget->GetContent());

		const auto& TreeView = SceneOutlinerWidget->GetTree();
		TSet<SceneOutliner::FTreeItemPtr> VisitingItems;
		TreeView.GetExpandedItems(VisitingItems);

		for (SceneOutliner::FTreeItemPtr& Item : VisitingItems)
		{
			TSharedPtr<SceneOutliner::FActorTreeItem> ActorTreeItem = StaticCastSharedPtr<SceneOutliner::FActorTreeItem>(Item);
			if (ActorTreeItem.IsValid() && ActorTreeItem->Actor.IsValid() && !ActorTreeItem->Actor->IsPendingKillPending() && ActorTreeItem->Actor == TableauActor)
			{
				FToggleExpansionVisitor ExpansionVisitor;
				ActorTreeItem->Visit(ExpansionVisitor);
				break;
			}

		}

		SceneOutlinerWidget->Refresh();
		GEditor->RedrawLevelEditingViewports();
	}

}


void FTableauActorManager::MoveSelectionToTableauParent()
{
	// Deselect children
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	TArray<FTableauInstanceTracker> Instances = Component->GetInstances();
	for (FTableauInstanceTracker& Instance : Instances)
	{

		Instance.Apply([](AActor* InActor)
			{
				GEditor->SelectActor(InActor, false, false, true);
			});

	}

	// Select parent
	GEditor->SelectActor(TableauActor, true, false, true);
}



void FTableauActorManager::ClearInstances()
{
	// Sever relationship between Tableau and its elements
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	Component->ClearInstances();
}

void FTableauActorManager::Unpack()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Unpack Tableau")));

	// Stash the world and current level
	ULevel* OldCurrentLevel = TargetWorld->GetCurrentLevel();

	// (Temporarily) set to Actor's level to avoid spawning actors into some other streaming level.
	TargetWorld->SetCurrentLevel(TableauActor->GetLevel());

	UTableauComponent* Component = TableauActor->GetTableauComponent();
	Component->Modify(true);
	TArray<FTableauInstanceTracker> Instances = Component->GetInstances();

	// Extract instance actors from Tableau and select them in order.
	GEditor->SelectNone(true, true, false);
	for (FTableauInstanceTracker& Instance : Instances)
	{
		Instance.Apply([](AActor* InActor)
			{
				InActor->Modify(true);
				FDetachmentTransformRules DetachmentRule(EDetachmentRule::KeepWorld, true);
				InActor->DetachFromActor(DetachmentRule);

				// Remove tag indicating Tableau element status.
				InActor->Tags.Remove(TableauActorConstants::TABLEAU_ELEMENT_TAG);

				GEditor->SelectActor(InActor, true, true, true, false);
			});

	}

	// Unpack HISMs by converting to Foliage Actor
	FTableauFoliageManager FoliageManager(TableauActor);
	FoliageManager.ExtractFoliage(TargetWorld);

	// Unpack StaticMesh Components
	for (UStaticMeshComponent* StaticMesh : TableauActor->StaticMeshComponents)
	{
		FTransform WorldTransform(StaticMesh->GetComponentTransform());
		if (UActorFactory* ActorFactory = FActorFactoryAssetProxy::GetFactoryForAssetObject(StaticMesh->GetStaticMesh()))
		{
			ActorFactory->CreateActor(StaticMesh->GetStaticMesh(), TableauActor->GetLevel(), WorldTransform, RF_Transactional, NAME_None);
		}

	}

	// Unpack ChildActor Components
	for (UChildActorComponent* ChildActor : TableauActor->ChildActorComponents)
	{
		FTransform WorldTransform(ChildActor->GetComponentTransform());
		TSubclassOf<AActor> ActorClass = ChildActor->GetChildActorClass();

		FActorSpawnParameters SpawnParameters;
		TargetWorld->SpawnActor(ActorClass, &WorldTransform, SpawnParameters);

	}

	// And then delete the Tableau actor
	ClearInstances();
	TargetWorld->DestroyActor(TableauActor, true);

	// We're done so revert the level
	TargetWorld->SetCurrentLevel(OldCurrentLevel);

}

void FTableauActorManager::UnpackTopLayer()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Unpack Tableau Layer")));

	// Stash the world and current level
	ULevel* ActorLevel = TableauActor->GetLevel();
	if (ActorLevel == nullptr)
	{
		UE_LOG(LogTableau, Warning, TEXT("%s must be in a valid level. The actor will not be unpacked."), *TableauActor->GetActorLabel());
		return;
	}
	ULevel* OldCurrentLevel = TargetWorld->GetCurrentLevel();

	// (Temporarily) set to Actor's level to avoid spawning actors into some other streaming level.
	TargetWorld->SetCurrentLevel(ActorLevel);

	//Collect parameters for the layer of the Tableau evaluation.
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	Component->Modify(true);
	UTableauAsset* TableauAsset = Component->Tableau;
	int32 Seed = Component->Seed;

	TSharedPtr<FTableauFilterSampler> Filter = GatherFilters(Component);

	// A new selection will be made of the unpacked Tableaux
	TArray<AActor*> NewSelection;

	// Asset needs to have at least 1 element
	if (TableauAsset->TableauElement.Num() > 0)
	{

		// Eliminate the existing heirarchy
		DeleteInstances();

		// We'll be dropping the new actors into the space currently defined by this Tableau.
		const FTransform TableauSpace = TableauActor->GetActorTransform();

		if (TableauAsset->EvaluationMode == ETableauEvaluationMode::Superposition)
		{
			// Make the same selection for superposition that the original actor did.
			FTableauLatentTree TableauLatentTree(TableauAsset, Filter, !bAssetEditorWorkflow);
			const FTableauAssetElement* Element = TableauLatentTree.SelectRandomElement(TableauAsset, Seed);
			if (Element)
			{
				if (Filter->Sample(Element->LocalTransform.GetLocation()))
				{
					NewSelection.Add(SpawnElement(ActorLevel, *Element, FTableauUtils::NextSeed(Seed), TableauSpace));
				}
			}
		}
		else // Composition
		{
			for (const FTableauAssetElement& Element : TableauAsset->TableauElement)
			{
				Seed = FTableauUtils::NextSeed(Seed);
				if (Filter->Sample(Element.LocalTransform.GetLocation()))
				{
					if (Element.bDeterministic)
					{
						NewSelection.Add(SpawnElement(ActorLevel, Element, Element.Seed, TableauSpace));
					}
					else
					{
						NewSelection.Add(SpawnElement(ActorLevel, Element, Seed, TableauSpace));
					}
				}
			}
		}
	}

	// And destroy the original actor
	TargetWorld->DestroyActor(TableauActor, true);

	// Modify selection
	GEditor->SelectNone(true, true, false);
	for (AActor* ActorSelection : NewSelection)
	{
		GEditor->SelectActor(ActorSelection, true, true, true, false);
	}

	// We're done so revert the level
	TargetWorld->SetCurrentLevel(OldCurrentLevel);

}


void FTableauActorManager::UnpackForEditing()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Unpack For Editing")));

	// Stash the world and current level
	ULevel* ActorLevel = TableauActor->GetLevel();
	if (ActorLevel == nullptr)
	{
		UE_LOG(LogTableau, Warning, TEXT("%s must be in a valid level. The actor will not be unpacked."), *TableauActor->GetActorLabel());
		return;
	}
	ULevel* OldCurrentLevel = TargetWorld->GetCurrentLevel();

	// (Temporarily) set to Actor's level to avoid spawning actors into some other streaming level.
	TargetWorld->SetCurrentLevel(ActorLevel);

	//Collect parameters for the layer of the Tableau evaluation.
	UTableauComponent* Component = TableauActor->GetTableauComponent();
	Component->Modify(true);
	UTableauAsset* TableauAsset = Component->Tableau;
	int32 Seed = Component->Seed;

	// A new selection will be made of the unpacked Tableaux
	TArray<AActor*> NewSelection;

	// Asset needs to have at least 1 element
	if (TableauAsset->TableauElement.Num() > 0)
	{

		// Eliminate the existing heirarchy
		DeleteInstances();

		// We'll be dropping the new actors into the space currently defined by this Tableau.
		const FTransform TableauSpace = TableauActor->GetActorTransform();
		
		for (const FTableauAssetElement& Element : TableauAsset->TableauElement)
		{
			Seed = FTableauUtils::NextSeed(Seed);
			if (Element.bDeterministic)
			{
				NewSelection.Add(SpawnElement(ActorLevel, Element, Element.Seed, TableauSpace));
			}
			else
			{
				NewSelection.Add(SpawnElement(ActorLevel, Element, Seed, TableauSpace));
			}
		}	
	}

	// And destroy the original actor
	TargetWorld->DestroyActor(TableauActor, true);

	// Modify selection
	GEditor->SelectNone(true, true, false);
	for (AActor* ActorSelection : NewSelection)
	{
		GEditor->SelectActor(ActorSelection, true, true, true, false);
	}

	// We're done so revert the level
	TargetWorld->SetCurrentLevel(OldCurrentLevel);

}


void FTableauActorManager::SnapToFloor()
{
	const FScopedTransaction Transaction(FText::FromString(TEXT("Snap Tableau")));

	UTableauComponent* Component = TableauActor->GetTableauComponent();
	SnapToFloor(Component->GetInstances(), FVector(0.f, 0.f, 0.f));

	// We also snap any attached Foliage HISMs
	FTableauFoliageManager FoliageManager(TableauActor);
	FoliageManager.SnapToFloor(TargetWorld);

	// Snap all StaticMesh Components
	for (UStaticMeshComponent* StaticMesh : TableauActor->StaticMeshComponents)
	{
		// Some of the components are Tableau HISMs. Skip them.
		if (!StaticMesh->IsA<UTableauHISMComponent>())
		{
			FTransform WorldTransform(StaticMesh->GetComponentLocation());
			FTableauUtils::TraceToWorld(WorldTransform, TargetWorld, DefaultViableGroundSlopeAngleInterval, TableauActor, false);
			StaticMesh->SetWorldLocation(WorldTransform.GetLocation());
		}
	}

	// Snap all ChildActor Components
	for (UChildActorComponent* ChildActor : TableauActor->ChildActorComponents)
	{
		FTransform WorldTransform(ChildActor->GetComponentLocation());
		FTableauUtils::TraceToWorld(WorldTransform, TargetWorld, DefaultViableGroundSlopeAngleInterval, TableauActor, false);
		ChildActor->SetWorldLocation(WorldTransform.GetLocation());
	}
}

void FTableauActorManager::SnapToFloor(const TArray<FTableauInstanceTracker>& Instances, const FVector& ParentTranslate)
{

	// Get all of the actor instances and snap them to the floor.

	// If the actor has a snap tag, always snap it.
	// Otherwise, translate the actor with it's parent.

	for (const FTableauInstanceTracker& Instance : Instances)
	{
		if (Instance.TableauInstance.IsValid())
		{
			Instance.TableauInstance->Modify(true);

			const FVector InitialLocation = Instance.TableauInstance->GetActorLocation();

			if (Instance.TableauInstance->ActorHasTag(TableauActorConstants::TABLEAU_SNAPTOFLOOR_TAG))
			{
				GEditor->SnapObjectTo(Instance.TableauInstance.Get(), false, true, true, false);
			}
			else
			{
				Instance.TableauInstance->AddActorLocalOffset(ParentTranslate, false);
			}

			if (Instance.SubInstances.Num() > 0)
			{
				// Find how much the owning actor has moved and translate unsnapped 
				// children by that offset.
				const FVector Offset = Instance.TableauInstance->GetActorLocation() - InitialLocation;

				SnapToFloor(Instance.SubInstances, Offset);
			}

		}
	}

}

void FTableauActorManager::FilterBySelectedActors()
{
	for (FSelectionIterator Iter(*GEditor->GetSelectedActors()); Iter; ++Iter)
	{
		if (AActor* SelectedActor = Cast<AActor>(*Iter))
		{
			if (!SelectedActor->IsA<ATableauActor>())
			{
				bool bAddActorToFilter = true;

				// Determine if the specified actor is already used as a filter.
				for (const FFilterActor FilterActor : TableauActor->GetTableauComponent()->FilterActors)
				{
					if (FilterActor.Actor.Get() == SelectedActor)
					{
						UE_LOG(LogTableau, Warning, TEXT("Selected actor %s already in filter list."), *SelectedActor->GetActorLabel());
						bAddActorToFilter = false;
						break;
					}

				}

				if (bAddActorToFilter)
				{
					FFilterActor NewFilterActor;
					NewFilterActor.Actor = SelectedActor;
					TableauActor->GetTableauComponent()->FilterActors.Add(NewFilterActor);
					
					UE_LOG(LogTableau, Display, TEXT("Selected actor %s added to filter."), *SelectedActor->GetActorLabel());
				}
				
			}
		}
		
	}
}

void FTableauActorManager::ValidateTracking()
{
	UTableauComponent* TableauComponent = TableauActor->GetTableauComponent();
	TableauComponent->RestoreInstances();

	// Foliage Manager validates foliage on instatiation.
	FTableauFoliageManager FoliageManager(TableauActor);
}

TSharedPtr<FTableauFilterSampler> FTableauActorManager::GatherFilters(const UTableauComponent* TableauComponent) const
{
	TSharedPtr<FTableauFilterSampler> Filter(new FTableauFilterSampler(TableauActor->ActorToWorld()));

	// Actors in the component's FilterActors list create cylinder filter volumes
	// corresponding to the bounds of the actor.
	// All locations are in the TableauComponent local space
	for (const FFilterActor& FilterActor : TableauComponent->FilterActors)
	{
		if (FilterActor.Actor.IsValid())
		{
			// If the actor contains a spline component, it is handled differently.
			if (USplineComponent* SplineComponent = Cast<USplineComponent>(FilterActor.Actor->GetRootComponent()))
			{
				TWeakObjectPtr<USplineComponent> SplineComponentPtr(SplineComponent);
				Filter->AddSplineFilter(SplineComponentPtr, FilterActor.ExpandVolume);
			}
			else
			{
				// This is treated as a general actor.
				float Radius, Height;
				FilterActor.Actor->GetComponentsBoundingCylinder(Radius, Height, false);

				Filter->AddCylinderVolumeFilter(FilterActor.Actor->GetActorLocation(), Radius + FilterActor.ExpandVolume);
			}
		}
	}

	// Add a filter for each specified landscape layer
	for (const FFilterWeightmap& FilterWeightmap : TableauComponent->FilterWeightmaps)
	{
		Filter->AddLandscapeLayerFilter(FilterWeightmap.LayerName, FilterWeightmap.WeightThreshold);
	}

	// Add a filter for each specified tag.
	for (const FFilterTag& FilterTag : TableauComponent->FilterTags)
	{
		Filter->AddTagFilter(FilterTag.Tag, FilterTag.Tolerance);
	}

	if (!TableauComponent->bIgnoreExclusion)
	{
		// Add any TableauExclusionVolumes in the currently loaded World.
		Filter->AddAllLoadedExclusionVolumes(TargetWorld);
	}

	return Filter;
}



