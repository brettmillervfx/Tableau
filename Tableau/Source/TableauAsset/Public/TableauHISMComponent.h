#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "TableauHISMComponent.generated.h"



/**
 *  TableauHISMComponent is a simple extension of HISM that maintains a link to its foliage type in order
 *  to facilitate a transform to the Foliage Tool.
 */
UCLASS()
class TABLEAUASSET_API UTableauHISMComponent : public UHierarchicalInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	UTableauHISMComponent(const FObjectInitializer& ObjectInitializer);

	// Set Component to be excluded from HLOD generation
	void ExcludeFromHLOD();

public:
	UPROPERTY()
	FSoftObjectPath FoliageType;

	UPROPERTY()
	bool bSnappable;

	UPROPERTY()
	bool bAlignToNormal;

	// If snappable, instances will only be placed on surfaces sloping in the specified angle range from the horizontal
	UPROPERTY()
	FFloatInterval GroundSlopeAngle;
	
};
