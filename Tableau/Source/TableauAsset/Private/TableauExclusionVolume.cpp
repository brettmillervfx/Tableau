
// Local Includes
#include "TableauExclusionVolume.h"

ATableauExclusionVolume::ATableauExclusionVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BrushColor = FColor::Emerald;
	bSolidWhenSelected = true;
	bIsEditorOnlyActor = true;
}