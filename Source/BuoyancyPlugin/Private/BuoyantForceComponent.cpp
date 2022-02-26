// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#include "BuoyantForceComponent.h"
#include "Engine/World.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "GameFramework/PhysicsVolume.h"
#include "PhysicsEngine/ConstraintInstance.h"
 	
UBuoyantForceComponent::UBuoyantForceComponent(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TickGroup;
	bWantsInitializeComponent = true;
	bAutoActivate = true;
	
	//Defaults
	MeshDensity = 600.0f;
	FluidDensity = 1025.0f;
	TestPointRadius = 10.0f;
	FluidLinearDamping = 1.0f;
	FluidAngularDamping = 1.0f;

	VelocityDamper = FVector(0.1, 0.1, 0.1);
	MaxUnderwaterVelocity = 1000.f;

	StayUprightStiffness = 50.0f;
	StayUprightDamping = 5.0f;

	WaveForceMultiplier = 2.0f;
}

void UBuoyantForceComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//Store the world ref.
	World = GetWorld();

	// If no OceanManager is defined, auto-detect
	if (!OceanManager)
	{
		for (TActorIterator<AOceanManager> ActorItr(World); ActorItr; ++ActorItr)
		{
			OceanManager = Cast<AOceanManager>(*ActorItr);
			break;
		}
	}

	TestPointRadius = FMath::Abs(TestPointRadius);

	UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(GetAttachParent());
	if (BasePrimComp)
	{
		ApplyUprightConstraint(BasePrimComp);

		//Store the initial damping values.
		_baseLinearDamping = BasePrimComp->GetLinearDamping();
		_baseAngularDamping = BasePrimComp->GetAngularDamping();
	}
}

void UBuoyantForceComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// If disabled or we are not attached to a parent component, return.
	if (!IsActive() || !GetAttachParent()) return;

	if (!OceanManager) return;

	UPrimitiveComponent* BasePrimComp = Cast<UPrimitiveComponent>(GetAttachParent());
	if (!BasePrimComp) return;

	if (!BasePrimComp->IsSimulatingPhysics())
	{
		if (!SnapToSurfaceIfNoPhysics) return;

		UE_LOG(LogTemp, Warning, TEXT("Running in no physics mode.."));

		float waveHeight = OceanManager->GetWaveHeightValue(BasePrimComp->GetComponentLocation(), World, true, TwoGerstnerIterations).Z;
		BasePrimComp->SetWorldLocation(FVector(BasePrimComp->GetComponentLocation().X, BasePrimComp->GetComponentLocation().Y, waveHeight));
		return;
	}

	//Get gravity
	float Gravity = BasePrimComp->GetPhysicsVolume()->GetGravityZ();

	//--------------- If Skeletal ---------------
	USkeletalMeshComponent* SkeletalComp = Cast<USkeletalMeshComponent>(GetAttachParent());
	if (SkeletalComp && ApplyForceToBones)
	{
		TArray<FName> BoneNames;
		SkeletalComp->GetBoneNames(BoneNames);

		for (int32 Itr = 0; Itr < BoneNames.Num(); Itr++)
		{
			FBodyInstance* BI = SkeletalComp->GetBodyInstance(BoneNames[Itr], false);
			if (BI && BI->IsValidBodyInstance()
				&& BI->bEnableGravity) //Buoyant doesn't exist without gravity
			{
				bool isUnderwater = false;
				//FVector worldBoneLoc = SkeletalComp->GetBoneLocation(BoneNames[Itr]);
				FVector worldBoneLoc = BI->GetCOMPosition(); //Use center of mass of the bone's physics body instead of bone's location
				FVector waveHeight = OceanManager->GetWaveHeightValue(worldBoneLoc, World, true, TwoGerstnerIterations);

				float BoneDensity = MeshDensity;
				float BoneTestRadius = FMath::Abs(TestPointRadius);
				float SignedBoneRadius = FMath::Sign(Gravity) * TestPointRadius; //Direction of radius (test radius is actually a Z offset, should probably rename it!). Just in case we need an upside down world.

				//Get density & radius from the override array, if available.
				for (int pointIndex = 0; pointIndex < BoneOverride.Num(); pointIndex++)
				{
					FStructBoneOverride Override = BoneOverride[pointIndex];

					if (Override.BoneName.IsEqual(BoneNames[Itr]))
					{
						BoneDensity = Override.Density;
						BoneTestRadius = FMath::Abs(Override.TestRadius);
						SignedBoneRadius = FMath::Sign(Gravity) * BoneTestRadius;
					}
				}

				//If test point radius is below water surface, add Buoyant force.
				if (waveHeight.Z > (worldBoneLoc.Z + SignedBoneRadius))
				{
					isUnderwater = true;

					float DepthMultiplier = (waveHeight.Z - (worldBoneLoc.Z + SignedBoneRadius)) / (BoneTestRadius * 2);
					DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

					float Mass = SkeletalComp->CalculateMass(BoneNames[Itr]); //Mass of this specific bone's physics body

					 /**
					* --------
					* Buoyant force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) / Total Points * Depth Multiplier
					* --------
					*/
					float BuoyantForceZ = Mass / BoneDensity * FluidDensity * -Gravity * DepthMultiplier;

					//Velocity damping.
					FVector DampingForce = -BI->GetUnrealWorldVelocity() * VelocityDamper * Mass * DepthMultiplier;

					//Experimental xy wave force
					if (EnableWaveForces)
					{
						float waveVelocity = FMath::Clamp(BI->GetUnrealWorldVelocity().Z, -20.f, 150.f) * (1 - DepthMultiplier);
						DampingForce += FVector(OceanManager->GlobalWaveDirection.X, OceanManager->GlobalWaveDirection.Y, 0) * Mass * waveVelocity * WaveForceMultiplier;
					}

					//Add force to this bone
					BI->AddForce(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ));
					//BasePrimComp->AddForceAtLocation(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ), worldBoneLoc, BoneNames[Itr]);
				}

				//Apply fluid damping & clamp velocity
				if (isUnderwater)
				{
					BI->SetLinearVelocity(-BI->GetUnrealWorldVelocity() * (FluidLinearDamping / 10), true);
					BI->SetAngularVelocityInRadians(-BI->GetUnrealWorldAngularVelocityInRadians() * FMath::DegreesToRadians(FluidAngularDamping / 10), true);

					//Clamp the velocity to MaxUnderwaterVelocity
					if (ClampMaxVelocity && BI->GetUnrealWorldVelocity().Size() > MaxUnderwaterVelocity)
					{
						FVector	Velocity = BI->GetUnrealWorldVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
						BI->SetLinearVelocity(Velocity, false);
					}
				}

				if (DrawDebugPoints)
				{
					FColor DebugColor = FLinearColor(0.8, 0.7, 0.2, 0.8).ToRGBE();
					if (isUnderwater) { DebugColor = FLinearColor(0, 0.2, 0.7, 0.8).ToRGBE(); } //Blue color underwater, yellow out of watter
					DrawDebugSphere(World, worldBoneLoc, BoneTestRadius, 8, DebugColor);
				}
			}
		}
		return;
	}
	//--------------------------------------------------------

	float TotalPoints = TestPoints.Num();
	if (TotalPoints < 1) return;

	PointsUnderWater = 0;
	for (int pointIndex = 0; pointIndex < TotalPoints; pointIndex++)
	{
		if (!TestPoints.IsValidIndex(pointIndex)) return; //Array size changed during runtime

		bool isUnderwater = false;
		FVector testPoint = TestPoints[pointIndex];
		FVector worldTestPoint = BasePrimComp->GetComponentTransform().TransformPosition(testPoint);
		FVector waveHeight = OceanManager->GetWaveHeightValue(worldTestPoint, World, !EnableWaveForces, TwoGerstnerIterations);

		//Direction of radius (test radius is actually a Z offset, should probably rename it!). Just in case we need an upside down world.
		float SignedRadius = FMath::Sign(BasePrimComp->GetPhysicsVolume()->GetGravityZ()) * TestPointRadius;

		//If test point radius is below water surface, add Buoyant force.
		if (waveHeight.Z > (worldTestPoint.Z + SignedRadius)
			&& BasePrimComp->IsGravityEnabled()) //Buoyant doesn't exist without gravity
		{
			PointsUnderWater++;
			isUnderwater = true;

			float DepthMultiplier = (waveHeight.Z - (worldTestPoint.Z + SignedRadius)) / (TestPointRadius * 2);
			DepthMultiplier = FMath::Clamp(DepthMultiplier, 0.f, 1.f);

			//If we have a point density override, use the overridden value instead of MeshDensity
			float PointDensity = PointDensityOverride.IsValidIndex(pointIndex) ? PointDensityOverride[pointIndex] : MeshDensity;

			/**
			* --------
			* Buoyant force formula: (Volume(Mass / Density) * Fluid Density * -Gravity) / Total Points * Depth Multiplier
			* --------
			*/
			float BuoyantForceZ = BasePrimComp->GetMass() / PointDensity * FluidDensity * -Gravity / TotalPoints * DepthMultiplier;

			//Experimental velocity damping using GetUnrealWorldVelocityAtPoint!
			FVector DampingForce = -GetUnrealVelocityAtPoint(BasePrimComp, worldTestPoint) * VelocityDamper * BasePrimComp->GetMass() * DepthMultiplier;

			//Experimental xy wave force
			if (EnableWaveForces)
			{
				DampingForce += BasePrimComp->GetMass() * FVector2D(waveHeight.X, waveHeight.Y).Size() * FVector(OceanManager->GlobalWaveDirection.X, OceanManager->GlobalWaveDirection.Y, 0) * WaveForceMultiplier / TotalPoints;
				//float waveVelocity = FMath::Clamp(GetUnrealVelocityAtPoint(BasePrimComp, worldTestPoint).Z, -20.f, 150.f) * (1 - DepthMultiplier);
				//DampingForce += OceanManager->GlobalWaveDirection * BasePrimComp->GetMass() * waveVelocity * WaveForceMultiplier / TotalPoints;
			}

			//Add force for this test point
			BasePrimComp->AddForceAtLocation(FVector(DampingForce.X, DampingForce.Y, DampingForce.Z + BuoyantForceZ), worldTestPoint);
		}

		if (DrawDebugPoints)
		{
			FColor DebugColor = FLinearColor(0.8, 0.7, 0.2, 0.8).ToRGBE();
			if (isUnderwater) { DebugColor = FLinearColor(0, 0.2, 0.7, 0.8).ToRGBE(); } //Blue color underwater, yellow out of watter
			DrawDebugSphere(World, worldTestPoint, TestPointRadius, 8, DebugColor);
		}
	}

	//Clamp the velocity to MaxUnderwaterVelocity if there is any point underwater
	if (ClampMaxVelocity && PointsUnderWater > 0
		&& BasePrimComp->GetPhysicsLinearVelocity().Size() > MaxUnderwaterVelocity)
	{
		FVector	Velocity = BasePrimComp->GetPhysicsLinearVelocity().GetSafeNormal() * MaxUnderwaterVelocity;
		BasePrimComp->SetPhysicsLinearVelocity(Velocity);
	}

	//Update damping based on number of underwater test points
	BasePrimComp->SetLinearDamping(_baseLinearDamping + FluidLinearDamping / TotalPoints * PointsUnderWater);
	BasePrimComp->SetAngularDamping(_baseAngularDamping + FluidAngularDamping / TotalPoints * PointsUnderWater);
}

