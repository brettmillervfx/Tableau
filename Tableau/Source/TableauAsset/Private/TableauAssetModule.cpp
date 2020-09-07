
#include "TableauAssetModule.h"

DEFINE_LOG_CATEGORY(LogTableau);

#define LOCTEXT_NAMESPACE "FTableauAssetModule"

void FTableauAssetModule::StartupModule()
{
	
}

void FTableauAssetModule::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTableauAssetModule, TableauAsset)