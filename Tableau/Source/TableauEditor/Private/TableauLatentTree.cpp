#include "TableauLatentTree.h"

// Local Includes
#include "TableauUtils.h"



//////////////////////////////////////////////////
// FTableauLatentTree

FTableauLatentTree::FTableauLatentTree(const UTableauAsset* InTableauAsset, TSharedPtr<FTableauFilterSampler> InFilter, bool bUseAssetEditorMode)
	: TableauAsset(InTableauAsset)
	, Filter(InFilter)
	, bAssetEditorMode(bUseAssetEditorMode)
{
}

void FTableauLatentTree::EvaluateLatentTree(int32 Seed)
{
	// Recurse the latent Tableau actor structure, depth first, to construct a list
	// of cached actors to be instantiated within the Tableau actor.

	// The latent tree consists of Tableau Element nodes. We'll need to wrap
	// the Tableau Object Ref root to pass it in.
	FTableauAssetElement Root;
	Root.AssetReference = FSoftObjectPath(TableauAsset);
	Root.LocalTransform = FTransform::Identity;

	// Begin the recursion with identity transform. The transform stack will
	// be appended as we dive into the tree.
	Recipe.Empty();
	TArray<UTableauAsset*> EvaluationHistory;
	EvaluateTableau(Root, FTransform::Identity, Seed, Recipe, EvaluationHistory);
}

TArray<FTableauRecipeNode>& FTableauLatentTree::GetRecipe()
{
	return Recipe;
}

TMap<UFoliageType*, TUniquePtr<FTableauFoliage>>& FTableauLatentTree::GetFoliages()
{
	return Foliages;
}

FTableauRecipeNode* FTableauLatentTree::EvaluateTableau(const FTableauAssetElement& TableauElement, const FTransform& CurrXform, int32 LocalSeed, TArray<FTableauRecipeNode>& CurrRecipe, TArray<UTableauAsset*> EvaluationHistory)
{

	FSoftObjectPath Ref = TableauElement.AssetReference;
	if (!Ref.IsAsset())
	{
		return EvaluateLeafElement(TableauElement, CurrXform, CurrRecipe);
	}
	else
	{
		UObject* Object = SafelyResolveSoftPath(Ref);
		if (UTableauAsset* SubTableauAsset = Cast<UTableauAsset>(Object))
		{
			// If the current Tableau asset to be evaluated can be found in an upstream evaluation,
			// we risk entering an infinite loop during latent tree expression. 
			// Check to see if this Tableau has been previously expressed in this branch.
			// If it hasn't, record it. If it has, issue a warning and abort this branch.
			if (EvaluationHistory.Contains(SubTableauAsset))
			{
				UE_LOG(LogTableau, Warning, TEXT("Branch evaluation aborted to avert risk of infinite loop. Self reference found in %s!!!"), *Ref.ToString());
				return nullptr;
			}
			else
			{
				EvaluationHistory.Add(SubTableauAsset);
			}

			if (SubTableauAsset->TableauElement.Num() == 0)
			{
				return nullptr;
			}

			// We don't use hierarchical composition in the asset editor
			TEnumAsByte<ETableauEvaluationMode::Type> EvaluationMode = SubTableauAsset->EvaluationMode;
			if (bAssetEditorMode)
			{
				if (EvaluationMode == ETableauEvaluationMode::HierarchicalComposition)
				{
					EvaluationMode = ETableauEvaluationMode::Composition;
				}
			}
			
			switch (EvaluationMode)
			{
				case ETableauEvaluationMode::Superposition:
				{
					const FTableauAssetElement* Element = SelectRandomElement(SubTableauAsset, LocalSeed);
					if (Element)
					{
						FVector Location = (Element->LocalTransform * CurrXform).GetLocation();
						if (Filter->Sample(Location))
						{
							if (Element->bDeterministic)
							{
								LocalSeed = Element->Seed - 1;
							}
							FTransform LocalTransform(Element->LocalTransform * CurrXform);
							FTableauUtils::JitterTransform(LocalSeed, Element->MinScaleJitter, Element->MaxScaleJitter, Element->bSpinZAxis, LocalTransform);
							return EvaluateTableau(*Element, LocalTransform, FTableauUtils::NextSeed(LocalSeed), CurrRecipe, EvaluationHistory);
						}
						else
						{
							return nullptr;
						}
					}
					else
					{
						// Superposition returns Nothing.
						return nullptr;
					}

				}

				case ETableauEvaluationMode::HierarchicalComposition:
				{
					return EvaluateCompositeElement(SubTableauAsset, CurrXform, LocalSeed, true, CurrRecipe, EvaluationHistory);
				}

				case ETableauEvaluationMode::Composition:
				{
					return EvaluateCompositeElement(SubTableauAsset, CurrXform, LocalSeed, false, CurrRecipe, EvaluationHistory);
				}

				default:
				{
					UE_LOG(LogTableau, Error, TEXT("Unrecognized Evaluation Mode encountered."));
					return nullptr;
				}
			}
		}
		else // Leaf node
		{
			
			// Foliage Types are converted to the associated static mesh if we're inasset editor mode
			// and passed to the foliage builder otherwise.
			if (UFoliageType* FoliageType = Cast<UFoliageType>(Object))
			{
				if (!bAssetEditorMode)
				{
					// Catch any leaf nodes configured with a Foliage Type asset and feed them into the foliage builder. 
					if (!Foliages.Contains(FoliageType))
					{
						Foliages.Add(FoliageType, TUniquePtr<FTableauFoliage>(new FTableauFoliage(FoliageType, CurrXform, TableauElement.Name, TableauElement.bSnapToFloor)));
					}
					else
					{
						Foliages[FoliageType]->AddInstance(CurrXform);
					}

					// We don't allow hierarchical composition with foliage.
					return nullptr;
				}
				
				// convert to static mesh
				const UFoliageType_InstancedStaticMesh* FoliageType_InstancedStaticMesh = Cast<UFoliageType_InstancedStaticMesh>(FoliageType);
				Object = FoliageType_InstancedStaticMesh->GetStaticMesh();
			}

			// Produce a recipe node.
			return EvaluateLeafElement(TableauElement, CurrXform, CurrRecipe, Object);
			
		}
	}
}

