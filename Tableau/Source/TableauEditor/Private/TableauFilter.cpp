
#include "TableauFilter.h"

// Engine Includes
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeComponent.h"
#include "LandscapeInfo.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"

// Local Includes
#include "TableauExclusionVolume.h"


FTableauCylinderVolumeFilter::FTableauCylinderVolumeFilter(const FVector& InCenter, float InRadius)
	: Center(InCenter)
	, Radius(InRadius)
{
}

bool FTableauCylinderVolumeFilter::Sample(const FVector& Location) const
{
	float Distance = FVector::Dist2D(Location, Center);
	return (Distance > Radius);
}


FTableauLandscapeWeightmapFilter::FTableauLandscapeWeightmapFilter(const FName InLayerName, float WeightThreshold)
	: LandscapeLayerName(InLayerName)
	, Threshold(WeightThreshold)
{
	LandscapeLayerCacheData.Reset(new TMap<ULandscapeComponent*, TArray<uint8>>);
	LandscapeLayerCacheData->Empty();
}

FTableauLandscapeWeightmapFilter::~FTableauLandscapeWeightmapFilter()
{
	LandscapeLayerCacheData->Empty();
}

bool FTableauLandscapeWeightmapFilter::Sample(const FVector& Location) const
{
	// Line trace the world and find the landscape component beneath this location.
	const static float SampleHeightOffset = 1000.0f;
	const static float SampleDepth = 5000.0f;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	TArray <struct FHitResult> OutHits;
	const FVector Start = Location + FVector(0.0f, 0.0f, SampleHeightOffset);
	const FVector End = Location - FVector(0.0f, 0.0f, SampleDepth);

	if (World->LineTraceMultiByObjectType(OutHits, Start, End, FCollisionObjectQueryParams(ECollisionChannel::ECC_WorldStatic)))
	{
		// Find hit result corresponding to landscape
		for (const auto& Hit : OutHits)
		{
			if (ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.Component.Get()))
			{
				if (ULandscapeComponent* HitLandscape = HitLandscapeCollision->RenderComponent.Get())
				{
					// Cache store mapping between component and weight data
					TArray<uint8>* LayerCache = &LandscapeLayerCacheData->FindOrAdd(HitLandscape);
					
					const float HitWeight = HitLandscape->GetLayerWeightAtLocation(Location, HitLandscape->GetLandscapeInfo()->GetLayerInfoByName(LandscapeLayerName), LayerCache);

					if (HitWeight > Threshold)
					{
						return true;
					}
				}		
			}
		}
	}

	return false;
}


FTableauSplineFilter::FTableauSplineFilter(TWeakObjectPtr<USplineComponent> InSplineComponent, float InRadius)
	: Radius(InRadius)
	, SplineComponent(InSplineComponent)
{
}

bool FTableauSplineFilter::Sample(const FVector& Location) const
{
	const FVector ClosestLocation = SplineComponent->FindLocationClosestToWorldLocation(Location, ESplineCoordinateSpace::Type::World);
	const float Distance = FVector::Dist2D(Location, ClosestLocation);
	return (Distance > Radius);
}


FTableauTagFilter::FTableauTagFilter(const FName InTag, float InTolerance)
	: Tag(InTag)
	, Tolerance(InTolerance)
{
}

bool FTableauTagFilter::Sample(const FVector& Location) const
{
	// Establish vertical ray along which the trace will be conducted.
	FVector TraceStart = Location + FVector(0.0, 0.0, Tolerance);
	FVector TraceEnd = Location - FVector(0.0, 0.0, Tolerance);

	FHitResult HitResult;

	FCollisionQueryParams CollisionQueryParams;
	FCollisionResponseParams CollisionResponseParams;

	if (GEditor->GetEditorWorldContext().World()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_WorldStatic, CollisionQueryParams, CollisionResponseParams))
	{
		if (AActor* HitActor = HitResult.GetActor())
		{
			if (HitActor->ActorHasTag(Tag))
			{
				return false;
			}
		}
	}

	return true;
}


FTableauExclusionVolumeFilter::FTableauExclusionVolumeFilter(const TWeakObjectPtr<ATableauExclusionVolume> InVolume)
	: ExclusionVolume(InVolume)
{
	
}

bool FTableauExclusionVolumeFilter::Sample(const FVector& Location) const
{
	return !ExclusionVolume->EncompassesPoint(Location, 0.0f, nullptr);
}



FTableauFilterSampler::FTableauFilterSampler(const FTransform& InToWorldTransform)
	: ToWorldTransform(InToWorldTransform)
{
	Filters.Empty();
}

FTableauFilterSampler::~FTableauFilterSampler()
{
	Filters.Empty();
}

void FTableauFilterSampler::AddCylinderVolumeFilter(const FVector& Origin, float Radius)
{
	TSharedPtr<FTableauFilter> NewFilter(new FTableauCylinderVolumeFilter(Origin, Radius));
	Filters.Add(NewFilter);
}

void FTableauFilterSampler::AddLandscapeLayerFilter(const FName& LayerName, float WeightThreshold)
{
	TSharedPtr<FTableauFilter> NewFilter(new FTableauLandscapeWeightmapFilter(LayerName, WeightThreshold));
	Filters.Add(NewFilter);
}

void FTableauFilterSampler::AddSplineFilter(TWeakObjectPtr<USplineComponent> SplineComponent, float Radius)
{
	TSharedPtr<FTableauFilter> NewFilter(new FTableauSplineFilter(SplineComponent, Radius));
	Filters.Add(NewFilter);
}

void FTableauFilterSampler::AddTagFilter(const FName FilterTag, float Tolerance)
{
	TSharedPtr<FTableauFilter> NewFilter(new FTableauTagFilter(FilterTag, Tolerance));
	Filters.Add(NewFilter);
}

void FTableauFilterSampler::AddAllLoadedExclusionVolumes(UWorld* World)
{
	// Find all loaded TableauExclusionActors and add filters for them.
	for (TActorIterator<ATableauExclusionVolume> ActorItr(World); ActorItr; ++ActorItr)
	{
		if (ATableauExclusionVolume* ExclusionVolume = Cast<ATableauExclusionVolume>(*ActorItr))
		{
			TSharedPtr<FTableauFilter> NewFilter(new FTableauExclusionVolumeFilter(ExclusionVolume));
			Filters.Add(NewFilter);
		}	
	}
}

bool FTableauFilterSampler::Sample(const FVector& TestLocation) const
{
	for (const TSharedPtr<FTableauFilter> Filter : Filters)
	{
		if (!Filter->Sample(ToWorldTransform.TransformPosition(TestLocation)))
		{
			return false;
		}
	}
	return true;
}

void FTableauFilterSampler::Reset()
{
	Filters.Empty();
}
