// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE
#include "BuoyantDestructibleComponent.h"
#include "GameFramework/PhysicsVolume.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Chaos/ParticleHandle.h"
#include "PhysicsProxy/GeometryCollectionPhysicsProxy.h"

UBuoyantDestructibleComponent::UBuoyantDestructibleComponent(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

	PrimaryComponentTick.bCanEverTick = true;
	bWantsInitializeComponent = true;
	bAutoActivate = true;

	//Defaults
	ChunkDensity = 600.0f;
	FluidDensity = 1025.0f;
	TestPointRadius = 10.0f;
	FluidLinearDamping = 1.0f;
	FluidAngularDamping = 1.0f;

	VelocityDamper = FVector(0.1, 0.1, 0.1);
	MaxUnderwaterVelocity = 1000.f;

	WaveForceMultiplier = 2.0f;
}

void UBuoyantDestructibleComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//// If no OceanManager is defined auto-detect
	if (!OceanManager)
	{
		for (TActorIterator<AOceanManager> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			OceanManager = Cast<AOceanManager>(*ActorItr);
			break;
		}
	}

	_baseLinearDamping = GetLinearDamping();
	_baseAngularDamping = GetAngularDamping();
}

void UBuoyantDestructibleComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OceanManager)
		return;

	float Gravity = GetPhysicsVolume()->GetGravityZ();
	TestPointRadius = FMath::Abs(TestPointRadius);

	//Signed based on gravity, just in case we need an upside down world
	_SignedRadius = FMath::Sign(Gravity) * TestPointRadius;
#if WITH_CHAOS
	if (GetPhysicsProxy()) {
		TArray<Chaos::FPBDRigidClusteredParticleHandle*> Particles = GetPhysicsProxy()->GetParticles();
		TArray<Chaos::FPBDRigidClusteredParticleHandle*> AppliedForceParticles;

		for (Chaos::FPBDRigidClusteredParticleHandle* Particle : Particles) {
			if (Particle)
			{
				//A particle will have a parent when it's not fully destructed yet
				Particle = Particle->Parent() ? Particle->Parent() : Particle;

				if (AppliedForceParticles.Contains(Particle))
					continue;

				Chaos::FVec3 Trans = Particle->X();
				Chaos::FRotation3 Rot = Particle->R();
				Chaos::FVec3 COM = Particle->CenterOfMass();
				Trans += Rot.RotateVector(COM);

				float waveHeight = OceanManager->GetWaveHeightValue(Trans).Z;
				bool isUnderwater = false;

				//If test point radius is touching water add Buoyant force
				if (waveHeight > (Trans.Z + _SignedRadius))
				{
					isUnderwater = true;

					float DepthMultiplier = (waveHeight - (Trans.Z + _SignedRadius)) / (TestPointRadius * 2);
					DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

					/**
					* --------
					* Buoyant force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) * Depth Multiplier
					* --------
					*/

					float BuoyantForceZ = Particle->M() / ChunkDensity * FluidDensity * -Gravity * DepthMultiplier;

					//Velocity damping
					FVector DampingForce = -Particle->V() * VelocityDamper * Particle->M() * DepthMultiplier;

					//Wave push force
					if (EnableWaveForces)
					{
						float waveVelocity = FMath::Clamp(Particle->V().Z, -20.f, 150.f) * (1 - DepthMultiplier);
						DampingForce += FVector(OceanManager->GlobalWaveDirection.X, OceanManager->GlobalWaveDirection.Y, 0) * Particle->M() * waveVelocity * WaveForceMultiplier;
					}

					if (Particle->Sleeping()) {
						Particle->SetObjectStateLowLevel(Chaos::EObjectStateType::Dynamic);
					}

					/*
					 *	Add force for this particle
					 *
					 *	A weird bug happenes here if we call Particle->AddForce()
					 *	AddForce modifies the acceleration of the body
					 *	but the velocity doesn't get updated as soon as acceleration gets modified
					 *	instead, velocity gets modified at random times and the body appears as if it's "lagging" (no the body isn't sleeping)
					 *	Also another weird thing
					 *	adding debug messages/objects like GEngine->AddOnScreenDebugMessage() or enabling DrawDebugPoints makes the lag worse!!???
					 *
					 *	I found that calling ApplyKinematicField() multiple times(about 50 times in a for loop)
					 *	forces the velocity to get modified faster (the more you call the function the less "lag" the body gets)
					 *
					 *	This bug doesn't happen when modifying the velocity of the object directly
					 *	I don't know what causes this bug..
					 *
					 */

					//Particle->SetV(Particle->V() + Chaos::FVec3(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ) * DeltaTime);


				   /*
					*	ok so to use AddForce we need to call a field command that dispatches to physics thread to force update the velocity
					*	if someone understands what's going on please let me know as i don't understand chaos anymore
					*/

					Particle->AddForce(Chaos::FVec3(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ));
					DispatchFieldCommand(FieldCommand);

					//To prevent adding force for the same parent particle multiple times
					//when multiple children particles are refrencing the same parent particle
					AppliedForceParticles.Add(Particle);

				}

				if (DrawDebugPoints)
				{
					FColor DebugColor = FLinearColor(0.8, 0.7, 0.2, 0.8).ToRGBE();
					if (isUnderwater) { DebugColor = FLinearColor(0, 0.2, 0.7, 0.8).ToRGBE(); } //Blue color underwater, yellow out of watter
					DrawDebugSphere(GetWorld(), Trans, TestPointRadius, 8, DebugColor);
				}

				//Update damping based on isUnderwater
				Particle->SetLinearEtherDrag(_baseLinearDamping + FluidLinearDamping * isUnderwater);
				Particle->SetAngularEtherDrag(_baseAngularDamping + FluidAngularDamping * isUnderwater);

				//Clamp the chunk's velocity to MaxUnderwaterVelocity if chunk is underwater
				if (ClampMaxVelocity && isUnderwater
					&& Particle->V().Size() > MaxUnderwaterVelocity)
				{
					FVector	Velocity = Particle->V().GetSafeNormal() * MaxUnderwaterVelocity;
					Particle->SetV(Velocity);
				}
			}
		}
	}
#endif // WITH_CHAOS 
}