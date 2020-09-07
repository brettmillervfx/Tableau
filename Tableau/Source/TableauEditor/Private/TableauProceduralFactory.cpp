
#include "TableauProceduralFactory.h"

// Engine Includes
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "EditorFramework/AssetImportData.h"

// Local Includes
#include "TableauEditorModule.h"

// Local constants
const static int32 CartesianCoordinateComponentCount = 3;
const static int32 QuaternionComponentCount = 4;

UTableauProceduralFactory::UTableauProceduralFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UTableauAsset::StaticClass();
	bEditorImport = true;
	bCreateNew = false;
	bEditAfterNew = false;
	bText = false;

	Formats.Add(TEXT("tab;Tableau config file"));
}

void UTableauProceduralFactory::GetSupportedFileExtensions(TArray< FString >& OutExtensions) const
{
	OutExtensions.Add(FString(TEXT("tab")));
}

UObject* UTableauProceduralFactory::FactoryCreateFile
	(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		const FString& Filename,
		const TCHAR* Parms,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled
	)
{
	CurrentFilename = Filename;

	FString FileContents;
	FFileHelper::LoadFileToString(FileContents, *CurrentFilename);

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		UTableauAsset* NewTableauAsset = NewObject<UTableauAsset>(InParent, InClass, InName, Flags);

		// Set Source for reimport
		NewTableauAsset->SourceFilePath = UAssetImportData::SanitizeImportFilename(CurrentFilename, NewTableauAsset->GetOutermost());

		PopulateTableauAssetFromSource(NewTableauAsset, JsonObject);
		
		return NewTableauAsset;	
	}
	else
	{
		UE_LOG(LogTableau, Error, TEXT("Unable to import %s: unexpected format."), *CurrentFilename);
		return nullptr;
	}	
}

void UTableauProceduralFactory::PopulateTableauAssetFromSource(UTableauAsset* TableauAsset, const TSharedPtr<FJsonObject>& JsonObject) const
{
	TableauAsset->EvaluationMode = ExtractEvaluationModeFromJsonObject(JsonObject);

	if (JsonObjectHasField(JsonObject, TEXT("Elements")))
	{
		const TArray<TSharedPtr<FJsonValue>> JsonElements = JsonObject->GetArrayField(TEXT("Elements"));
		for (const TSharedPtr<FJsonValue>& JsonElement : JsonElements)
		{
			FTableauAssetElement ExtractedElement;
			if (ExtractTableauElementFromJsonObject(JsonElement->AsObject(), ExtractedElement))
			{
				TableauAsset->AddElement(ExtractedElement);
			}
		}
	}
}

bool UTableauProceduralFactory::JsonObjectHasField(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName) const
{
	if (JsonObject->HasField(FieldName))
	{
		return true;
	}

	UE_LOG(LogTableau, Warning, TEXT("Unexpected format in %s: no %s field found."), *CurrentFilename, *FieldName);
	return false;
}

ETableauEvaluationMode::Type UTableauProceduralFactory::ExtractEvaluationModeFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject) const
{
	// If no valid Evaluation Mode is found, we default to Composition.
	if (JsonObjectHasField(JsonObject, TEXT("EvaluationMode")))
	{
		FString Mode(JsonObject->GetStringField(TEXT("EvaluationMode")));
		if (Mode == TEXT("Composition"))
		{
			return ETableauEvaluationMode::Type::Composition;
		}
		else if (Mode == TEXT("Superposition"))
		{
			return ETableauEvaluationMode::Type::Superposition;
		}
		else if (Mode == TEXT("HierarchicalComposition"))
		{
			return ETableauEvaluationMode::Type::HierarchicalComposition;
		}
		else
		{
			UE_LOG(LogTableau, Warning, TEXT("Invalid EvaluationMode found in %s: defaulting to Composition."), *CurrentFilename);
			return ETableauEvaluationMode::Type::Composition;
		}
	}
	return ETableauEvaluationMode::Type::Composition;
}

bool UTableauProceduralFactory::ExtractTableauElementFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTableauAssetElement& ExtractedElement) const
{
	FName Name;
	if (!ExtractNameFromJsonObject(JsonObject, Name))
	{
		return false;
	}

	FSoftObjectPath AssetReference;
	if (!ExtractAssetReferenceFromJsonObject(JsonObject, AssetReference))
	{
		return false;
	}

	FTransform LocalTransform;
	if (!ExtractLocalTransformFromJsonObject(JsonObject, LocalTransform))
	{
		return false;
	}

	FTableauAssetElement TableauAssetElement(Name, AssetReference, TEXT(""), LocalTransform, 0);
	TableauAssetElement.bUseConfig = false;

	ExtractedElement = TableauAssetElement;

	return true;
}

bool UTableauProceduralFactory::ExtractNameFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FName& ExtractedName) const
{
	if (JsonObjectHasField(JsonObject, TEXT("Name")))
	{
		ExtractedName = *(JsonObject->GetStringField(TEXT("Name")));
		return true;
	}
	return false;
}

