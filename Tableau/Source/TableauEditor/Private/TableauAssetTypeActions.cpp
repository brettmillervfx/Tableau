

#include "TableauAssetTypeActions.h"

// Engine Includes
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Engine/Selection.h"
#include "EditorReimportHandler.h"
#include "AssetData.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

// Local Includes
#include "TableauUtils.h"
#include "TableauAssetFactory.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


bool FTableauAssetTypeActions::CanFilter()
{
	return true;
}

uint32 FTableauAssetTypeActions::GetCategories()
{
	return EAssetTypeCategories::Basic | EAssetTypeCategories::Misc;
}

FText FTableauAssetTypeActions::GetName() const
{
	return LOCTEXT("Tableau", "Tableau");
}

UClass* FTableauAssetTypeActions::GetSupportedClass() const
{
	return UTableauAsset::StaticClass();
}

FColor FTableauAssetTypeActions::GetTypeColor() const
{
	return FColor::Blue;
}

bool FTableauAssetTypeActions::HasActions(const TArray<UObject*>& InObjects) const
{
	return true;
}

void FTableauAssetTypeActions::GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder)
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	bool ValidObjects = false;
	TArray<TWeakObjectPtr<UTableauAsset>> TableauAssets;
	if (InObjects.Num() > 0)
	{
		TableauAssets = GetTypedWeakObjectPtrs< UTableauAsset >(InObjects);
		ValidObjects = true;
	}

	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TableauAsset_ReimportTableau", "Reimport"),
			LOCTEXT("TableauAsset_ReimportTooltip", "Reimport the selected Tableau TAB file(s)."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ContentBrowser.AssetActions.ReimportAsset"),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTableauAssetTypeActions::ExecuteReimport, TableauAssets),
				FCanExecuteAction::CreateSP(this, &FTableauAssetTypeActions::CanExecuteReimport, TableauAssets)
				)
			);
		
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TableauAsset_UpdateTableau", "Update Tableau Asset using selected actors"),
			LOCTEXT("TableauAsset_UpdateTableauToolTip", "Update Tableau Asset using selected actors"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTableauAssetTypeActions::UpdateTableauAsset, TableauAssets),
				FCanExecuteAction::CreateLambda([=] {
						return ValidObjects;
					})
				)
			);
		
		MenuBuilder.AddMenuEntry(
			LOCTEXT("TableauAsset_MakeDeterministic", "Switch Tableau to Deterministic"),
			LOCTEXT("TableauAsset_MakeDeterministicToolTip", "Switch all Elements contained by Tableau to use Deterministic mode."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTableauAssetTypeActions::MakeDeterministic, TableauAssets),
				FCanExecuteAction::CreateLambda([=] {
					return ValidObjects;
					})
				)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("TableauAsset_InsertCompositionContainer", "Insert Composition Container"),
			LOCTEXT("TableauAsset_InsertCompositionContainerToolTip", "Wrap the current Tableau in a Composition container Tableau and update all referencers to reference the container."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &FTableauAssetTypeActions::InsertCompositionContainer, TableauAssets),
				FCanExecuteAction::CreateLambda([=] {
					return ValidObjects;
					})
			)
		);
			
	}
}

void FTableauAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Obj: InObjects)
	{
		if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(Obj))
		{
			ITableauEditorModule* TableauEditorModule = &FModuleManager::LoadModuleChecked<ITableauEditorModule>("TableauEditor");
			TableauEditorModule->CreateTableauEditor(Mode, EditWithinLevelEditor, TableauAsset);
		}
	}
}

void FTableauAssetTypeActions::UpdateTableauAsset(TArray<TWeakObjectPtr<UTableauAsset>> Objects)
{
	for (TWeakObjectPtr<UTableauAsset>& TableauAssetPtr : Objects)
	{
		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

		FTableauUtils::FillTableauAssetElements(TableauAssetPtr.Get(), SelectedActors);

		FTableauUtils::ReplaceActorsWithTableau(TableauAssetPtr.Get(), SelectedActors);
	}
}

