
#include "TableauComponent.h"

// Local includes
#include "TableauAssetModule.h"
#include "TableauActor.h"


//////////////////////////////////////////////
//UTableauComponent

UTableauComponent::FOnTableauDuplicateDelegate  UTableauComponent::OnTableauDuplicate;

UTableauComponent::UTableauComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FBoxSphereBounds  UTableauComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return GetBoundingBox();
}

UTableauAsset* UTableauComponent::GetTableau() const
{
	return Tableau;
}

bool UTableauComponent::SetTableau(UTableauAsset* InTableau)
{
	if (InTableau == GetTableau())
	{
		return false;
	}

	Tableau = InTableau;
	return true;
}

void UTableauComponent::SetSeed(int32 InSeed)
{
	Seed = InSeed;
}

FLinearColor UTableauComponent::GetColor() const
{
	FSoftObjectPath AssetReference = FSoftObjectPath(GetTableau());
	UObject* Object = SafelyResolveSoftPath(AssetReference);
	if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(Object))
	{
		switch (TableauAsset->EvaluationMode)
		{
			case ETableauEvaluationMode::Composition:
				return TableauColors::CompositionColor.ReinterpretAsLinear();

			case ETableauEvaluationMode::Superposition:
				return TableauColors::SuperpositionColor.ReinterpretAsLinear();

			case ETableauEvaluationMode::HierarchicalComposition:
				return TableauColors::HierarchicalCompositionColor.ReinterpretAsLinear();

			default:
				return TableauColors::ErrorColor.ReinterpretAsLinear();
		}
	}
	else
	{
		// For some reason this isn't pointing to a Tableau.
		return TableauColors::ErrorColor.ReinterpretAsLinear();
	}
}

FBox UTableauComponent::GetBoundingBox() const
{
	// Iterate the actors owned by the Tableau and append to the box.
	FBox Box(ForceInit);
	Box += GetBoundingBox(GetInstances());

	// And append with the bounds of attached HISMs, StaticMeshComponents, ChildActorComponents
	if (ATableauActor* TableauActor = Cast<ATableauActor>(GetOwner()))
	{
		for (const UTableauHISMComponent* Component : TableauActor->FoliageComponents)
		{
			Box += Component->Bounds.GetBox();
		}

		for (const UStaticMeshComponent* StaticMesh : TableauActor->StaticMeshComponents)
		{
			Box += StaticMesh->Bounds.GetBox();
		}

		for (const UChildActorComponent* ChildActor : TableauActor->ChildActorComponents)
		{
			Box += ChildActor->Bounds.GetBox();
		}
	}

	return Box;
}

FBox UTableauComponent::GetBoundingBox(const TArray<FTableauInstanceTracker>& SubInstances) const
{
	// Recurse the Instances registry and return the bounds of the local strata.
	
	FBox Box(ForceInit);

	for (const FTableauInstanceTracker& Instance: SubInstances)
	{
		if (Instance.TableauInstance!=nullptr)
		{
			for (const UActorComponent* ActorComponent : Instance.TableauInstance->GetComponents())
			{
				if (const UPrimitiveComponent * PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent))
				{
					Box += PrimitiveComponent->Bounds.GetBox();
				}
			}
		}

		// Recurse
		Box += GetBoundingBox(Instance.SubInstances);
	}

	return Box;

}

void UTableauComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	for (FTableauInstanceTracker& Instance : TableauInstances)
	{
		Instance.Apply([](AActor* InActor)
			{
				if (InActor && !InActor->IsPendingKillPending())
				{
					InActor->GetWorld()->DestroyActor(InActor, true);
				}
			});

	}

	ClearInstances();
}

void UTableauComponent::PostEditImport()
{
	AActor* Owner = GetOwner();
	OnTableauDuplicate.ExecuteIfBound(Owner);
}

UObject* UTableauComponent::SafelyResolveSoftPath(const FSoftObjectPath& path) const
{
	// Trying to resolve an unloaded object will crash Editor (v4.24).
	// It's sort of slow and not ideal but we need to be certain that the requested Object is
	// loaded first.
	path.TryLoad();
	return path.ResolveObject();
}

void UTableauComponent::ClearInstances()
{
	// Recursively clear the Instances registry starting with the deepest arrays first.
	ClearInstances(TableauInstances);
}

void UTableauComponent::ClearInstances(TArray<FTableauInstanceTracker>& SubInstances)
{
	// Recurse
	for (FTableauInstanceTracker& Instance : SubInstances)
	{
		ClearInstances(Instance.SubInstances);
	}

	SubInstances.Empty();
}

void UTableauComponent::RestoreInstances()
{
	if (!IsValid())
	{
		// Invalid tracking system. Simplest and quickest solution is to rebuild it from child actors.
		ClearInstances();
		
		TArray<AActor*> Children;
		GetOwner()->GetAttachedActors(Children);

		for (AActor* ChildActor : Children)
		{
			RegisterInstance(TWeakObjectPtr<AActor>(ChildActor));
		}
	}	
}

bool UTableauComponent::IsValid() const
{
	// Check all of the Instance Trackers. If they're all still pointing
	// to valid actors, return true. A false return indicates that the 
	// Component is no longer maintaining a valid tracking system and 
	// should rebuild it.
	for (const auto& InstanceTracker : TableauInstances)
	{
		if (!InstanceTracker.TableauInstance.IsValid())
		{
			return false;
		}
	}

	return true;
}

void UTableauComponent::RegisterInstance(TWeakObjectPtr<AActor> InInstance, FTableauInstanceTracker* Parent)
{
	TArray<FTableauInstanceTracker>* InstanceContainer = nullptr;
	if (Parent == nullptr)
	{
		// No parent indicates root level registration
		InstanceContainer = &TableauInstances;
	}
	else
	{
		InstanceContainer = &(Parent->SubInstances);
	}

	FTableauInstanceTracker InstanceTracker;
	InstanceTracker.TableauInstance = InInstance;
	InstanceContainer->Add(InstanceTracker);	
}

FTableauInstanceTracker* UTableauComponent::GetLastInstanceTracker()
{
	// The most recent Instance registered will be the last element of
	// the last element of the last elements &c.
	if (TableauInstances.Num() == 0)
	{
		return nullptr;
	}
	else
	{
		if (TableauInstances.Last().SubInstances.Num() > 0)
		{
			return &TableauInstances.Last().SubInstances.Last();
		}
		else
		{
			return &TableauInstances.Last();
		}
	}
}
