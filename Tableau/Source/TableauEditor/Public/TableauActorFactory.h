#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"

// Generated Include
#include "TableauActorFactory.generated.h"


UCLASS()
class TABLEAUEDITOR_API UTableauActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:

	UTableauActorFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UActorFactory interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	virtual bool PreSpawnActor(UObject* Asset, FTransform& InOutLocation) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	//~ End UActorFactory interface
	
	void SetSeed(int32 InSeed);
	void RandomizeSeed();


private:
	UPROPERTY()
	int32 Seed;

};
