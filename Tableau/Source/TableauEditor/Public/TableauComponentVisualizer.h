
#pragma once

#include "ComponentVisualizer.h"


/**
Draw the Tableau component (simply) 
 */

class FTableauComponentVisualizer : public FComponentVisualizer
{

public:
	//~ Begin FComponentVisualizer interface
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	//~ End FComponentVisualizer interface
};
