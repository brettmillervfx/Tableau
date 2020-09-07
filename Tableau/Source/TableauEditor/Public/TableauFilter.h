#pragma once

// Engine Includes
#include "Math/Vector.h"
#include "Math/TransformNonVectorized.h"

// Local Includes

// Forward Declares
class ULandscapeComponent;
class USplineComponent;
class ATableauExclusionVolume;


// Abstract base class of all tableau filters.
class FTableauFilter
{
public:
	// Returns false if the sampled point should be culled.
	virtual bool Sample(const FVector& Location) const = 0;
};

class FTableauCylinderVolumeFilter : public FTableauFilter
{
public:
	FTableauCylinderVolumeFilter(const FVector& InCenter, float InRadius);

	virtual bool Sample(const FVector& Location) const override;

private:
	FVector Center;
	float Radius;
};

class FTableauLandscapeWeightmapFilter : public FTableauFilter
{
public:
	FTableauLandscapeWeightmapFilter(const FName InLayerName, float WeightThreshold);
	virtual ~FTableauLandscapeWeightmapFilter();

	virtual bool Sample(const FVector& Location) const override;

private:
	FName LandscapeLayerName;
	float Threshold;
	TUniquePtr<TMap<ULandscapeComponent*, TArray<uint8>>> LandscapeLayerCacheData;
};

class FTableauSplineFilter : public FTableauFilter
{
public:
	FTableauSplineFilter(const TWeakObjectPtr<USplineComponent> InSplineComponent, float InRadius);

	virtual bool Sample(const FVector& Location) const override;

private:
	float Radius;
	TWeakObjectPtr<USplineComponent> SplineComponent;
};

class FTableauExclusionVolumeFilter : public FTableauFilter
{
public:
	FTableauExclusionVolumeFilter(const TWeakObjectPtr<ATableauExclusionVolume> InVolume);

	virtual bool Sample(const FVector& Location) const override;

private:
	TWeakObjectPtr<ATableauExclusionVolume> ExclusionVolume;
};

class FTableauTagFilter : public FTableauFilter
{
public:
	FTableauTagFilter(const FName InTag, float InTolerance);

	virtual bool Sample(const FVector& Location) const override;

private:
	FName Tag;
	float Tolerance;
};


class FTableauFilterSampler
{
public:
	FTableauFilterSampler(const FTransform& InToWorldTransform);
	~FTableauFilterSampler();

	// Add a cylindrical filter. Any sample points inside the cylinder will be culled.
	void AddCylinderVolumeFilter(const FVector& Origin, float Radius);

	// Add a landscape weight map filter. The Landscape is sampled in the vertical line of the location.
	// If the prescribed weightmap has a value below the threshold at that location, the point is culled.
	void AddLandscapeLayerFilter(const FName& LayerName, float WeightmapThreshold);

	// Add a spline filter. The point is culled if the projected 2d distance from the closest 
	// location on the spline component is less than the Radius.
	void AddSplineFilter(TWeakObjectPtr<USplineComponent> SplineComponent, float Radius);

	// Add a tag filter. If the point is projected with tolerance onto an actor with the specified tag, it is filtered.
	void AddTagFilter(const FName FilterTag, float Tolerance);

	// Search the World for all TableauExclusionVolumes and include them in the filter.
	void AddAllLoadedExclusionVolumes(UWorld* World);

	// Tests a sample location: returns true if the test location should be kept.
	bool Sample(const FVector& TestLocation) const;

	// Clear all prexisting filters
	void Reset();

private:
	const FTransform ToWorldTransform;
	TArray<TSharedPtr<FTableauFilter>> Filters;

};