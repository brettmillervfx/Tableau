
// This Include
#include "TableauEditorViewportClient.h"

// Engine Includes
#include "AdvancedPreviewScene.h"
#include "AssetViewerSettings.h"
#include "SEditorViewport.h"
#include "EditorViewportClient.h"

// Local Includes
#include "STableauEditorViewport.h"


namespace {
	static const float LightRotSpeed = 0.22f;
	static const float StaticMeshEditor_RotateSpeed = 0.01f;
	static const float	StaticMeshEditor_TranslateSpeed = 0.25f;
	static const float GridSize = 2048.0f;
	static const int32 CellSize = 16;
	static const float AutoViewportOrbitCameraTranslate = 256.0f;

	static float AmbientCubemapIntensity = 0.4f;
}

FTableauEditorViewportClient::FTableauEditorViewportClient(TWeakPtr<ITableauEditor> InTableauEditor, const TSharedRef<STableauEditorViewport>& InTableauEditorViewport, const TSharedRef<FAdvancedPreviewScene>& InPreviewScene)
	: FEditorViewportClient(nullptr, &InPreviewScene.Get(), StaticCastSharedRef<SEditorViewport>(InTableauEditorViewport))
	, TableauEditorPtr(InTableauEditor)
	, TableauEditorViewportPtr(InTableauEditorViewport)
{
	
	// Setup defaults for the common draw helper.
	DrawHelper.bDrawPivot = false;
	DrawHelper.bDrawWorldBox = false;
	DrawHelper.bDrawKillZ = false;
	DrawHelper.bDrawGrid = true;
	DrawHelper.GridColorAxis = FColor(160, 160, 160);
	DrawHelper.GridColorMajor = FColor(144, 144, 144);
	DrawHelper.GridColorMinor = FColor(128, 128, 128);
	DrawHelper.PerspectiveGridSize = GridSize;
	DrawHelper.NumCells = DrawHelper.PerspectiveGridSize / (CellSize * 2);

	SetViewMode(VMI_Lit);

	WidgetMode = FWidget::WM_None;

	EngineShowFlags.SetSeparateTranslucency(true);
	EngineShowFlags.SetSnap(0);
	EngineShowFlags.SetCompositeEditorPrimitives(true);
	EngineShowFlags.SetSelectionOutline(GetDefault<ULevelEditorViewportSettings>()->bUseSelectionOutline);
	OverrideNearClipPlane(1.0f);
	bUsingOrbitCamera = true;

	AdvancedPreviewScene = static_cast<FAdvancedPreviewScene*>(PreviewScene);

}

