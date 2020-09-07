
#include "TableauEditorModule.h"

// Engine Includes
#include "Editor.h"
#include "Engine/Selection.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "Engine/Selection.h"

// Local Includes
#include "TableauActorFactory.h"
#include "TableauActor.h"
#include "TableauComponent.h"
#include "TableauActorManager.h"
#include "TableauUtils.h"
#include "TableauEditorStyle.h"
#include "TableauEditorCommands.h"
#include "TableauEditorActions.h"
#include "TableauComponentVisualizer.h"
#include "TableauAssetTypeActions.h"
#include "TableauEditor.h"


const FName TableauEditorAppIdentifier = FName(TEXT("TableauEditorApp"));

DEFINE_LOG_CATEGORY(LogTableau);

#define LOCTEXT_NAMESPACE "FTableauEditorModule"


TSharedPtr<FExtensibilityManager> FTableauEditorModule::GetMenuExtensibilityManager()
{
	return MenuExtensibilityManager;
}

void FTableauEditorModule::StartupModule()
{
	FTableauEditorStyle::Initialize();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	{
		TSharedRef<IAssetTypeActions> ACT_UTableauAsset = MakeShareable(new FTableauAssetTypeActions);
		AssetTools.RegisterAssetTypeActions(ACT_UTableauAsset);
	}
	
	RegisterCommands();
	RegisterActorFactories();
	RegisterMenuExtensions();
	if (!IsRunningCommandlet())
	{
		RegisterEditorDelegates();
	}
	RegisterComponentVisualizer();
}

void FTableauEditorModule::ShutdownModule()
{
	if (!IsRunningCommandlet())
	{
		UnregisterEditorDelegates();
	}
	UnregisterMenuExtensions();
	UnregisterComponentVisualizer();

	FTableauEditorStyle::Shutdown();
}

TSharedRef<ITableauEditor> FTableauEditorModule::CreateTableauEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UTableauAsset* TableauAsset)
{
	// Initialize and spawn a new tableau asset editor with the provided parameters
	TSharedRef<FTableauEditor> NewTableauEditor(new FTableauEditor());
	NewTableauEditor->InitTableauEditor(Mode, InitToolkitHost, TableauAsset);
	return NewTableauEditor;
}



/*
Begin Menu Extensions
*/

struct FTableauMenuExtend
{
	static void ExtendMenu(class FMenuBuilder& InputMenuBuilder)
	{
		// These commands will only function for selected Tableau actors.
		TArray<ATableauActor*> SelectedActors;
		GEditor->GetSelectedActors()->GetSelectedObjects<ATableauActor>(SelectedActors);

		const bool bHasValidActorSelected = (SelectedActors.Num() > 0) && (SelectedActors.Last() != NULL);
		if (!bHasValidActorSelected)
		{
			return;
		}
		
		InputMenuBuilder.BeginSection("Tableau", LOCTEXT("Tableau", "Tableau"));

		// --------------------
		// Reseed selected Tableau Actors
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().ReseedSelectedTableauActors,
			NAME_None,
			LOCTEXT("ReseedSelectedTableauActors", "Reseed Selected Tableau Actors"),
			LOCTEXT("ReseedSelectedTableauActorsTooltip", "Change Tableau seed to random value and regenerate the resulting hierarchy."));

		// --------------------
		// Snap To Floor for Tableau Actors
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().SnapTableauActorsToFloor,
			NAME_None,
			LOCTEXT("SnapTableauActorsToFloor", "Snap Tableau Actors To Floor"),
			LOCTEXT("SnapTableauActorsToFloorTooltip", "Snap Tableau actors to floor using hierarchical configuration for parenting."));

		// --------------------
		// Regenerate selected Tableau Actors
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().RegenerateSelectedTableauActors,
			NAME_None,
			LOCTEXT("RegenerateSelectedTableauActors", "Regenerate Selected Tableau Actors"),
			LOCTEXT("RegenerateSelectedTableauActorsTooltip", "Regenerate the hierarchy to pick up any changes."));

		// --------------------
		// Unpack selected Tableau Actors
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().UnpackSelectedTableauActors,
			NAME_None,
			LOCTEXT("UnpackSelectedTableauActors", "Unpack Selected Tableau Actors"),
			LOCTEXT("UnpackSelectedTableauActorsTooltip", "Unpack Tableau completely and spawn resulting actors into level."));

		// --------------------
		// Unpack one layer
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().UnpackOneLayer,
			NAME_None,
			LOCTEXT("UnpackOneLayer", "Unpack One Layer"),
			LOCTEXT("UnpackOneLayerTooltip", "Strip down a single layer of the Tableau."));

		// --------------------
		// Unpack for editing
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().UnpackForEditing,
			NAME_None,
			LOCTEXT("UnpackForEditing", "Unpack For Editing"),
			LOCTEXT("UnpackForEditingTooltip", "Unpack each element of a Tableau, regardless of mode."));

		// --------------------
		// Filter by selected non-tableau actors
		// --------------------
		InputMenuBuilder.AddMenuEntry(
			FTableauEditorCommands::Get().FilterBySelectedActors,
			NAME_None,
			LOCTEXT("FilterBySelectedActors", "Filter By Selected Actors"),
			LOCTEXT("FilterBySelectedActorsTooltip", "Use selected non-Tableau actors as filters."));

		InputMenuBuilder.EndSection();
	}
};

