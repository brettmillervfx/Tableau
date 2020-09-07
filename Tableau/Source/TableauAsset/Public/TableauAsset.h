#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

// Generated Include
#include "TableauAsset.generated.h"

/*
* The Tableau Asset holds configuration information for generating a set of actors in the scene.
* As such, it contains no renderable models per se, only a list of references to other assets. 
* The Tableau Asset can be one of several configurations which determines how the the list of asset
* references will be used to generate the set of Instanced Actors.
*
* In Composition mode (the default), each asset in the list will be Instantiated into the scene, relative
* to the Tableau Actor. The first element in the list determines the pivot and all other Instanced Actors are relative 
* to this one. If a Tableau asset is encountered in the list, a recursive process is initiated and the Tableau begins
* to unfold into a complex scene as deeper Tableaux are "collapsed".
*
* In Superposition mode, one asset is selected based on a weighted russian roulette. The Tableau Asset has a property called
* NothingWeight: this is the weighted chance that a Superposition will produce nothing at all.
*
* HierarchicalComposition mode works exactly like Composition mode, except that it introduces hierachy to the otherwise flat structure.
* The first Element in the asset will "own" all of the others. This relationship is only important with respect to the snap to floor
* operation. Any leaf object generated that does not have the Snap toggle set will simply follow the transform of it's owner when the owner
* is snapped. 
*
* Elements in the asset list refer back to the asset the will instance from. However, the Instance produced is generated from the
* stored clipboard string, allowing actor specific tweaks to survive the process, eg. vertex painting, light adjustments, material overrides, etc.
* Each Element also flags if it is "snapable". This attribute has no meaning if applied to a Tableau asset. For any other asset, ie. leaf nodes
* in the expressed structure, the flag determines if the snap to floor operation will be applied. 
*/

UENUM()
namespace ETableauEvaluationMode
{
	enum Type
	{
		Composition,
		Superposition,
		HierarchicalComposition
	};
}

USTRUCT()
struct TABLEAUASSET_API FTableauAssetElement
{
	GENERATED_BODY()

public:

	FTableauAssetElement();
	FTableauAssetElement(const FName InName, const FSoftObjectPath& InAssetReference, const FString& InAssetConfig, const FTransform& InLocalTransform, uint32 InSeed);

public:

	UPROPERTY(Category = Tableau, EditAnywhere)
	FName Name;

	UPROPERTY(Category = Tableau, EditAnywhere)
	FSoftObjectPath AssetReference;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Allow Snapping")
	bool bSnapToFloor = true;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Use Captured Config")
	bool bUseConfig = false;

	UPROPERTY()
	FString AssetConfig;

	UPROPERTY(Category = Tableau, EditAnywhere)
	FTransform LocalTransform;

	UPROPERTY(Category = Tableau, EditAnywhere)
	float Weight = 1.0;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Deterministic")
	bool bDeterministic = false;

	UPROPERTY(Category = Tableau, EditAnywhere)
	uint32 Seed = 0;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Spin Z Axis")
	bool bSpinZAxis = false;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Minimum Scale Jitter")
	float MinScaleJitter = 1.0f;

	UPROPERTY(Category = Tableau, EditAnywhere, DisplayName = "Maximum Scale Jitter")
	float MaxScaleJitter = 1.0f;

};

UCLASS(Blueprintable, BlueprintType, ClassGroup = Tableau, Category = "Tableau")
class TABLEAUASSET_API UTableauAsset : public UObject
{
	GENERATED_BODY()

public:

	//~ Begin UObject interface
	UTableauAsset(const FObjectInitializer& ObjectInitializer);
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	//~ End UObject interface

	void AddElement(const FName Name, const FSoftObjectPath& AssetReference, const FString& AssetConfig, const FTransform& LocalTransform, uint32 Seed=0);
	void AddElement(const FTableauAssetElement& NewElement);
	void ClearElements();

	void ReplaceTableauReferences(const FSoftObjectPath& ReferenceToReplace, const FSoftObjectPath& ReferenceReplacement);

public:

	// Super/Comp mode of evaluation
	UPROPERTY(Category = Tableau, EditAnywhere)
	TEnumAsByte<ETableauEvaluationMode::Type> EvaluationMode;

	UPROPERTY(Category = Tableau, EditAnywhere)
	FString Notes;

	UPROPERTY(Category = Tableau, EditAnywhere)
	float NothingWeight;

	// Array of contained elements
	UPROPERTY(EditAnywhere, Category = Tableau)
	TArray<FTableauAssetElement> TableauElement;

#if WITH_EDITORONLY_DATA
	// Source for reimport
	UPROPERTY(Category = SourceAsset, VisibleAnywhere)
	FString SourceFilePath;
#endif
	
};
