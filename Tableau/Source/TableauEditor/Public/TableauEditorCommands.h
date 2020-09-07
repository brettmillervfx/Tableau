#pragma once

// Engine Includes
#include "Framework/Commands/Commands.h"

// Local Includes
#include "TableauEditorStyle.h"


/*
Edit Commands for Tableau Actors
*/

class FTableauEditorCommands : public TCommands<FTableauEditorCommands>
{

public:
	FTableauEditorCommands();

	virtual void RegisterCommands() override;

public:

	TSharedPtr<FUICommandInfo> ReseedSelectedTableauActors;
	TSharedPtr<FUICommandInfo> SnapTableauActorsToFloor;
	TSharedPtr<FUICommandInfo> RegenerateSelectedTableauActors;
	TSharedPtr<FUICommandInfo> UnpackSelectedTableauActors;
	TSharedPtr<FUICommandInfo> UnpackOneLayer;
	TSharedPtr<FUICommandInfo> UnpackForEditing;
	TSharedPtr<FUICommandInfo> FilterBySelectedActors;

};