void FTableauAssetTypeActions::MakeDeterministic(TArray<TWeakObjectPtr<UTableauAsset>> Objects)
{
	for (TWeakObjectPtr<UTableauAsset>& TableauAssetPtr : Objects)
	{
		for (FTableauAssetElement& Element : TableauAssetPtr->TableauElement)
		{
			Element.bDeterministic = true;
		}
	}
}

bool FTableauAssetTypeActions::CanExecuteReimport(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const
{
	for (const TWeakObjectPtr<UTableauAsset>& TableauAssetPtr : Objects)
	{
		// If one object is valid, we can execute the action
		if (!TableauAssetPtr->SourceFilePath.IsEmpty())
		{
			return true;
		}	
	}

	return false;
}

void FTableauAssetTypeActions::ExecuteReimport(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const
{
	for (const TWeakObjectPtr<UTableauAsset>& TableauAssetPtr : Objects)
	{
		if (!TableauAssetPtr->SourceFilePath.IsEmpty())
		{
			FReimportManager::Instance()->Reimport(TableauAssetPtr.Get(), true);
		}
	}
}

void FTableauAssetTypeActions::InsertCompositionContainer(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const
{
	// Track affected objects for later synchronization.
	TArray<UObject*> AffectedObjects;
	
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
	const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	// Create Wrapper Composition Tableau
	UTableauAssetFactory* TableauAssetFactory = NewObject<UTableauAssetFactory>(UTableauAssetFactory::StaticClass());


	for (const TWeakObjectPtr<UTableauAsset>& TableauAssetPtr : Objects)
	{
		const FSoftObjectPath PrimeAssetPath(TableauAssetPtr.Get());
		
		// Collect all referencing assets
		TArray<FName> Referencers;
		AssetRegistryModule.Get().GetReferencers(TableauAssetPtr->GetOutermost()->GetFName(), Referencers, EAssetRegistryDependencyType::Soft);
		
		FString Name, PackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(TableauAssetPtr->GetOutermost()->GetFName().ToString(), TEXT("Wrapper"), /*out*/ PackageName, /*out*/ Name);
		const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
		UTableauAsset* NewTableauAsset = Cast<UTableauAsset>(AssetToolsModule.Get().CreateAsset(Name, PackagePath, UTableauAsset::StaticClass(), TableauAssetFactory));

		// Check if a valid Tableau was created
		if (NewTableauAsset == nullptr)
		{
			UE_LOG(LogTableau, Error, TEXT("Unable to create new wrapper Tableau asset"));
			return;
		}

		NewTableauAsset->EvaluationMode = ETableauEvaluationMode::Composition;
		const FSoftObjectPath IntermediateAssetPath(NewTableauAsset);

		// Add this Tableau to the composition.
		FTableauAssetElement PrimeAssetWrapper;
		PrimeAssetWrapper.Name = TableauAssetPtr->GetFName();
		PrimeAssetWrapper.AssetReference = PrimeAssetPath;
		NewTableauAsset->AddElement(PrimeAssetWrapper);

		// Flag new asset for synch
		AffectedObjects.Add(NewTableauAsset);

		// Replace references to this Tableau with Wrapper Tableau
		for (const FName& Referencer : Referencers)
		{
			UE_LOG(LogTableau, Warning, TEXT("Updating soft reference in: %s"), *Referencer.ToString());
			
			TArray <FAssetData> AssetDatas;
			AssetRegistryModule.Get().GetAssetsByPackageName(Referencer, AssetDatas, false);

			for (const FAssetData& AssetData : AssetDatas)
			{
				if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(AssetData.GetAsset()))
				{					
					AffectedObjects.Add(TableauAsset);
					TableauAsset->ReplaceTableauReferences(PrimeAssetPath, IntermediateAssetPath);
					TableauAsset->GetOutermost()->SetDirtyFlag(true);
				}
			}	
		}
	}

	// Select the affected assets
	ContentBrowserModule.Get().SyncBrowserToAssets(AffectedObjects);
}

#undef LOCTEXT_NAMESPACE