FVector UBuoyantForceComponent::GetUnrealVelocityAtPoint(UPrimitiveComponent* Target, FVector Point, FName BoneName)
{
	if (!Target) return FVector::ZeroVector;

	FBodyInstance* BI = Target->GetBodyInstance(BoneName);
	if (BI->IsValidBodyInstance())
	{
		return BI->GetUnrealWorldVelocityAtPoint(Point);
	}

	return FVector::ZeroVector;
}

void UBuoyantForceComponent::ApplyUprightConstraint(UPrimitiveComponent* BasePrimComp)
{
	//Stay upright physics constraint (inspired by UDK's StayUprightSpring)
	if (EnableStayUprightConstraint)
	{
		UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(BasePrimComp);

		//Settings
		FConstraintInstance ConstraintInstance;

		ConstraintInstance.SetLinearXMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearYMotion(ELinearConstraintMotion::LCM_Free);
		ConstraintInstance.SetLinearZMotion(ELinearConstraintMotion::LCM_Free);

		//ConstraintInstance.LinearLimitSize = 0;

		//ConstraintInstance.SetAngularSwing1Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularSwing2Motion(EAngularConstraintMotion::ACM_Limited);
		ConstraintInstance.SetAngularTwistMotion(EAngularConstraintMotion::ACM_Limited);

		ConstraintInstance.SetOrientationDriveTwistAndSwing(true, true);

		//ConstraintInstance.SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
		ConstraintInstance.SetAngularTwistLimit(EAngularConstraintMotion::ACM_Locked, 0);

		ConstraintInstance.SetAngularDriveParams(StayUprightStiffness, StayUprightDamping, 0);

		ConstraintInstance.AngularRotationOffset = BasePrimComp->GetComponentRotation().GetInverse() + StayUprightDesiredRotation;

		//UPhysicsConstraintComponent* ConstraintComp = NewObject<UPhysicsConstraintComponent>(BasePrimComp);
		if (ConstraintComp)
		{
			ConstraintComp->ConstraintInstance = ConstraintInstance; //Set instance parameters
			ConstraintComp->SetWorldLocation(BasePrimComp->GetComponentLocation());

			//Attach
			ConstraintComp->AttachToComponent(BasePrimComp, FAttachmentTransformRules::KeepRelativeTransform, NAME_None);
			ConstraintComp->SetConstrainedComponents(BasePrimComp, NAME_None, NULL, NAME_None);
			ConstraintComp->RegisterComponent();
		}
	}
}
