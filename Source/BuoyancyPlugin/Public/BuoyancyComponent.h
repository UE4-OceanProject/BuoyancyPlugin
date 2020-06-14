// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "OceanPlugin/Public/OceanManager.h"
#include "BuoyancyComponent.generated.h"


/**
 *	Buoyancy component
 *	OceanManager is required in the level for this to work.
 */
UCLASS(ClassGroup = Movement, meta = (BlueprintSpawnableComponent), HideCategories = (PlanarMovement, "Components|Movement|Planar", Velocity))
class BUOYANCYPLUGIN_API UBuoyancyComponent : public UMovementComponent
{
	GENERATED_UCLASS_BODY()

	/* OceanManager used by the component, if unassigned component will auto-detect */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyancy Settings")
	AOceanManager* OceanManager;
	
	/* Density of mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float MeshDensity;

	/* Density of water. Typically you don't need to change this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float FluidDensity;

	/* Linear damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float FluidLinearDamping;

	/* Angular damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float FluidAngularDamping;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyancy Settings")
	FVector VelocityDamper;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	bool ClampMaxVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float MaxUnderwaterVelocity;

	/* Radius of the points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	float TestPointRadius;

	/* Test point array. At least one point is required for buoyancy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy Settings")
	TArray<FVector> TestPoints;

	/* Per-point mesh density override, can be used for half-sinking objects etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	TArray<float> PointDensityOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	bool DrawDebugPoints;

	/**
	* Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	* -STILL WIP-
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	bool EnableStayUprightConstraint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	float StayUprightStiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	float StayUprightDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	FRotator StayUprightDesiredRotation;

	/**
	* Waves will push objects towards the wave direction set in the Ocean Manager.
	* -STILL WIP-
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	bool EnableWaveForces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyancy Settings")
	float WaveForceMultiplier;

	//Begin UActorComponent Interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void InitializeComponent() override;
	//End UActorComponent Interface

private:

	static FVector GetVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName = NAME_None);

	void ApplyUprightConstraint();
	UPhysicsConstraintComponent* ConstraintComp;
	bool _hasTicked;

	float _SignedRadius;
	float _baseAngularDamping;
	float _baseLinearDamping;

	UWorld* World;
	
};
