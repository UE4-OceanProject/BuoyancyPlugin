// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"


// Associates a position with a height above water.
struct FBuoyantMeshVertex
{
	// Height Above Water
	const float Height;
	const FVector Position;

	bool IsUnderwater() const;

	FBuoyantMeshVertex(const FVector& Position, float HeightAboveWater);
};