TSharedRef<FExtender> ExtendLevelViewportContextMenuForTableau(const TSharedRef<FUICommandList> CommandList, TArray<AActor*> SelectedActors)
{
	TSharedRef<FExtender> Extender = MakeShareable(new FExtender);
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedRef<FUICommandList> LevelEditorCommandBindings = LevelEditorModule.GetGlobalLevelEditorActions();

	if (SelectedActors.Num() > 0 && SelectedActors.Last() != nullptr)
	{
		Extender->AddMenuExtension(
			"ActorControl", EExtensionHook::After, LevelEditorCommandBindings,
			FMenuExtensionDelegate::CreateStatic(&FTableauMenuExtend::ExtendMenu)
		);
	}

	return Extender;
}

FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors LevelViewportContextMenuTableauExtender;

void  FTableauEditorModule::RegisterActorFactories()
{
	UTableauActorFactory* TableauActorFactory = NewObject<UTableauActorFactory>();
	GEditor->ActorFactories.Add(TableauActorFactory);
}

void FTableauEditorModule::RegisterMenuExtensions()
{
	MenuExtensibilityManager = MakeShareable(new FExtensibilityManager);
	
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& ContextMenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();
	
	LevelViewportContextMenuTableauExtender = FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&ExtendLevelViewportContextMenuForTableau);
	ContextMenuExtenders.Add(LevelViewportContextMenuTableauExtender);
	LevelViewportContextMenuTableauExtenderDelegateHandle = ContextMenuExtenders.Last().GetHandle();
}

void FTableauEditorModule::UnregisterMenuExtensions()
{
	MenuExtensibilityManager.Reset();

	if (LevelViewportContextMenuTableauExtenderDelegateHandle.IsValid())
	{
		if (FLevelEditorModule* LevelEditorModule = FModuleManager::Get().GetModulePtr<FLevelEditorModule>("LevelEditor"))
		{
			// Remove the Menu Extender we previously added.
			LevelEditorModule->GetAllLevelViewportContextMenuExtenders().RemoveAll([=](const FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors& In)
				{
					return In.GetHandle() == LevelViewportContextMenuTableauExtenderDelegateHandle;
				});

			LevelEditorModule->GetMenuExtensibilityManager()->RemoveExtender(ViewportMenuExtender); 
			ViewportMenuExtender = nullptr;
		}
	}
}

/*
End Menu Extensions
*/

void FTableauEditorModule::RegisterCommands()
{
	
	FTableauEditorCommands::Register();
	TableauCommands = MakeShareable(new FUICommandList);

	const FTableauEditorCommands& Commands = FTableauEditorCommands::Get();
	TableauCommands->MapAction(Commands.ReseedSelectedTableauActors, FExecuteAction::CreateStatic(&FTableauEditorActions::ReseedSelectedTableauActors));
	TableauCommands->MapAction(Commands.SnapTableauActorsToFloor, FExecuteAction::CreateStatic(&FTableauEditorActions::SnapTableauActorsToFloor));
	TableauCommands->MapAction(Commands.RegenerateSelectedTableauActors, FExecuteAction::CreateStatic(&FTableauEditorActions::RegenerateSelectedTableauActors));
	TableauCommands->MapAction(Commands.UnpackSelectedTableauActors, FExecuteAction::CreateStatic(&FTableauEditorActions::UnpackSelectedTableauActors));
	TableauCommands->MapAction(Commands.UnpackOneLayer, FExecuteAction::CreateStatic(&FTableauEditorActions::UnpackOneLayer));
	TableauCommands->MapAction(Commands.UnpackForEditing, FExecuteAction::CreateStatic(&FTableauEditorActions::UnpackForEditing));
	TableauCommands->MapAction(Commands.FilterBySelectedActors, FExecuteAction::CreateStatic(&FTableauEditorActions::FilterBySelectedActors));

	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedRef<FUICommandList> CommandBindings = LevelEditor.GetGlobalLevelEditorActions();
	CommandBindings->Append(TableauCommands.ToSharedRef());
	
}

