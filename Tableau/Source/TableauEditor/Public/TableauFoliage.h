#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Editor.h"

// Local Includes
#include "TableauEditorModule.h"
#include "TableauActor.h"
#include "TableauHISMComponent.h"

// Forward Declares
class UFoliageType;

class TABLEAUEDITOR_API FTableauFoliage
{
public:
	FTableauFoliage(UFoliageType* InFoliageType, const FTransform& Xform, const FName& InName, bool bIsSnappable);
	~FTableauFoliage();

	// Add an instance of this type.
	void AddInstance(const FTransform& Transform);

	// Configure a HISM Component using the composed Foliage Type and attach it to the Actor.
	void ConfigureAndAttachHISM(ATableauActor* Actor);

private:
	void UpdateComponentSettings(UTableauHISMComponent* HISMComponent);
	void RefreshComponentProperties(UTableauHISMComponent* Component);

private:
	TWeakObjectPtr<UFoliageType> FoliageType;

	TArray<FTransform> Instances;

	FName Name;

	bool bSnappable;
	bool bAlignToSurfaceNormal;

};

class FTableauFoliageManager
{
public:
	FTableauFoliageManager(ATableauActor* InActor);

	// Destroy Foliage Components attached to the actor.
	void DestroyFoliageComponents();

	// Trace all Foliage instances to floor
	void SnapToFloor(const UWorld* InWorld);

	// How do we extract foliage? I don't know yet.
	void ExtractFoliage(UWorld* InWorld);

private:
	void SnapInstancesToFloor(UTableauHISMComponent* Component, const UWorld* InWorld);

private:
	TWeakObjectPtr<ATableauActor> TableauActor;
};