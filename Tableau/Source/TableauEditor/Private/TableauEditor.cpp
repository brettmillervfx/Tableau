// This Include
#include "TableauEditor.h"

// Engine Includes
#include "Modules/ModuleManager.h"
#include "EditorStyleSet.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor.h"
#include "Toolkits/AssetEditorToolkit.h"

// Local Includes
#include "TableauEditorModule.h"
#include "STableauEditorViewport.h"

#define LOCTEXT_NAMESPACE "TableauEditor"

const FName FTableauEditor::ToolkitFName(TEXT("TableauEditor"));
const FName FTableauEditor::ViewportTabId(TEXT("TableauEditor_Viewport"));
const FName FTableauEditor::PropertiesTabId(TEXT("TableauEditor_Properties"));

void FTableauEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	// Add a new workspace menu category to the tab manager
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_TableauAssetEditor", "Tableau Asset Editor"));

	// We register the tab manager to the asset editor toolkit so we can use it in this editor
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
	
	InTabManager->RegisterTabSpawner(ViewportTabId, FOnSpawnTab::CreateSP(this, &FTableauEditor::SpawnViewportTab))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));
	
	InTabManager->RegisterTabSpawner(PropertiesTabId, FOnSpawnTab::CreateSP(this, &FTableauEditor::SpawnPropertiesTab))
		.SetDisplayName(LOCTEXT("PropertiesTab", "Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef())
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FTableauEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	// Unregister the tab manager from the asset editor toolkit
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);

	// Unregister our custom tab from the tab manager, making sure it is cleaned up when the editor gets destroyed
	InTabManager->UnregisterTabSpawner(ViewportTabId);
	InTabManager->UnregisterTabSpawner(PropertiesTabId);
}

TSharedRef<SDockTab> FTableauEditor::SpawnViewportTab(const FSpawnTabArgs& Args)
{
	check(Args.GetTabId() == ViewportTabId);
	
	TSharedRef<SDockTab> SpawnedTab =
		SNew(SDockTab)
		.Label(LOCTEXT("TableauViewport_TabTitle", "Viewport"))
		[
			Viewport.ToSharedRef()
		];

	Viewport->SetParentTab(SpawnedTab);

	return SpawnedTab;
	
}

TSharedRef<SDockTab> FTableauEditor::SpawnPropertiesTab(const FSpawnTabArgs& Args)
{
	// Make sure we have the correct tab id
	check(Args.GetTabId() == PropertiesTabId);

	// Return a new slate dockable tab that contains our details view
	return SNew(SDockTab)
		.Icon(FEditorStyle::GetBrush("GenericEditor.Tabs.Properties"))
		.Label(LOCTEXT("GenericDetailsTitle", "Details"))
		.TabColorScale(GetTabColorScale())
		[
			// Provide the details view as this tab its content
			DetailsView.ToSharedRef()
		];
}

FTableauEditor::~FTableauEditor()
{
	// On destruction we reset our tab and details view 
	Viewport.Reset();
	DetailsView.Reset();
	PropertiesTab.Reset();
}

FName FTableauEditor::GetToolkitFName() const
{
	return ToolkitFName;
}

FString FTableauEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Tableau ").ToString();
}

FText FTableauEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Tableau Asset Editor");
}

FLinearColor FTableauEditor::GetWorldCentricTabColorScale() const
{
	return FColor::Black;
}

UTableauAsset* FTableauEditor::GetTableau()
{
	return Tableau;
}

void FTableauEditor::SetTableau(UTableauAsset* InTableau)
{
	Tableau = InTableau;
}

void FTableauEditor::InitEditorForTableau(UTableauAsset* ObjectToEdit)
{
	
	Viewport = SNew(STableauEditorViewport)
		.TableauEditor(SharedThis(this))
		.ObjectToEdit(ObjectToEdit);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	SetEditorTableau(ObjectToEdit);
}

void FTableauEditor::InitTableauEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UTableauAsset* ObjectToEdit)
{
	if (Tableau != ObjectToEdit)
	{
		InitEditorForTableau(ObjectToEdit);
	}

	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_TableauEditor_Layout")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.9f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.6f)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
					->SetHideTabWell(true)
				)
			)
		);

	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	FAssetEditorToolkit::InitAssetEditor(Mode, InitToolkitHost, TableauEditorAppIdentifier, StandaloneDefaultLayout, bCreateDefaultToolbar, bCreateDefaultStandaloneMenu, ObjectToEdit);

}

void FTableauEditor::SetEditorTableau(UTableauAsset* InTableau, bool bResetCamera/*=true*/)
{

	Tableau = InTableau;

	// Set the details view.
	DetailsView->SetObject(Tableau);

	Viewport->UpdatePreviewTableau(Tableau, bResetCamera);
	Viewport->RefreshViewport();
}

#undef LOCTEXT_NAMESPACE