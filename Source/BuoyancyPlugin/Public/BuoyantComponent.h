// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "OceanPlugin/Public/OceanManager.h"
#include "BuoyantComponent.generated.h"


/**
 *	Buoyant component
 *	OceanManager is required in the level for this to work.
 */
UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent), HideCategories = (PlanarMovement, "Components|Movement|Planar", Velocity))
class BUOYANCYPLUGIN_API UBuoyantComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
	UBuoyantComponent(const class FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;

	/* OceanManager used by the component, if unassigned component will auto-detect */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	AOceanManager* OceanManager;
	
	/* Density of mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float MeshDensity = 600.0f;

	/* Density of water. Typically you don't need to change this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidDensity = 1025.0f;

	/* Linear damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidLinearDamping = 1.0f;

	/* Angular damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidAngularDamping = 1.0f;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	FVector VelocityDamper = FVector(0.1, 0.1, 0.1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool ClampMaxVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float MaxUnderwaterVelocity = 1000.0f;

	/* Radius of the points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float TestPointRadius = 10.0f;

	/* Test point array. At least one point is required for buoyancy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	TArray<FVector> TestPoints;

	/* Per-point mesh density override, can be used for half-sinking objects etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	TArray<float> PointDensityOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool DrawDebugPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float StayUprightStiffness = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float StayUprightDamping = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	FRotator StayUprightDesiredRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float WaveForceMultiplier = 2.0f;


protected:
	/* Stay upright physics constraint (inspired by UDK's StayUprightSpring) (WIP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
		bool EnableStayUprightConstraint;


	/* Waves will push objects towards the wave direction set in the Ocean Manager. (WIP) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
		bool EnableWaveForces;

	UPROPERTY()
		UPhysicsConstraintComponent* ConstraintComp;

	float SignedRadius;
	float BaseAngularDamping;
	float BaseLinearDamping;

	static FVector GetVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName = NAME_None);

	void ApplyUprightConstraint();
};
