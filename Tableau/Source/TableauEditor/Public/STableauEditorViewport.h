#pragma once

// Editor Includes
#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "UObject/GCObject.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "EditorViewportClient.h"
#include "AdvancedPreviewScene.h"
#include "SEditorViewport.h"
#include "SCommonEditorViewportToolbarBase.h"

// Local Includes
class ITableauEditor;
class SVerticalBox;
class UTableauAsset;
class UTableauComponent;



/**
 * Tableau Editor Preview viewport widget
 */
class STableauEditorViewport : public SEditorViewport, public ICommonEditorViewportToolbarInfoProvider
{
public:
	SLATE_BEGIN_ARGS(STableauEditorViewport) {}
		SLATE_ARGUMENT(TWeakPtr<ITableauEditor>, TableauEditor)
		SLATE_ARGUMENT(UTableauAsset*, ObjectToEdit)
	SLATE_END_ARGS()

	STableauEditorViewport();
	~STableauEditorViewport();

	// ICommonEditorViewportToolbarInfoProvider interface
	virtual TSharedRef<class SEditorViewport> GetViewportWidget() override;
	virtual TSharedPtr<FExtender> GetExtenders() const override;
	virtual void OnFloatingButtonClicked() override;
	// End of ICommonEditorViewportToolbarInfoProvider interface

	void Construct(const FArguments& InArgs);

	void RefreshViewport();
	
	void SetParentTab(TSharedRef<SDockTab> InParentTab) { ParentTab = InParentTab; }

	class FTableauEditorViewportClient& GetViewportClient();
	TSharedRef<FAdvancedPreviewScene> GetPreviewScene() { return PreviewScene.ToSharedRef(); }
	
	void UpdatePreviewTableau(UTableauAsset* InTableau, bool bResetCamera = true);

protected:
	// SEditorViewport interface 
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;
	virtual EVisibility OnGetViewportContentVisibility() const override;
	// End SEditorViewport interface 

private:
	//Determines the visibility of the viewport.
	bool IsVisible() const override;

private:
	// The parent tab where this viewport resides
	TWeakPtr<SDockTab> ParentTab;

	// Pointer back to the Tableau editor tool that owns us
	TWeakPtr<ITableauEditor> TableauEditorPtr;

	// The scene for this viewport.
	TSharedPtr<FAdvancedPreviewScene> PreviewScene;

	// Editor viewport client
	TSharedPtr<class FTableauEditorViewportClient> EditorViewportClient;

	// Tableau being edited
	UTableauAsset* Tableau;

	// Seed value used to evaluate current Tableau
	int32 Seed;

	// Components owned by viewport
	TArray<USceneComponent*> TableauComponents;

	// The currently selected view mode.
	EViewModeIndex CurrentViewMode;

};
