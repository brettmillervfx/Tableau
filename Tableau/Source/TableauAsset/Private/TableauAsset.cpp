
#include "TableauAsset.h"


FTableauAssetElement::FTableauAssetElement()
	: bSnapToFloor(true)
	, bUseConfig(true)
	, LocalTransform(FTransform::Identity)
	, Weight(1.0)
	, bDeterministic(false)
	, Seed(0)
{
}

FTableauAssetElement::FTableauAssetElement(const FName InName, const FSoftObjectPath& InAssetReference, const FString& InAssetConfig, const FTransform& InLocalTransform, uint32 InSeed)
	: Name(InName)
	, AssetReference(InAssetReference)
	, bSnapToFloor(true)
	, bUseConfig(false)
	, AssetConfig(InAssetConfig)
	, LocalTransform(InLocalTransform)
	, Weight(1.0)
	, bDeterministic(false)
	, Seed(InSeed)
	, bSpinZAxis(false)
	, MinScaleJitter(1.0f)
	, MaxScaleJitter(1.0f)
{
}


UTableauAsset::UTableauAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NothingWeight(0.f)
{
	TableauElement.Empty();
}

void UTableauAsset::AddElement(const FName Name, const FSoftObjectPath& AssetReference, const FString& AssetConfig, const FTransform& LocalTransform, uint32 Seed)
{
	FTableauAssetElement NewElement(Name, AssetReference, AssetConfig, LocalTransform, Seed);
	TableauElement.Add(NewElement);
}

void UTableauAsset::AddElement(const FTableauAssetElement& NewElement)
{
	TableauElement.Add(NewElement);
}

void UTableauAsset::ClearElements()
{
	TableauElement.Empty();
}

void UTableauAsset::ReplaceTableauReferences(const FSoftObjectPath& ReferenceToReplace, const FSoftObjectPath& ReferenceReplacement)
{
	// Iterate the Tableau elements. If the Asset Reference matches the ReferenceToReplace,
	// change it to the ReferenceReplacement path.
	for (FTableauAssetElement& Element : TableauElement)
	{
		if (Element.AssetReference == ReferenceToReplace)
		{
			Element.AssetReference = ReferenceReplacement;
		}
	}
}

void UTableauAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (EvaluationMode == ETableauEvaluationMode::Type::Composition)
	{
		OutTags.Add(FAssetRegistryTag("Mode", "Composition", FAssetRegistryTag::TT_Alphabetical));
	}
	else
	{
		OutTags.Add(FAssetRegistryTag("Mode", "Superposition", FAssetRegistryTag::TT_Alphabetical));
	}

	OutTags.Add(FAssetRegistryTag("Notes", Notes, FAssetRegistryTag::TT_Alphabetical));
}
