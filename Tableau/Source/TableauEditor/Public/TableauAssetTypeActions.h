#pragma once

// Engine includes
#include "AssetTypeActions_Base.h"

// Local Includes
#include "TableauAsset.h"

/**
* AssetTypeActions for Tableau
*/
class FTableauAssetTypeActions : public FAssetTypeActions_Base
{

public:

	//~ Begin IAssetTypeActions
	virtual bool CanFilter() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;
	virtual uint32 GetCategories() override;
	virtual FText GetName() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual FColor GetTypeColor() const override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	//~ End IAssetTypeActions

private:
	// Handler for when Tableau is updated
	void UpdateTableauAsset(TArray<TWeakObjectPtr<UTableauAsset>> Objects);

	// Switch all contained elements to Deterministic mode.
	void MakeDeterministic(TArray<TWeakObjectPtr<UTableauAsset>> Objects);

	// Reimport actions
	bool CanExecuteReimport(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const;
	void ExecuteReimport(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const;

	// Wrap the current Tableau in a Composition container Tableau and update all referencers
	// to reference the container. This is useful for attaching additional elements to a Tableau without
	// directly editing it.
	void InsertCompositionContainer(const TArray<TWeakObjectPtr<UTableauAsset>> Objects) const;

};