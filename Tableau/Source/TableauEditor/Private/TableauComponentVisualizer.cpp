
#include "TableauComponentVisualizer.h"

// Local includes
#include "TableauComponent.h"


void FTableauComponentVisualizer::DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	const UTableauComponent* TableauComponent = Cast<UTableauComponent>(Component);
	if (!TableauComponent)
	{
		return;
	}

	FLinearColor DrawColor = TableauComponent->GetColor();
	FBox BoundingBox = TableauComponent->GetBoundingBox();

	// Draw a colored box around the contained objects, color coded to the mode.
	DrawWireBox(PDI, BoundingBox, DrawColor, SDPG_Foreground, 2.5);
}
