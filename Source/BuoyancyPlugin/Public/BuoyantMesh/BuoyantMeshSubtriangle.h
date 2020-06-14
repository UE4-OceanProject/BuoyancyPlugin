// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"


struct FBuoyantMeshVertex;

// Represents a submerged part of a BuoyantMeshTriangle.
struct FBuoyantMeshSubtriangle
{
	const FVector A;
	const FVector B;
	const FVector C;

	// Calculate the barycenter of the triangle.
	FVector GetCenter() const;

	// Calculate the area of the triangle.
	float GetArea() const;

	// Calculates the hydrostatic forces on the submerged part of triangle.
	// Returns a force vector and its application point.
	static FVector GetHydrostaticForce(const float WaterDensity,
	                                   const float GravityMagnitude,
	                                   const FBuoyantMeshVertex& Center,
	                                   const FVector& TriangleNormal,
	                                   const float TriangleArea);
	// Calculates the hydrodynamic forces on the submerged part of triangle.
	// Returns a force vector and its application point.
	static FVector GetHydrodynamicForce(const float WaterDensity,
	                                    const FVector& TriangleCenter,
	                                    const FVector& TriangleCenterVelocity,
	                                    const FVector& TriangleNormal,
	                                    float const TriangleArea);

	FBuoyantMeshSubtriangle(const FVector& A, const FVector& B, const FVector& C);

   private:
	// Calculate the area of the triangle by using Heron's formula
	static float GetTriangleAreaHeron(const FVector& Vertex1, const FVector& Vertex2, const FVector& Vertex3);
};
