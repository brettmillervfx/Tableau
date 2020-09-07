#pragma once

// Engine Includse
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// Generated Include
#include "TableauBPLibrary.generated.h"

// Forward Declares
class ATableauActor;


UCLASS()
class TABLEAUEDITOR_API UTableauBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tableau Function Library")
	static void RegenerateTableauActor(ATableauActor* TableauActor);

	UFUNCTION(BlueprintCallable, Category = "Tableau Function Library")
	static void ReseedTableauActor(ATableauActor* TableauActor);

	UFUNCTION(BlueprintCallable, Category = "Tableau Function Library")
	static void SnapTableauActor(ATableauActor* TableauActor);

	UFUNCTION(BlueprintCallable, Category = "Tableau Function Library")
	static void UnpackTableauActor(ATableauActor* TableauActor);
	
};
