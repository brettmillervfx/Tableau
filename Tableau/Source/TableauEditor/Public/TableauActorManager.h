#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Editor.h"

// Local Includes
#include "TableauActor.h"
#include "TableauUtils.h"
#include "TableauFilter.h"

/*
* FTableauActorManager contains most of the Tableau logic. It is responsible for evaluating Tableau and 
* spawning actors for it. It is responsible for maintaining current state in the Tableau Component.
* If any changes are requested to the state of the Tableau Actor (eg. reseeding, regeneration to pick up "upstream" 
* changes, snapping to lansdscape, etc.) the is the class that handles it.
* Various TableauUtils functions and classes are used in the process but the high level logic is contained in this class.
*/

class TABLEAUEDITOR_API FTableauActorManager
{
public:
	FTableauActorManager(ATableauActor* InTableauActor, UWorld* InTargetWorld=nullptr);
	~FTableauActorManager();

	// Evaluate the latent tree, spawn the actors required by the recipe, build the scene hierarchies,
	// and update the Component registry.
	void UpdateInstances(bool bIsPreview);

	// If a Tableau element actor is selected, deselect and select the parent Tableau Actor instead.
	void MoveSelectionToTableauParent();

	// If the World Outliner is viewable, collapse the Tableau Actor tree to keep the Outliner visually tidy.
	void CollapseActorInOutliner();

	// Destroy all actor instances owned by this Tableau actor.
	void DeleteInstances();

	// Change the Component's seed value and re-evaluate the latent tree.
	void Reseed();

	// Reset the Component's instances registry. This does not destroy the actors pointed to!
	void ClearInstances();

	// Convert the Tableau actor to regular actors in the map, then delete the Tableau actor.
	void Unpack();

	// Evaluate the root Tableau but instead of spawning a single Tableau actor, spawn the first strata of 
	// actors created by the Tableau. This may be deeper Tableau actors or thay may be conventional
	// actors. 
	void UnpackTopLayer();

	// Unpack all of the elements of the Tableau asset referenced by the actor, regardless of the 
	// assets mode. This is a useful command for editing Superpositions by unpacking the entire element list.
	void UnpackForEditing();

	// Snap the actors comprising the Tableau to the floor, abiding rules for Hierarchical Composition modes.
	void SnapToFloor();

	// Attach selected non-Tableau actors to the actor filter array.
	void FilterBySelectedActors();

	// Check that the Tableau is tracking its owned actors correctly and fix any errors.
	void ValidateTracking();

	// Assemble filters from Tableau Component configuration.
	TSharedPtr<FTableauFilterSampler> GatherFilters(const UTableauComponent* TableauComponent) const;

private:
	void SpawnInstances(TArray<FTableauRecipeNode> &Recipe, bool bIsPreview);
	void SpawnFoliage(TMap<UFoliageType*, TUniquePtr<FTableauFoliage>>& Foliages);
	AActor* SpawnElement(ULevel* Level, const FTableauAssetElement& Element, int32 Seed, const FTransform& Space);
	void SnapToFloor(const TArray<FTableauInstanceTracker>& Instances, const FVector& ParentTranslate);
	
	
private:
	UPROPERTY()
	ATableauActor* TableauActor;

	UPROPERTY()
	UWorld* TargetWorld;

	bool bAssetEditorWorkflow;

};
