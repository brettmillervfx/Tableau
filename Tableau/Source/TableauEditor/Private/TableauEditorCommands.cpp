
#include "TableauEditorCommands.h"

// Local Includes
#include "TableauEditorActions.h"


#define LOCTEXT_NAMESPACE "TableauEditorCommands"

FTableauEditorCommands::FTableauEditorCommands() 
	: TCommands<FTableauEditorCommands>
		(TEXT("TableauEditor"),
		LOCTEXT("TableauEditor", "Tableau Editor"),
		NAME_None,
		FTableauEditorStyle::Get().GetStyleSetName())
{
}

void FTableauEditorCommands::RegisterCommands()
{
	
	FString CommandsLinkRoot(TEXT("Shared/Commands"));

	UI_COMMAND(ReseedSelectedTableauActors, "ReseedSelectedTableauActors", "Reseed Selected Tableau Actors", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift | EModifierKey::Control,EKeys::R));
	UI_COMMAND(SnapTableauActorsToFloor, "SnapTableauActorsToFloor", "Snap Tableau Actors To Floor", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::End));
	UI_COMMAND(RegenerateSelectedTableauActors, "RegenerateSelectedTableauActors", "Regenerate Selected Tableau Actors", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UnpackSelectedTableauActors, "UnpackSelectedTableauActors", "Unpack Selected Tableau Actors", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UnpackOneLayer, "UnpackOneLayer", "Unpack One Layer", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(UnpackForEditing, "UnpackForEditing", "Unpack For Editing", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(FilterBySelectedActors, "FilterBySelectedActors", "Filter By Selected Actors", EUserInterfaceActionType::Button, FInputChord());

}


#undef LOCTEXT_NAMESPACE