bool UTableauProceduralFactory::ExtractAssetReferenceFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FSoftObjectPath& AssetReference) const
{
	if (JsonObjectHasField(JsonObject, TEXT("AssetReference")))
	{
		AssetReference = FSoftObjectPath(JsonObject->GetStringField(TEXT("AssetReference")));
		if (AssetReference.IsValid())
		{
			return true;
		}
		else
		{
			UE_LOG(LogTableau, Warning, TEXT("Element skipped in %s: invalid asset %s."), *CurrentFilename, *AssetReference.ToString());
		}
	}
	return false;
}

bool UTableauProceduralFactory::ExtractLocalTransformFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTransform& LocalTransform) const
{
	FVector Translate;
	if (!ExtractTranslateFromJsonObject(JsonObject, Translate))
	{
		return false;
	}	
	LocalTransform.SetTranslation(Translate);

	FVector Scale;
	if (!ExtractScaleFromJsonObject(JsonObject, Scale))
	{
		return false;
	}
	LocalTransform.SetScale3D(Scale);

	FQuat Orient;
	if (!ExtractOrientFromJsonObject(JsonObject, Orient))
	{
		return false;
	}
	LocalTransform.SetRotation(Orient);

	return true;
}

bool UTableauProceduralFactory::ExtractTranslateFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FVector& Translate) const
{
	if (JsonObjectHasField(JsonObject, TEXT("Translate")))
	{
		TArray<TSharedPtr<FJsonValue>> TranslateJsonComponents = JsonObject->GetArrayField(TEXT("Translate"));

		if (TranslateJsonComponents.Num() != CartesianCoordinateComponentCount)
		{
			UE_LOG(LogTableau, Warning, TEXT("Unexpected format in %s: poorly formed Translate found."), *CurrentFilename);
			return false;
		}

		for (int32 i = 0; i < CartesianCoordinateComponentCount; ++i)
		{
			Translate[i] = TranslateJsonComponents[i]->AsNumber();
		}

		return true;
	}
	return false;
}

bool UTableauProceduralFactory::ExtractScaleFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FVector& Scale) const
{
	if (JsonObjectHasField(JsonObject, TEXT("Scale")))
	{
		Scale = FVector(JsonObject->GetNumberField(TEXT("Scale")));	
		return true;
	}
	return false;
}

bool UTableauProceduralFactory::ExtractOrientFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FQuat& Orient) const
{
	if (JsonObjectHasField(JsonObject, TEXT("Orient")))
	{
		TArray<TSharedPtr<FJsonValue>> OrientJsonComponents = JsonObject->GetArrayField(TEXT("Orient"));

		if (OrientJsonComponents.Num() != QuaternionComponentCount)
		{
			UE_LOG(LogTableau, Warning, TEXT("Unexpected format in %s: poorly formed Orient found."), *CurrentFilename);
			return false;
		}
		
		Orient = FQuat(OrientJsonComponents[0]->AsNumber(),
			OrientJsonComponents[1]->AsNumber(),
			OrientJsonComponents[2]->AsNumber(),
			OrientJsonComponents[3]->AsNumber());

		return true;
	}
	return false;
}



bool UTableauReimportProceduralFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(Obj))
	{
		if (TableauAsset->SourceFilePath.IsEmpty())
		{
			return false;
		}
		OutFilenames.Add(UAssetImportData::ResolveImportFilename(TableauAsset->SourceFilePath, TableauAsset->GetOutermost()));
		return true;
	}
	return false;
}

void UTableauReimportProceduralFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(Obj))
	{
		if (NewReimportPaths.Num() == 1)
		{
			TableauAsset->SourceFilePath = UAssetImportData::SanitizeImportFilename(NewReimportPaths[0], TableauAsset->GetOutermost());
		}
	}
}

EReimportResult::Type UTableauReimportProceduralFactory::Reimport(UObject* Obj)
{
	if (UTableauAsset* TableauAsset = Cast<UTableauAsset>(Obj))
	{
		const FString Filename = UAssetImportData::ResolveImportFilename(TableauAsset->SourceFilePath, TableauAsset->GetOutermost());
		if (!FPaths::GetExtension(Filename).Equals(TEXT("tab")))
		{
			return EReimportResult::Failed;
		}

		// Clear the current Elements for the asset
		TableauAsset->ClearElements();

		// Rebuild the asset from the source file
		CurrentFilename = Filename;

		FString FileContents;
		FFileHelper::LoadFileToString(FileContents, *CurrentFilename);

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(FileContents);
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			// Set Source for reimport
			TableauAsset->SourceFilePath = UAssetImportData::SanitizeImportFilename(CurrentFilename, TableauAsset->GetOutermost());

			PopulateTableauAssetFromSource(TableauAsset, JsonObject);

			return EReimportResult::Succeeded;
		}
		
		// There were problems on import
		return EReimportResult::Failed;
		
	}
	else
	{
		return EReimportResult::Failed;
	}

}
