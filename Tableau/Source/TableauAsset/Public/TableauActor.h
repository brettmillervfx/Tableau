#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// Local Includes
#include "TableauComponent.h"
#include "TableauHISMComponent.h"
// Generated Include
#include "TableauActor.generated.h"


/*
* The Tableau Actor is not much more than an anchor to parent the Instanced Actors to
* and contain the Tableau Component. 
* The actor is tagged to indicate if it is a transient preview actor.
*/

namespace TableauActorConstants
{
	const FName TABLEAU_ELEMENT_TAG = TEXT("TTableauElement");
	const FName TABLEAU_PREVIEW_TAG = TEXT("TTableauPreview");
	const FName TABLEAU_SNAPTOFLOOR_TAG = TEXT("TTableauSnapToFloor");
}

UCLASS(Blueprintable, BlueprintType)
class TABLEAUASSET_API ATableauActor : public AActor
{
	GENERATED_BODY()

public:
	
	//~ Begin AActor interface
	ATableauActor(const FObjectInitializer& ObjectInitializer);

#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray< UObject* >& Objects) const override;
#endif
	//~ End AActor interface

	UTableauComponent* GetTableauComponent() const;
	
public:
	UPROPERTY(Category = Tableau, VisibleAnywhere, BlueprintReadWrite)
	UTableauComponent* TableauComponent;

	UPROPERTY()
	TArray<UStaticMeshComponent*> StaticMeshComponents;

	UPROPERTY()
	TArray<UChildActorComponent*> ChildActorComponents;
	
	UPROPERTY()
	TArray<UTableauHISMComponent*> FoliageComponents;

};
