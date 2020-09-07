#pragma once

// Engine Includes
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTableau, Log, All);


namespace TableauColors
{
	constexpr FColor CompositionColor{ FColor(49, 100, 158) };
	constexpr FColor SuperpositionColor{ FColor(212, 86, 19) };
	constexpr FColor HierarchicalCompositionColor{ FColor(235, 52, 183) };
	constexpr FColor ErrorColor{ FColor(160, 0, 0) };
}


class FTableauAssetModule : public IModuleInterface
{
public:
	//~ Begin IModuleInterface interface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~ End IModuleInterface interface
};