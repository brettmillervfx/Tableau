#pragma once

// Editor Includes
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "UnrealWidget.h"
#include "EditorViewportClient.h"

class FAdvancedPreviewScene;
class ITableauEditor;
class STableauEditorViewport;


/** Viewport Client for the preview viewport */
class FTableauEditorViewportClient : public FEditorViewportClient, public TSharedFromThis<FTableauEditorViewportClient>
{
public:
	FTableauEditorViewportClient(TWeakPtr<ITableauEditor> InTableauEditor, const TSharedRef<STableauEditorViewport>& InTableauEditorViewport, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene);

private:

	// Pointer back to the Tableau editor tool that owns us */
	TWeakPtr<ITableauEditor> TableauEditorPtr;

	FWidget::EWidgetMode WidgetMode;

	// Pointer back to the StaticMeshEditor viewport control that owns us 
	TWeakPtr<STableauEditorViewport> TableauEditorViewportPtr;

	// Stored pointer to the preview scene in which the tableau is shown
	FAdvancedPreviewScene* AdvancedPreviewScene;
};