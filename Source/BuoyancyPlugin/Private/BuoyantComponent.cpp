// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#include "BuoyantComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/ConstraintInstance.h"


UBuoyantComponent::UBuoyantComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBuoyantComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Store the world ref.
	UWorld* World = GetWorld();

	// If no OceanManager is defined, auto-detect
	if (!OceanManager)
	{
		for (TActorIterator<AOceanManager> ActorItr(World); ActorItr; ++ActorItr)
		{
			OceanManager = Cast<AOceanManager>(*ActorItr);
			break;
		}
	}

	ApplyUprightConstraint();

	TestPointRadius = FMath::Abs(TestPointRadius);

	// Signed based on gravity, just in case we need an upside down world
	SignedRadius = FMath::Sign(GetGravityZ()) * TestPointRadius;

	if (IsValid(UpdatedPrimitive))
	{
		// Store the initial damping values.
		BaseLinearDamping = UpdatedPrimitive->GetLinearDamping();
		BaseAngularDamping = UpdatedPrimitive->GetAngularDamping();
	}
}

void UBuoyantComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyUprightConstraint();
}

void UBuoyantComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Make sure everything is valid
	if (!OceanManager || !UpdatedComponent || !UpdatedPrimitive) return;

	// No physics
	if (!UpdatedComponent->IsSimulatingPhysics())
	{
		const FVector WaveHeight = OceanManager->GetWaveHeightValue(UpdatedComponent->GetComponentLocation());
		UpdatedPrimitive->SetWorldLocation(FVector(UpdatedComponent->GetComponentLocation().X, UpdatedComponent->GetComponentLocation().Y, WaveHeight.Z), true);
		return;
	}

	// Buoyant
	const float TotalPoints = TestPoints.Num();
	if (TotalPoints < 1) return;

	int PointsUnderWater = 0;
	for (int PointIndex = 0; PointIndex < TotalPoints; PointIndex++)
	{
		if (!TestPoints.IsValidIndex(PointIndex)) return; // Array size changed during runtime

		bool bIsUnderwater = false;
		FVector TestPoint = TestPoints[PointIndex];
		FVector WorldTestPoint = UpdatedComponent->GetComponentTransform().TransformPosition(TestPoint);
		const float WaveHeight = OceanManager->GetWaveHeightValue(WorldTestPoint).Z;

		// If test point radius is touching water add Buoyant force
		if (WaveHeight > (WorldTestPoint.Z + SignedRadius))
		{
			PointsUnderWater++;
			bIsUnderwater = true;

			float DepthMultiplier = (WaveHeight - (WorldTestPoint.Z + SignedRadius)) / (TestPointRadius * 2);
			DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

			// If we have a point density override, use the overridden value instead of MeshDensity
			const float PointDensity = PointDensityOverride.IsValidIndex(PointIndex) ? PointDensityOverride[PointIndex] : MeshDensity;

			/**
			* --------
			* Buoyant force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) / Total Points * Depth Multiplier
			* --------
			*/
			const float BuoyantForceZ = UpdatedPrimitive->GetMass() / PointDensity * FluidDensity * -GetGravityZ() / TotalPoints * DepthMultiplier;

			// Experimental velocity damping using GetUnrealWorldVelocityAtPoint!
			FVector DampingForce = -GetVelocityAtPoint(UpdatedPrimitive, WorldTestPoint) * VelocityDamper * UpdatedPrimitive->GetMass() * DepthMultiplier;

			// Wave push force
			if (EnableWaveForces)
			{
				const float WaveVelocity = FMath::Clamp(GetVelocityAtPoint(UpdatedPrimitive, WorldTestPoint).Z, -20.f, 150.f) * (1 - DepthMultiplier);
				DampingForce += FVector(OceanManager->GlobalWaveDirection.X, OceanManager->GlobalWaveDirection.Y, 0) * UpdatedPrimitive->GetMass() * WaveVelocity * WaveForceMultiplier / TotalPoints;
			}

			// Add force for this test point
			UpdatedPrimitive->AddForceAtLocation(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ), WorldTestPoint);
		}

		if (DrawDebugPoints)
		{
			FColor DebugColor = FLinearColor(0.8, 0.7, 0.2, 0.8).ToRGBE();
			if (bIsUnderwater) { DebugColor = FLinearColor(0, 0.2, 0.7, 0.8).ToRGBE(); } //Blue color underwater, yellow out of watter
			DrawDebugSphere(GetWorld(), WorldTestPoint, TestPointRadius, 8, DebugColor);
		}
	}

	// Clamp the velocity to MaxUnderwaterVelocity if there is any point underwater
	if (ClampMaxVelocity && PointsUnderWater > 0
		&& UpdatedPrimitive->GetPhysicsLinearVelocity().Size() > MaxUnderwaterVelocity)
	{
		const FVector LastVelocity = UpdatedPrimitive->GetPhysicsLinearVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
		UpdatedPrimitive->SetPhysicsLinearVelocity(LastVelocity);
	}

	// Update damping based on number of underwater test points
	UpdatedPrimitive->SetLinearDamping(BaseLinearDamping + FluidLinearDamping / TotalPoints * PointsUnderWater);
	UpdatedPrimitive->SetAngularDamping(BaseAngularDamping + FluidAngularDamping / TotalPoints * PointsUnderWater);
}

FVector UBuoyantComponent::GetVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName)
{
	if (!Target) return FVector::ZeroVector;

	FBodyInstance* Bi = Target->GetBodyInstance(BoneName);
	if (Bi->IsValidBodyInstance())
	{
		return Bi->GetUnrealWorldVelocityAtPoint(Point);
	}

	return FVector::ZeroVector;
}

void UBuoyantComponent::ApplyUprightConstraint()
{
	// Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	if (EnableStayUprightConstraint)
	{
		if (!ConstraintComp) ConstraintComp = NewObject<UPhysicsConstraintComponent>(UpdatedPrimitive);

		// Settings
		FConstraintInstance ConstraintInstance;

		ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);

		// ConstraintInstance.LinearLimitSize = 0;

		// ConstraintInstance.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Limited);

		ConstraintInstance.SetOrientationDriveTwistAndSwing(true, true);

		// ConstraintInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

		ConstraintInstance.SetAngularDriveParams(StayUprightStiffness, StayUprightDamping, 0);

		ConstraintInstance.AngularRotationOffset = UpdatedPrimitive->GetComponentRotation().GetInverse() + StayUprightDesiredRotation;

		// UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(UpdatedPrimitive);
		if (ConstraintComp)
		{
			ConstraintComp->ConstraintInstance = ConstraintInstance; //Set instance parameters
			ConstraintComp->SetWorldLocation(UpdatedPrimitive->GetComponentLocation());

			// Attach
			ConstraintComp->AttachToComponent(UpdatedComponent, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			ConstraintComp->SetConstrainedComponents(UpdatedPrimitive, NAME_None, NULL, NAME_None);
		}
	}
}
