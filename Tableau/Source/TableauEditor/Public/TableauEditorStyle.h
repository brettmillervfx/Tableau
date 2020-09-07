#pragma once

// Engine Includes
#include "Styling/SlateStyle.h"

/*
Editor Style Set for Tableau
*/
class FTableauEditorStyle : public FSlateStyleSet
{
public:
	FTableauEditorStyle();
	~FTableauEditorStyle();

	static void Initialize();
	static void Shutdown();

	static const ISlateStyle& Get();

private:
	static TSharedRef<class FSlateStyleSet> Create();

public:
	static TSharedPtr<class FSlateStyleSet> StyleInstance;


};
