
#include "TableauActorFactory.h"

// Local Includes
#include "TableauActorManager.h"
#include "TableauActor.h"
#include "TableauAsset.h"
#include "TableauComponent.h"

UTableauActorFactory::UTableauActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("Tableau", "TableauFactoryDisplayName", "Add Tableau");
	NewActorClass = ATableauActor::StaticClass();

	RandomizeSeed();
}

bool UTableauActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (AssetData.IsValid() && AssetData.GetClass()->IsChildOf(UTableauAsset::StaticClass()))
	{
		return true;
	}
	else
	{
		OutErrorMsg = NSLOCTEXT("Tableau", "CanCreateActorFrom_NoTableau", "No Tableau was specified.");
		return false;
	}
}

UObject* UTableauActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	check(ActorInstance->IsA(NewActorClass));
	ATableauActor* TableauActor = CastChecked<ATableauActor>(ActorInstance);
	check(TableauActor->GetTableauComponent());
	return TableauActor->GetTableauComponent()->GetTableau();
}

bool UTableauActorFactory::PreSpawnActor(UObject* Asset, FTransform& InOutLocation)
{
	UTableauAsset* Tableau = CastChecked<UTableauAsset>(Asset);

	return (Tableau != nullptr);
}

void UTableauActorFactory::PostSpawnActor(UObject* Asset, AActor* InNewActor)
{
	Super::PostSpawnActor(Asset, InNewActor);

	UTableauAsset* Tableau = CastChecked<UTableauAsset>(Asset);

	ATableauActor* TableauActor = CastChecked<ATableauActor>(InNewActor);
	UTableauComponent* TableauComponent = TableauActor->GetTableauComponent();
	check(TableauComponent);

	TableauComponent->UnregisterComponent();

	TableauComponent->SetTableau(Tableau);
	TableauComponent->SetSeed(Seed);

	TableauComponent->RegisterComponent();

	
	FTableauActorManager Manager(TableauActor);
	Manager.UpdateInstances(TableauActor->bIsEditorPreviewActor);
	
	// Next Actor placed gets a new seed.
	if (!TableauActor->bIsEditorPreviewActor)
	{
		RandomizeSeed();
	}
	
}

void UTableauActorFactory::SetSeed(int32 InSeed)
{
	Seed = InSeed;
}

void UTableauActorFactory::RandomizeSeed()
{
	Seed = FMath::RandHelper(TNumericLimits<int32>::Max());
}
