
#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Editor.h"
#include "Engine/Selection.h"

// Local Includes
#include "TableauActor.h"

/*
* Implementations of context menu commands.
*/
struct FTableauEditorActions
{
public:

	// Change the actor's seed to a random value and regenerate the Instances.
	static void ReseedSelectedTableauActors();

	// Snap actors vertically to local geometry, respecting snap flags and hierarchical composition.
	static void SnapTableauActorsToFloor();

	// Regenerate the Instances using the same seed. Useful for refreshing if "upstream" changes 
	// we made to Tableaux that contribute. Not that snapping will be reset.
	static void RegenerateSelectedTableauActors();

	// Remove the Tableau Actor from the scene and place all Instances in the scene as ordinary
	// actors.
	static void UnpackSelectedTableauActors();

	// Evaluate the root Tableau and place the resulting actors in the scene, as individual
	// actors or as dynamically expressed Tableaux as the case may be.
	static void UnpackOneLayer();

	// Replace the root Tableau actor with the full set of variants available (if the Tableau is a Superposition).
	// If the actor is not a Superposition, nothing is done.
	static void UnpackForEditing();

	// Attach all non-Tableau selected actors to Tableau as an Actor Filter.
	static void FilterBySelectedActors();

private:
	// Apply function to each selected Tableau Actor.
	static void ApplyToAllSelectedTableau(void (*ApplyFunc)(ATableauActor*))
	{
		TArray<AActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<AActor>(SelectedActors);

		for (AActor* Actor : SelectedActors)
		{
			if (ATableauActor *TableauActor = Cast<ATableauActor>(Actor))
			{
				ApplyFunc(TableauActor);
			}
		}

		// Reselect actors
		for (AActor* Actor : SelectedActors)
		{
			GEditor->SelectActor(Actor, true, true, true, true);
		}
	
	}

};
