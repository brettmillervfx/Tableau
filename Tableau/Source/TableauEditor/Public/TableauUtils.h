#pragma once

// Editor Includes
#include "CoreMinimal.h"
#include "Editor.h"

// Local Includes
#include "TableauActor.h"
#include "TableauAsset.h"
#include "TableauEditorModule.h"
#include "TableauLatentTree.h"


/*
* Helper classes for Tableau in the Unreal Editor
*/


/*
* Static utility functions 
* Miscellaneous low level Tableau functions.
*/
struct TABLEAUEDITOR_API FTableauUtils
{
	// Assuming that InActor is a Tableau Element, return a pointer to it's parent (ie. container)
	// Tableau actor. Rteurn NULL if there isn't one.
	static ATableauActor* GetFirstTableauParentActor(AActor* InActor);

	// Spawn Actors into the current map using Recipe instructions.
	static TArray<TWeakObjectPtr<AActor>> SpawnInstances(ATableauActor* OwningActor, const FTransform& TableauSpace, const TArray<FTableauRecipeNode>& Recipe, UTableauComponent* TableauComponent, FTableauInstanceTracker* CurrTracker, bool bIsPreview = false);
	
	// Spawn a single actor using captured clipboard data.
	static AActor* PasteActor(UWorld* InWorld, const FString& Config, const FTransform& Xform, const FName& Name);

	// Build a Scene Component from a Recipe node (with idenity transform).
	// Component created with this method is transient, ie. will not be saved.
	static USceneComponent* BuildComponent(UObject* TargetAsset);
	
	// Spawn a single component attached to the actor
	static void SpawnComponent(ATableauActor* OwningActor, UObject* TargetAsset, const FTransform& Xform, const FName& Name);
	
	// Spawn a single actor using the appropriate factory.
	static AActor* SpawnActor(UWorld* World, UObject* TargetAsset, const FTransform& Xform, const FName& Name);

	// Spawn a single Tableau actor from the provided asset.
	static AActor* SpawnTableauActor(UWorld* World, UTableauAsset* TargetTableauAsset, const FTransform& Xform, const FName& Name, const int32 Seed=0);

	// Replace a set of scene actors with a Tableau.
	// Returns a pointer to the spawned Tableau actor.
	static AActor* ReplaceActorsWithTableau(UTableauAsset* TableauAsset, const TArray<AActor*>& SelectedActors);

	// Fill Tablea Asset's element list using selected actors.
	static void FillTableauAssetElements(UTableauAsset* InAsset, const TArray<AActor*>& SelectedActors);

	static void StoreActorAsString(UWorld* InWorld, AActor* InActor, FString* DestinationData);

	// Modify transform to reflect random rotation and scaling around origin.
	static void JitterTransform(uint32 Seed, float MinScale, float MaxScale, bool bSpinZAxis, FTransform& Transform);

	// Snap the provided transform to the world, optionally aligning it to the normal at point of intersection.
	// We specifiy the allowable range of slope for a valid snap to occur (expressed in degrees from horizontal).
	// Returns true if a valid snap has occurred.
	static bool TraceToWorld(FTransform& Xform, const UWorld* InWorld, const FFloatInterval& GroundSlopeAngle, AActor* IgnoredActor, bool bAlignToNormal);

	static int32 NextSeed(int32 Seed);

};


/*
* Rewrite of native Unreal clipboard paste function. Features superfluous to Tableau have been removed
* and pasted actors are exposed to client.
*/
struct TABLEAUEDITOR_API FClipboard
{
public:

	// This is a stripped down version of the implentation in the Engine source.
	// In addition to removing features that aren't needed for this application,
	// we also return the list of pasted actors, which is necessary for post paste work.
	static void edactPasteSelected(UWorld* InWorld, const FString* SourceData, TArray<AActor*>& PastedActors);

};

