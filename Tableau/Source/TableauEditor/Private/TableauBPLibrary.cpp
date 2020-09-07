
// This Include
#include "TableauBPLibrary.h"

// Local Includes
#include "TableauActor.h"
#include "TableauActorManager.h"


void UTableauBPLibrary::RegenerateTableauActor(ATableauActor* TableauActor)
{
	FTableauActorManager Manager(TableauActor);
	Manager.UpdateInstances(false);
}

void UTableauBPLibrary::ReseedTableauActor(ATableauActor* TableauActor)
{
	FTableauActorManager Manager(TableauActor);
	Manager.Reseed();
}

void UTableauBPLibrary::SnapTableauActor(ATableauActor* TableauActor)
{
	FTableauActorManager Manager(TableauActor);
	Manager.SnapToFloor();
}

void UTableauBPLibrary::UnpackTableauActor(ATableauActor* TableauActor)
{
	FTableauActorManager Manager(TableauActor);
	Manager.Unpack();
}