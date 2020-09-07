#pragma once

// Editor Includes
#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/PropertyEditor/Public/PropertyEditorDelegates.h"

// Local Includes

// Forward Declarations
class IDetailsView;
class SDockableTab;
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


class TABLEAUEDITOR_API FTableauEditor : public ITableauEditor, public FNotifyHook
{
public:

	virtual ~FTableauEditor();
	
	// Edits the specified asset object
	void InitTableauEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UTableauAsset* ObjectToEdit);

	// Initializes the editor to use a tableau. Should be the first thing called. 
	void InitEditorForTableau(UTableauAsset* ObjectToEdit);

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	// Begin IToolkit interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual bool IsPrimaryEditor() const override { return true; }
	// End IToolkit interface

	// Begin ITableauEditor initerface
	virtual UTableauAsset* GetTableau();
	virtual void SetTableau(UTableauAsset* InTableau);
	// End ITableauEditor initerface

private:
	// Spawn tabs and their contents
	TSharedRef<SDockTab> SpawnPropertiesTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnViewportTab(const FSpawnTabArgs& Args);

	//Sets the editor's current tableau and refreshes various settings to correspond with the new data.
	void SetEditorTableau(UTableauAsset* InTableau, bool bResetCamera = true);

public:
	static const FName ToolkitFName;

private:

	TSharedPtr< SDockableTab > PropertiesTab;
	TSharedPtr<class IDetailsView> DetailsView;
	TSharedPtr<class STableauEditorViewport> Viewport;

	// The tab ids for all the tabs used
	static const FName PropertiesTabId;
	static const FName ViewportTabId;

	// The Tableau Asset open within this editor
	UTableauAsset* Tableau;
};
