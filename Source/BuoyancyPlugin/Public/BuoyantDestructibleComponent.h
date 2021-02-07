// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"
#include "OceanPlugin/Public/OceanManager.h"
#include "DestructibleComponent.h"
#include "BuoyantDestructibleComponent.generated.h"


UCLASS(ClassGroup = Physics, hidecategories = (Object, Mesh, "Components|SkinnedMesh", Mirroring, Activation, "Components|Activation"), config = Engine, editinlinenew, meta = (BlueprintSpawnableComponent))
class BUOYANCYPLUGIN_API UBuoyantDestructibleComponent : public UDestructibleComponent
{
	GENERATED_BODY()

public:
	UBuoyantDestructibleComponent(const class FObjectInitializer& ObjectInitializer);
 
protected:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	virtual void InitializeComponent() override;

private:
	float _SignedRadius;
	float _baseAngularDamping;
	float _baseLinearDamping;
 
public:
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	AOceanManager* OceanManager;

	/* Density of each chunk */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float ChunkDensity;

	/* Density of water */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidDensity;

	/* Linear damping when chunk is in fluid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidLinearDamping;

	/* Angular damping when chunk is in fluid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float FluidAngularDamping;

	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = "Buoyant Settings")
	FVector VelocityDamper;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	bool ClampMaxVelocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float MaxUnderwaterVelocity;

	/* Radius of the test point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyant Settings")
	float TestPointRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool DrawDebugPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	bool EnableWaveForces;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Buoyant Settings")
	float WaveForceMultiplier;

	/*
	* Sets the mass-normalized kinetic energy threshold below which an actor may go to sleep. 
	* Default physx value is ~50.0f (we set it 0 to avoid weird sleeping chunks on water).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	float ChunkSleepThreshold;

	/*
	* Sets the mass-normalized kinetic energy threshold below which an actor may participate in stabilization.
	* (This value has no effect if PxSceneFlag::eENABLE_STABILIZATION was not enabled on the PxSceneDesc)
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Advanced")
	float ChunkStabilizationThreshold;
 
};
