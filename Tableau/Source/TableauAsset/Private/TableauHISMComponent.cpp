#include "TableauHISMComponent.h"

UTableauHISMComponent::UTableauHISMComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bSnappable(true)
	, bAlignToNormal(false)
	, GroundSlopeAngle(0.0f, 90.0f)
{
}

void UTableauHISMComponent::ExcludeFromHLOD()
{
#if WITH_EDITOR
	static constexpr int32 HighestExpectedHLODLevel = 5;
	for (int32 HLODLevel = 0; HLODLevel < HighestExpectedHLODLevel; HLODLevel++)
	{
		ExcludeForSpecificHLODLevels.Add(HLODLevel);
	}
#endif
}