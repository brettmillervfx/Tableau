#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Factories/Factory.h"

// Local Includes
#include "TableauAsset.h"

// Generated Include
#include "TableauAssetFactory.generated.h"


UCLASS(hidecategories = Object)
class UTableauAssetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UTableauAssetFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UAssetFactory interface
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override;
	virtual FString GetDefaultNewAssetName() const override;
	//~ End UAssetFactory interface

private:
	FString ConstructNameFromActor(AActor* InActor) const;

};