void FTableauEditorModule::RegisterComponentVisualizer()
{
	if (GUnrealEd)
	{
		GUnrealEd->RegisterComponentVisualizer(UTableauComponent::StaticClass()->GetFName(), MakeShareable(new FTableauComponentVisualizer()));
	}
}

void FTableauEditorModule::UnregisterComponentVisualizer()
{
	if (GUnrealEd)
	{
		GUnrealEd->UnregisterComponentVisualizer(UTableauComponent::StaticClass()->GetFName());
	}
}

void  FTableauEditorModule::RegisterEditorDelegates()
{
	USelection::SelectionChangedEvent.AddRaw(this, &FTableauEditorModule::OnLevelSelectionChanged);
	USelection::SelectObjectEvent.AddRaw(this, &FTableauEditorModule::OnLevelSelectionChanged);

	// Bind Actor duplicate delegate.
	UTableauComponent::OnTableauDuplicate.BindRaw(this, &FTableauEditorModule::OnTableauActorDuplicate);
}

void  FTableauEditorModule::UnregisterEditorDelegates()
{
	USelection::SelectionChangedEvent.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
}

void FTableauEditorModule::OnLevelSelectionChanged(UObject* Obj)
{

	struct Local
	{
		static void PreventTableauElementSelection(AActor* InActor)
		{
			if (!InActor->IsSelected())
			{
				return;
			}

			// If this actor is a Tableau, take the opportunity to validate its tracking.
			if (ATableauActor* TableauActor = Cast<ATableauActor>(InActor))
			{
				FTableauActorManager Manager(TableauActor);
				Manager.ValidateTracking();
			}

			// If this is a Tableau Element, shift the selection to its
			// Tableau actor parent.
			if (InActor->ActorHasTag(TableauActorConstants::TABLEAU_ELEMENT_TAG))
			{
				if (ATableauActor * FirstTableauParentActor = FTableauUtils::GetFirstTableauParentActor(InActor))
				{
					GEditor->SelectActor(InActor, false, false, true);
					GEditor->SelectActor(FirstTableauParentActor, true, false, true);

					// And collapse the TopTableau since it should be essentially off limits anyway.
					FTableauActorManager Manager(FirstTableauParentActor);
					Manager.ValidateTracking();
					Manager.CollapseActorInOutliner();
				}
			}
		}
	};

	// If a Tableau element is selected, select the Tableau instead.
	if (USelection * Selection = Cast<USelection>(Obj))
	{
		const int32 SelectionCount = Selection->Num();
		for (int32 Index = SelectionCount - 1; Index >= 0; --Index)
		{
			if (AActor * Actor = Cast<AActor>(Selection->GetSelectedObject(Index)))
			{
				Local::PreventTableauElementSelection(Actor);
			}
		}
	}
	else if (AActor * SelectedActor = Cast<AActor>(Obj))
	{
		Local::PreventTableauElementSelection(SelectedActor);
	}
}

void FTableauEditorModule::OnLevelActorAdded(AActor* Actor)
{
	if (ATableauActor * TableauActor = Cast<ATableauActor>(Actor))
	{
		FTableauActorManager Manager(TableauActor);
		Manager.ClearInstances();
		Manager.Reseed();
	}
}

void FTableauEditorModule::OnTableauActorDuplicate(AActor* Actor)
{
	// Store current selection
	TArray<TWeakObjectPtr<UObject> > PreviouslySelectedObjects;
	GEditor->GetSelectedActors()->GetSelectedObjects(PreviouslySelectedObjects);

	if (ATableauActor* TableauActor = Cast<ATableauActor>(Actor))
	{
		FTableauActorManager Manager(TableauActor);
		Manager.ValidateTracking();
		Manager.DeleteInstances();
		Manager.Reseed();
	}

	// Restore previous selection
	GEditor->SelectNone(false, true);
	for (const TWeakObjectPtr<UObject>& ObjectToSelect : PreviouslySelectedObjects)
	{
		if (AActor* PreviouslySelectedActor = Cast<AActor>(ObjectToSelect.Get()))
		{
			GEditor->SelectActor(PreviouslySelectedActor, true, true);
		}
		
	}
	
}



#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FTableauEditorModule, TableauEditor)