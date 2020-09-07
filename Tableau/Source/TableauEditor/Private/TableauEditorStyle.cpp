
#include "TableauEditorStyle.h"

// Engine Includes
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"


#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

TSharedPtr<FSlateStyleSet> FTableauEditorStyle::StyleInstance = NULL;

FTableauEditorStyle::FTableauEditorStyle()
	: FSlateStyleSet("TableauEditorStyle")
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon64x64(40.0f, 40.0f);
	const FVector2D Icon128x128(128.0f, 128.0f);

	const FString BaseDir = IPluginManager::Get().FindPlugin("Tableau")->GetBaseDir();
	SetContentRoot(BaseDir / TEXT("Resources"));

	// Class Icon
	{
		Set("ClassIcon.TableauActor", new IMAGE_BRUSH(TEXT("Icons/TableauAssetIcon16"), Icon16x16));
		Set("ClassThumbnail.TableauActor", new IMAGE_BRUSH(TEXT("Icons/TableauAssetIcon64"), Icon64x64));

		Set("ClassIcon.TableauAsset", new IMAGE_BRUSH(TEXT("Icons/TableauAssetIcon16"), Icon16x16));
		Set("ClassThumbnail.TableauAsset", new IMAGE_BRUSH(TEXT("Icons/TableauAssetIcon64"), Icon64x64));

	}

}

FTableauEditorStyle::~FTableauEditorStyle()
{

}

const ISlateStyle& FTableauEditorStyle::Get()
{
	return *StyleInstance;
}

TSharedRef<FSlateStyleSet> FTableauEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FTableauEditorStyle());
	return Style;
}

void FTableauEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FTableauEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	StyleInstance.Reset();
}

