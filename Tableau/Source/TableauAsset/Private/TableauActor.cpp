
#include "TableauActor.h"
#include "TableauAssetModule.h"


// Sets default values
ATableauActor::ATableauActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	TableauComponent = CreateDefaultSubobject<UTableauComponent>(TEXT("TableauComponent"));
	TableauComponent->Mobility = EComponentMobility::Static;
	SetRootComponent(TableauComponent);

}

#if WITH_EDITOR
bool ATableauActor::GetReferencedContentObjects(TArray< UObject* >& Objects) const
{
	Super::GetReferencedContentObjects(Objects);
	Objects.Add(TableauComponent->GetTableau());
	return true;
}
#endif

UTableauComponent* ATableauActor::GetTableauComponent() const
{ 
	return TableauComponent; 
}








