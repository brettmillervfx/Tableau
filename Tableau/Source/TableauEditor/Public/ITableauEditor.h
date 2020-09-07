#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"

// Forward Declarations
class UTableauAsset;

/**
 * Public interface to Tableau Asset Editor
 */
class ITableauEditor : public FAssetEditorToolkit
{
public:
	// Retrieves the current tableau asset.
	virtual UTableauAsset* GetTableau() = 0;

	// Set the current tableau asset. 
	virtual void SetTableau(UTableauAsset* InTableauAsset) = 0;
};
