// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"
#include "OceanPlugin/Public/OceanManager.h"
#include <Components/SceneComponent.h>
#include "BuoyantForceComponent.generated.h"


//Custom bone density/radius override struct.
USTRUCT(BlueprintType)
struct BUOYANCYPLUGIN_API FStructBoneOverride
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyant)
	FName BoneName;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyant)
	float Density;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Buoyant)
	float TestRadius;

	//Default struct values
	FStructBoneOverride()
	{
		Density = 600.f;
		TestRadius = 10.f;
	}
};

/** 
 *	Applies Buoyant forces to physics objects.
 *	OceanManager is required in the level for this to work.
 */
UCLASS(hidecategories=(Object, Mobility, LOD), ClassGroup=Physics, showcategories=Trigger, MinimalAPI, meta=(BlueprintSpawnableComponent))
class UBuoyantForceComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UBuoyantForceComponent(const class FObjectInitializer& ObjectInitializer);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void InitializeComponent() override;


	/* OceanManager used by the component, if unassigned component will auto-detect */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	AOceanManager* OceanManager;
	
	/* Density of mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float MeshDensity;

	/* Density of water. Typically you don't need to change this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidDensity;

	/* Linear damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidLinearDamping;

	/* Angular damping when object is in fluid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidAngularDamping;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	FVector VelocityDamper;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool ClampMaxVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float MaxUnderwaterVelocity;

	/* Radius of the points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings", meta = (DisplayName = "Buoyancy Point Radius"))
	float TestPointRadius;

	/* Buoyancy points array. At least one point is required for buoyancy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings", meta = (DisplayName = "Buoyancy Points"))
	TArray<FVector> TestPoints;

	/*This returns number of buoyancy points under water. Can be used to check if the mesh is on water or on land*/
	UPROPERTY(BlueprintReadOnly, Category = "Buoyancy Settings")
	int32 PointsUnderWater;

	/* If skeletal mesh with physics asset, it will apply buoyant force at the COM of each bone instead of using the test point array. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool ApplyForceToBones;

	/* If object has no physics enabled, snap to water surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool SnapToSurfaceIfNoPhysics;

	/**
	* More accurate gerstner wave height calculations by accounting for the x/y displacement.
	* Keep in mind that this effectively doubles the gerstner calculations per test point.
	* Use only if accurate height readback is needed.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool TwoGerstnerIterations;

	/* Per-point mesh density override, can be used for half-sinking objects etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	TArray<float> PointDensityOverride;

	/* Density & radius overrides per skeletal bone (ApplyForceToBones needs to be true). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	TArray<FStructBoneOverride> BoneOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool DrawDebugPoints;

	/**
	* Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	* -STILL WIP-
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool EnableStayUprightConstraint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float StayUprightStiffness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float StayUprightDamping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	FRotator StayUprightDesiredRotation;

	/**
	* Waves will push objects towards the wave direction set in the Ocean Manager.
	* -STILL WIP-
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool EnableWaveForces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float WaveForceMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	TEnumAsByte<enum ETickingGroup> TickGroup;

private:

	static FVector GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName = NAME_None);
	void ApplyUprightConstraint(UPrimitiveComponent* BasePrimComp);

	float _baseAngularDamping;
	float _baseLinearDamping;

	UWorld* World;
	
};
