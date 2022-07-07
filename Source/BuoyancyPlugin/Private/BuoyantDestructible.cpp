// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#include "BuoyantDestructible.h"
#include "BuoyantDestructibleComponent.h"

ABuoyantDestructible::ABuoyantDestructible(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	BuoyantDestructibleComponent = CreateDefaultSubobject<UBuoyantDestructibleComponent>(TEXT("DestructibleComponent"));
	RootComponent = BuoyantDestructibleComponent;
}