FTableauRecipeNode* FTableauLatentTree::EvaluateLeafElement(const FTableauAssetElement& TableauElement, const FTransform& CurrXform, TArray<FTableauRecipeNode>& CurrRecipe, UObject* Asset)
{
	CurrRecipe.Add(ConvertTableauElementToRecipeNode(TableauElement, CurrXform, Asset));
	return &CurrRecipe.Last();
}

FTableauRecipeNode* FTableauLatentTree::EvaluateCompositeElement(UTableauAsset* InTableauAsset, const FTransform& CurrXform, int32 LocalSeed, bool bHierarchical, TArray<FTableauRecipeNode>& CurrRecipe, TArray<UTableauAsset*> EvaluationHistory)
{
	int32 Seed = LocalSeed;

	// It's possible that one of the evaluated elements of the composition will return no actors.
	// Move to the next one until an actor is manifested.
	int32 FirstManifest = 0;
	FTableauRecipeNode* FirstRecipe = nullptr;
	for (; FirstManifest < InTableauAsset->TableauElement.Num(); ++FirstManifest)
	{
		Seed = FTableauUtils::NextSeed(Seed);
		FVector Location = (InTableauAsset->TableauElement[FirstManifest].LocalTransform * CurrXform).GetLocation();
		if (Filter->Sample(Location))
		{
			if (InTableauAsset->TableauElement[FirstManifest].bDeterministic)
			{
				Seed = InTableauAsset->TableauElement[FirstManifest].Seed;
			}

			if (FuzzyTrue(InTableauAsset->TableauElement[FirstManifest].Weight, Seed))
			{
				FTransform LocalTransform(InTableauAsset->TableauElement[FirstManifest].LocalTransform * CurrXform);
				FTableauUtils::JitterTransform(Seed, InTableauAsset->TableauElement[FirstManifest].MinScaleJitter, InTableauAsset->TableauElement[FirstManifest].MaxScaleJitter, InTableauAsset->TableauElement[FirstManifest].bSpinZAxis, LocalTransform);
				FirstRecipe = EvaluateTableau(InTableauAsset->TableauElement[FirstManifest], LocalTransform, Seed, CurrRecipe, EvaluationHistory);
				if (FirstRecipe)
				{
					break;
				}
			}
		}
	}

	// Evaluate the remaining elements.
	for (int32 Index = FirstManifest + 1; Index < InTableauAsset->TableauElement.Num(); ++Index)
	{
		Seed = FTableauUtils::NextSeed(Seed);
		FVector Location = (InTableauAsset->TableauElement[FirstManifest].LocalTransform * CurrXform).GetLocation();
		if (Filter->Sample(Location))
		{
			if (InTableauAsset->TableauElement[Index].bDeterministic)
			{
				Seed = InTableauAsset->TableauElement[Index].Seed;
			}

			if (FuzzyTrue(InTableauAsset->TableauElement[Index].Weight, Seed))
			{
				FTransform LocalTransform(InTableauAsset->TableauElement[Index].LocalTransform * CurrXform);
				FTableauUtils::JitterTransform(Seed, InTableauAsset->TableauElement[Index].MinScaleJitter, InTableauAsset->TableauElement[Index].MaxScaleJitter, InTableauAsset->TableauElement[Index].bSpinZAxis, LocalTransform);

				if (bHierarchical)
				{
					EvaluateTableau(InTableauAsset->TableauElement[Index], LocalTransform, Seed, FirstRecipe->SubRecipe, EvaluationHistory);
				}
				else
				{
					EvaluateTableau(InTableauAsset->TableauElement[Index], LocalTransform, Seed, CurrRecipe, EvaluationHistory);
				}
			}
		}
	}

	return FirstRecipe;
}

