#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Serialization/JsonSerializer.h"
#include "EditorReimportHandler.h"

// Local Includes
#include "TableauAsset.h"

// Generated Include
#include "TableauProceduralFactory.generated.h"

/**
 *  Implements a factory for importing Tableau from a source "TAB" file.
 *  The TAB file (extension is '.tab') is a JSON format ascii database of Tableau configuration
 *  presumably built procedurally. It leaves many of the Tableau properties at default and sets
 *  those properties required for large Tableau populations.
 *
 *  The TAB file should be formatted so:
 
{
	"EvaluationMode" : "Composition",
	"Elements" :[
		{
			"Name" : "foo",
			"AssetReference" : "/Game/Base/Environments/Kits/GenericFoliage_Kit/Meshes/SM_GenericFoliage_BlackFern_01a.SM_GenericFoliage_BlackFern_01a",
			"Scale": 1.2800674438476562, 
			"Translate": [200, 100, 300],
			"Orient": [
                -0.07732092589139938, 
                0.03553477302193642, 
                0.9419571161270142, 
                0.3247699737548828
            ],
			"Name" : "bar",
			"AssetReference" : "/Game/Base/Environments/Kits/GenericFoliage_Kit/Meshes/SM_GenericFoliage_BlackFern_01a.SM_GenericFoliage_BlackFern_01b",
			"Scale": 1.2800674438476562,
			"Translate": [200, 100, 300],
			"Orient": [
				-0.07732092589139938,
				0.03553477302193642,
				0.9419571161270142,
				0.3247699737548828
			]
		}
	]	
}

*	containing as many elements as required. Valid EvaluationModes are "Composition", "Superposition", or "HierarchicalComposition". AssetReference should
*   provide a path to an existing content browser asset. Orient is a quaternion.
**/


UCLASS()
class TABLEAUEDITOR_API UTableauProceduralFactory : public UFactory
{
	GENERATED_BODY()
	
public:

	UTableauProceduralFactory(const FObjectInitializer& ObjectInitializer);

	//~ Begin UFactory interface
	virtual void GetSupportedFileExtensions(TArray< FString >& OutExtensions) const override;
	virtual UObject* FactoryCreateFile
	(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		const FString& Filename,
		const TCHAR* Parms,
		FFeedbackContext* Warn,
		bool& bOutOperationCanceled
	) override;
	//~ End UFactory interface

	void PopulateTableauAssetFromSource(UTableauAsset* TableauAsset, const TSharedPtr<FJsonObject>& JsonObject) const;

private:
	ETableauEvaluationMode::Type ExtractEvaluationModeFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject) const;
	bool ExtractTableauElementFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTableauAssetElement& ExtractedElement) const;
	bool JsonObjectHasField(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName) const;
	bool ExtractNameFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FName& ExtractedName) const;
	bool ExtractAssetReferenceFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FSoftObjectPath& AssetReference) const;
	bool ExtractLocalTransformFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FTransform& LocalTransform) const;
	bool ExtractTranslateFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FVector& Translate) const;
	bool ExtractScaleFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FVector& Scale) const;
	bool ExtractOrientFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject, FQuat& Orient) const;

protected:
	FString CurrentFilename;

};


UCLASS()
class TABLEAUEDITOR_API UTableauReimportProceduralFactory : public UTableauProceduralFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	//~ Begin FReimportHandler interface
	virtual bool CanReimport(UObject* Obj, TArray< FString >& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray< FString >& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ End FReimportHandler interface

};
