
// This Include
#include "STableauEditorViewport.h"

// Engine Includes
#include "Slate/SceneViewport.h"

// Local Includes
#include "TableauAsset.h"
#include "TableauEditorViewportClient.h"
#include "TableauLatentTree.h"
#include "TableauUtils.h"


#define LOCTEXT_NAMESPACE "TableauEditorViewport"


void STableauEditorViewport::Construct(const FArguments& InArgs)
{

	// restore last used feature level
	UWorld* World = PreviewScene->GetWorld();
	if (World != nullptr)
	{
		World->ChangeFeatureLevel(GWorld->FeatureLevel);
	}

	TableauEditorPtr = InArgs._TableauEditor;

	Tableau = InArgs._ObjectToEdit;

	CurrentViewMode = VMI_Lit;

	SEditorViewport::Construct(SEditorViewport::FArguments());

}

STableauEditorViewport::STableauEditorViewport()
	: PreviewScene(MakeShareable(new FAdvancedPreviewScene(FPreviewScene::ConstructionValues())))
	, Seed(0.0)
{

}

STableauEditorViewport::~STableauEditorViewport()
{
	if (EditorViewportClient.IsValid())
	{
		EditorViewportClient->Viewport = nullptr;
	}
}

TSharedRef<class SEditorViewport> STableauEditorViewport::GetViewportWidget()
{
	return SharedThis(this);
}

TSharedPtr<FExtender> STableauEditorViewport::GetExtenders() const
{
	TSharedPtr<FExtender> Result(MakeShareable(new FExtender));
	return Result;
}

void STableauEditorViewport::OnFloatingButtonClicked()
{

}

void STableauEditorViewport::RefreshViewport()
{
	// Invalidate the viewport's display.
	SceneViewport->Invalidate();
}

void STableauEditorViewport::UpdatePreviewTableau(UTableauAsset* InTableau, bool bResetCamera/*= true*/)
{
	// Clear any existing components
	for (USceneComponent* Component : TableauComponents)
	{
		Component->DestroyComponent();
	}
	TableauComponents.Empty();

	FBox Box(ForceInit);

	if (Tableau != nullptr)
	{
		// Express the latent tree -- disregarding foliage and captured config -- for display in the editor.
		
		TSharedPtr<FTableauFilterSampler> Filter = MakeShareable(new FTableauFilterSampler(FTransform::Identity));
		FTableauLatentTree TableauLatentTree(Tableau, Filter, true);
		TableauLatentTree.EvaluateLatentTree(Seed);

		TArray<FTableauRecipeNode> Recipes = TableauLatentTree.GetRecipe();
		for (const FTableauRecipeNode& RecipeNode : Recipes)
		{
			if (RecipeNode.AssetReference.IsValid())
			{
				USceneComponent* NewComponent = FTableauUtils::BuildComponent(RecipeNode.AssetReference.Get());
				TableauComponents.Add(NewComponent);
				PreviewScene->AddComponent(NewComponent, RecipeNode.LocalTransform);
				Box += NewComponent->Bounds.GetBox();
			}
		}

		if (bResetCamera)
		{
			// Set camera
			EditorViewportClient->FocusViewportOnBox(Box);
		}		
	}
}

bool STableauEditorViewport::IsVisible() const
{
	return ViewportWidget.IsValid() && (!ParentTab.IsValid() || ParentTab.Pin()->IsForeground());
}

FTableauEditorViewportClient& STableauEditorViewport::GetViewportClient()
{
	return *EditorViewportClient;
}

TSharedRef<FEditorViewportClient> STableauEditorViewport::MakeEditorViewportClient()
{
	EditorViewportClient = MakeShareable(new FTableauEditorViewportClient(TableauEditorPtr, SharedThis(this), PreviewScene.ToSharedRef()));

	EditorViewportClient->bSetListenerPosition = false;

	EditorViewportClient->SetRealtime(true);
	EditorViewportClient->VisibilityDelegate.BindSP(this, &STableauEditorViewport::IsVisible);

	return EditorViewportClient.ToSharedRef();
}

EVisibility STableauEditorViewport::OnGetViewportContentVisibility() const
{
	return IsVisible() ? EVisibility::Visible : EVisibility::Collapsed;
}


#undef LOCTEXT_NAMESPACE