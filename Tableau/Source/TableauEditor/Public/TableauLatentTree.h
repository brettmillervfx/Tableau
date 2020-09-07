#pragma once

// Editor Includes
#include "CoreMinimal.h"
#include "Editor.h"
#include "FoliageType.h"
#include "FoliageType_InstancedStaticMesh.h"

// Local Includes
#include "TableauFoliage.h"
#include "TableauFilter.h"


/*
* Node describing the required information to spawn an actor comprising the Tableau.
*/
struct FTableauRecipeNode
{
	FTableauRecipeNode()
		: Name("TableauInstance")
		, AssetConfig("")
		, AssetReference(nullptr)
		, LocalTransform(FTransform::Identity)
		, bSnapToFloor(true)
		, bUseConfig(false)
	{
		SubRecipe.Empty();
	}

	// Name of the original actor cloned for this Tableau
	FName Name;

	// Clipboard contents of the actor resulting from a copy/paste edact operation.
	FString AssetConfig;

	// Pointer to the referenced asset.
	TWeakObjectPtr<UObject> AssetReference;

	// Transform of the actor local to the Tableau actor
	FTransform LocalTransform;

	// Should this actor snap to the floor?
	bool bSnapToFloor;

	// Should this recipe express a configured asset instead of using the asset factory?
	bool bUseConfig;

	// Recipes for actors subordinate to this one
	TArray<FTableauRecipeNode> SubRecipe;
};

/*
* Utility class for unpacking a latent tree description from latently nested Tableau assets.
*/
class FTableauLatentTree
{

public:

	FTableauLatentTree(const UTableauAsset* InTableauAsset, TSharedPtr<FTableauFilterSampler> InFilter, bool bUseAssetEditorMode=false);

	// Prepare Recipe for spawning by evaluating the latent Tableau tree using the provided Seed for random Superposition selections.
	void EvaluateLatentTree(int32 Seed);
	TArray<FTableauRecipeNode>& GetRecipe();
	TMap<UFoliageType*, TUniquePtr<FTableauFoliage>>& GetFoliages();

	// Select random element from Superposition list.
	const FTableauAssetElement* SelectRandomElement(const UTableauAsset* TableauAsset, int32 LocalSeed) const;

private:
	FTableauRecipeNode* EvaluateTableau(const FTableauAssetElement& TableauElement, const FTransform& CurrXform, int32 LocalSeed, TArray<FTableauRecipeNode>& CurrRecipe, TArray<UTableauAsset*> EvaluationHistory);
	FTableauRecipeNode* EvaluateLeafElement(const FTableauAssetElement& TableauElement, const FTransform& CurrXform, TArray<FTableauRecipeNode>& CurrRecipe, UObject* Asset = nullptr);
	FTableauRecipeNode* EvaluateCompositeElement(UTableauAsset* InTableauAsset, const FTransform& CurrXform, int32 LocalSeed, bool bHierarchical, TArray<FTableauRecipeNode>& CurrRecipe, TArray<UTableauAsset*> EvaluationHistory);

	UObject* SafelyResolveSoftPath(const FSoftObjectPath& path) const;
	FTableauRecipeNode ConvertTableauElementToRecipeNode(const FTableauAssetElement& TableauElement, const FTransform& Transform, UObject* Asset) const;

	bool FuzzyTrue(float Probability, int32 Seed) const;

public:
	UPROPERTY()
	const UTableauAsset* TableauAsset;

	TArray<FTableauRecipeNode> Recipe;

	TMap<UFoliageType*, TUniquePtr<FTableauFoliage>> Foliages;

	TSharedPtr<FTableauFilterSampler> Filter;

	bool bAssetEditorMode;

};
