
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "TableauExclusionVolume.generated.h"

/**
 * Basic volume used to prevent Tableau spawning.
 */
UCLASS()
class TABLEAUASSET_API ATableauExclusionVolume : public AVolume
{
	GENERATED_BODY()

public:
	ATableauExclusionVolume(const FObjectInitializer& ObjectInitializer);
	
};
