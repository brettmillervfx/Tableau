
#include "TableauAssetFactory.h"

// Engine Includes
#include "Editor.h"
#include "Engine/Selection.h"

// Local Includes
#include "TableauEditorModule.h"
#include "TableauActor.h"
#include "TableauComponent.h"
#include "TableauUtils.h"




UTableauAssetFactory::UTableauAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UTableauAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = false;
}


UObject* UTableauAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	// Filter the currently selected actors by accepted types
	// then store the class and config information in the elements array.
	// The new asset will pivot on the last selected actor.
	UTableauAsset* NewTableauAsset = NewObject<UTableauAsset>(InParent, InClass, InName, Flags);

	TArray<AActor*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

	if (SelectedActors.Num() > 0)
	{
		FTableauUtils::FillTableauAssetElements(NewTableauAsset, SelectedActors);
		FTableauUtils::ReplaceActorsWithTableau(NewTableauAsset, SelectedActors);
	}
	else
	{
		// An empty Tableau is the usual setup for a Superposition of assets.
		NewTableauAsset->EvaluationMode = ETableauEvaluationMode::Type::Superposition;
	}

	return NewTableauAsset;
}

bool UTableauAssetFactory::ShouldShowInNewMenu() const
{
	return true;
}

FString UTableauAssetFactory::GetDefaultNewAssetName() const
{
	// Preliminary name comes from the last selected actor.
	return ConstructNameFromActor(GEditor->GetSelectedActors()->GetBottom<AActor>());
}

FString UTableauAssetFactory::ConstructNameFromActor(AActor* InActor) const
{
	const FString TabName = TEXT("TAB_{Name}");
	FString Name = TEXT("Base");
	if (InActor)
	{
		Name = InActor->GetActorLabel();
	}
	return TabName.Replace(TEXT("{Name}"), *Name);
}

