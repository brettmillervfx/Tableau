#include "TableauEditorActions.h"

// Local Includes
#include "TableauActorManager.h"

#define LOCTEXT_NAMESPACE "TableauEditorActions"

void FTableauEditorActions::ReseedSelectedTableauActors()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.Reseed();
		});
}

void FTableauEditorActions::SnapTableauActorsToFloor()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.SnapToFloor();
		});
}

void FTableauEditorActions::RegenerateSelectedTableauActors()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.UpdateInstances(false);
		});
}

void FTableauEditorActions::UnpackSelectedTableauActors()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.Unpack();
		});
}

void FTableauEditorActions::UnpackOneLayer()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.UnpackTopLayer();
		});
}

void FTableauEditorActions::UnpackForEditing()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.UnpackForEditing();
		});
}

void FTableauEditorActions::FilterBySelectedActors()
{
	ApplyToAllSelectedTableau([](ATableauActor* InActor)
		{
			FTableauActorManager Manager(InActor);
			Manager.FilterBySelectedActors();
		});
}


#undef LOCTEXT_NAMESPACE