const FTableauAssetElement* FTableauLatentTree::SelectRandomElement(const UTableauAsset* InTableauAsset, int32 LocalSeed) const
{
	// Method presumes that Tableau has at least 1 element.
	check(InTableauAsset->TableauElement.Num() > 0);

	// Find total weight
	float TotalWeight = InTableauAsset->NothingWeight;
	for (const FTableauAssetElement& TableauElement : InTableauAsset->TableauElement)
	{
		TotalWeight += TableauElement.Weight;
	}

	// Get a weighted random index
	FRandomStream Stream(LocalSeed);
	const float Cutoff = Stream.FRand() * TotalWeight;

	// If our roulette falls below NothingWeight, we'll not emit anything.
	if (Cutoff < InTableauAsset->NothingWeight)
	{
		return nullptr;
	}

	// Select the element
	float Accumulated = InTableauAsset->NothingWeight;
	int Index = 0;
	for (; Index < InTableauAsset->TableauElement.Num(); Index++)
	{
		Accumulated += InTableauAsset->TableauElement[Index].Weight;
		if (Accumulated >= Cutoff)
		{
			break;
		}
	}

	return &InTableauAsset->TableauElement[Index];

}

UObject* FTableauLatentTree::SafelyResolveSoftPath(const FSoftObjectPath& path) const
{
	FSoftObjectPtr SoftObjectPtr(path);
	if (SoftObjectPtr.IsValid())
	{
		return SoftObjectPtr.Get();
	}
	else
	{
		return SoftObjectPtr.LoadSynchronous();
	}
}

FTableauRecipeNode FTableauLatentTree::ConvertTableauElementToRecipeNode(const FTableauAssetElement& TableauElement, const FTransform& Transform, UObject* Asset) const
{
	FTableauRecipeNode RecipeNode;
	RecipeNode.Name = TableauElement.Name;
	RecipeNode.LocalTransform = Transform;
	RecipeNode.bSnapToFloor = TableauElement.bSnapToFloor;
	RecipeNode.bUseConfig = !bAssetEditorMode && TableauElement.bUseConfig;

	if (RecipeNode.bUseConfig)
	{
		RecipeNode.AssetConfig = TableauElement.AssetConfig;
	}
	else
	{
		// Convert any Foliage type to its wrapped Static Mesh asset.


		RecipeNode.AssetReference = TWeakObjectPtr<UObject>(Asset);
	}

	return RecipeNode;
}

bool FTableauLatentTree::FuzzyTrue(float Probability, int32 Seed) const
{
	// Return true if randomly generated sample is below the probability threshold.
	FRandomStream Stream(Seed);

	return (Stream.FRand() < Probability);
}

