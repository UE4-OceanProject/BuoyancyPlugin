// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#include "BuoyantMesh/BuoyantMeshVertex.h"

bool FBuoyantMeshVertex::IsUnderwater() const
{
	return Height < 0.f;
}

FBuoyantMeshVertex::FBuoyantMeshVertex(const FVector& Position, float HeightAboveWater)
    : Height{HeightAboveWater}, Position{Position}
{
}