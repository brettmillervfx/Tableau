#pragma once


/*
* Tableau
*
* Tableau is a system that implements a "prefab" workflow: ie. multiple assets may be arranged and grouped
* together in such a way that they may be treated as a single asset, to speed up layout work and remove arbitrary 
* decisions from the process. Tableau goes a little further and describes the collections as either Composition
* or Superposition. (There are other modes but they categorically generalize to Composition or Superposition.)
*
* In Composition mode, the Tableau works as a traditional prefab: each element listed in the Tableau asset is spawned
* relative to the Tableau object. In Superposition mode, a single element is selected randomly from the element list and 
* spawned relative to the Tableau. A Tableau element may refer to another Tableau. By doing so, we can express deep complex
* latent hierarchies of assets that only "collapse" into a finite list once the Tableau is instantiated, yeilding 
* considerable variation without a lot of work on the user's part.
*
* Tableau utilizes Unreal Editor's clipboard functionality to capture the state of a instatiated actor for use
* as an element -- it does not receive raw assets except in the case of a nested Tableau. This way, any vertex painting,
* added components, material reassignments, property settings, etc. are captured.
*
* Assembling a Tableau is straighforward: assets are spawned into a map, arranged and configured. The actors are selected and
* a new Tableau is made from the selection. The process is imlemented in the UTableauAssetFactory class. Once created, the 
* mode may be set in the asset properties.
*
* When a Tableau asset is instantiated into the map, the UTableauActorFactory spawns the Tableau actor with a UTableauComponent
* attached. The actor factory creates an FTableauActorManager that sets the seed on the component and clears it's current state. 
* It calls on FTableauLatentTree in TableauUtils to expand the Tableau into a hierarchical Recipe -- a tree of Recipe nodes
* that describe the actors that will comprise the manifested Tableau. Using this Recipe, the FTableauActorManager spawns the 
* specified actors using the clipboard data stored in the Recipe. These new actors and parented to the instantiated Tableau
* actor the weak pointers to them are registed with the Tableau Component. Generally, if anything needs to happen to the 
* Tableau Actor -- eg. reseeding, regeneration, deletion, snapping -- the process is handled by the FTableauActorManager, with
* low level process delegated to various TableauUtils. The Tableau Actor and Component contain as little logic as possible
* and the factory classes delegate most of the functionality to the Actor Manager.
*/



// Engine Includes
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameFramework/Actor.h"
#include "AssetTypeCategories.h"
#include "AssetTypeActions_Base.h"

// Local Includes
#include "TableauAsset.h"

// Forward Declarations
class ITableauEditor;

extern const FName TableauEditorAppIdentifier;

DECLARE_LOG_CATEGORY_EXTERN(LogTableau, Log, All);


class ITableauEditorModule : public IModuleInterface, public IHasMenuExtensibility
{
public:
	// Creates a new custom asset editor.
	virtual TSharedRef<ITableauEditor> CreateTableauEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UTableauAsset* TableauAsset) = 0;
};

class FTableauEditorModule : public ITableauEditorModule
{
public:

	//~ Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface interface

	//~ Begin IHasMenuExtensibility interface
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() override;
	//~ End IHasMenuExtensibility interface

	// Creates a new tableau editor.
	TSharedRef<ITableauEditor> CreateTableauEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UTableauAsset* TableauAsset);



protected:

	void RegisterActorFactories();

	void RegisterEditorDelegates();
	void UnregisterEditorDelegates();

	void RegisterMenuExtensions();
	void UnregisterMenuExtensions();

	void RegisterCommands();

	void RegisterComponentVisualizer();
	void UnregisterComponentVisualizer();

	// delegate functions
	void OnLevelSelectionChanged(UObject* Obj);
	void OnLevelActorAdded(AActor* Actor);
	void OnTableauActorDuplicate(AActor* Actor);

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtender> ViewportMenuExtender;
	TSharedPtr<FUICommandList> ViewportUICommandList;
	FDelegateHandle LevelViewportContextMenuTableauExtenderDelegateHandle;
	TSharedPtr<FUICommandList> TableauCommands;

	
};
