
#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"

// Local Includes
#include "TableauAsset.h"

// Generated Include
#include "TableauComponent.generated.h"

/*
* The Tableau Component is responsible for keeping track of the Instanced Actors resulting from the 
* evaluation of the associated Tableau Asset. Note that there is very little core logic contained by
* the Component itself: generation and management of the Instances is handled by the FTableauActorManager
* class and the various structs and classes of TableauUtils.
*
* The registry of Instances maintained by the Component is structured as a forest, ie. an array of trees.
* In most cases the registry will probably just be a flat list. Hierarchy is only introduced when snapping
* relationships appear when a HierarchicalComposition Tableau is encountered in evaluation.
*
* The Tableau Component holds reference to the Tableau Asset and a Seed value for generating the Instances.
* It is also responsible for generating it's bounds and Color for drawing.
*/

USTRUCT()
struct FTableauInstanceTracker
{
	GENERATED_BODY()

	TWeakObjectPtr<AActor> TableauInstance;

	TArray<FTableauInstanceTracker> SubInstances;

	// Perform function on each leaf member of tree.
	void Apply(void (*ApplyFunc)(AActor*))
	{
		if (TableauInstance.IsValid())
		{
			ApplyFunc(TableauInstance.Get());
		}

		for (FTableauInstanceTracker& Instance : SubInstances)
		{
			Instance.Apply(ApplyFunc);
		}
	}
};

USTRUCT()
struct TABLEAUASSET_API FFilterActor
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = Tableau, EditAnywhere)
	TSoftObjectPtr<AActor> Actor;

	UPROPERTY(Category = Tableau, EditAnywhere)
	float ExpandVolume = 250.0f;
};

USTRUCT()
struct TABLEAUASSET_API FFilterWeightmap
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = Tableau, EditAnywhere)
	FName LayerName;

	UPROPERTY(Category = Tableau, EditAnywhere)
	float WeightThreshold = 0.5f;
};

USTRUCT()
struct TABLEAUASSET_API FFilterTag
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = Tableau, EditAnywhere)
	FName Tag;

	UPROPERTY(Category = Tableau, EditAnywhere)
	float Tolerance = 1000.0f;
};


UCLASS()
class TABLEAUASSET_API UTableauComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UTableauComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin UPrimitiveComponent interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void PostEditImport() override;
	//~ End UPrimitiveComponent interface

	UTableauAsset* GetTableau() const;

	bool SetTableau(UTableauAsset* InTableau);

	void SetSeed(int32 InSeed);

	// Reset the instances registry. This does not delete the Instanced Actors! For
	// that use FTableauActorManager::DeleteInstances.
	void ClearInstances();

	// When an Instanced Actor is spawned, it must be registered with the Component.
	// Parent indicates which tree the reference should be added to; nullptr indicates
	// that it should be added as a new root.
	void RegisterInstance(TWeakObjectPtr<AActor> InInstance, FTableauInstanceTracker* Parent = nullptr);

	// Get a pointer to the most recently registered instance.
	FTableauInstanceTracker* GetLastInstanceTracker();

	// Validate and update instance tracking.
	void RestoreInstances();

	TArray<FTableauInstanceTracker> GetInstances() const
	{ 
		return TableauInstances; 
	}

	FLinearColor GetColor() const;
	FBox GetBoundingBox() const;

	DECLARE_DELEGATE_OneParam(FOnTableauDuplicateDelegate, AActor*);
	static FOnTableauDuplicateDelegate OnTableauDuplicate;

private:
	FBox GetBoundingBox(const TArray<FTableauInstanceTracker>& SubInstances) const;
	UObject* SafelyResolveSoftPath(const FSoftObjectPath& path) const;
	void ClearInstances(TArray<FTableauInstanceTracker>& SubInstances);
	bool IsValid() const;

public:
	UPROPERTY(BlueprintReadWrite, Category = Tableau, EditAnywhere, meta = (DisplayThumbnail = "true", AllowPrivateAccess = "true"))
	UTableauAsset* Tableau;

	UPROPERTY(BlueprintReadWrite, Category = Tableau, EditAnywhere)
	int32 Seed;

	UPROPERTY(BlueprintReadWrite, Category = Tableau, EditAnywhere, meta = (DisplayName = "Automatically Snap on Regeneration"))
	bool bAutoSnap = false;

	UPROPERTY(BlueprintReadWrite, Category = Tableau, EditAnywhere, meta = (DisplayName = "Ignore Tableau Exclusion Volumes"))
	bool bIgnoreExclusion = true;

	UPROPERTY(BlueprintReadWrite, Category = Tableau, EditAnywhere, meta = (DisplayName = "Exclude HISM Components from HLOD Generation"))
	bool bExcludeHISMsFromHLOD = false;

	UPROPERTY(Category = Tableau, EditAnywhere)
	TArray<FFilterActor> FilterActors;

	UPROPERTY(Category = Tableau, EditAnywhere)
	TArray<FFilterWeightmap> FilterWeightmaps;

	UPROPERTY(Category = Tableau, EditAnywhere)
	TArray<FFilterTag> FilterTags;

private:
	UPROPERTY()
	TArray<FTableauInstanceTracker> TableauInstances;

};


