// For copyright see LICENSE in EnvironmentProject root dir, or:
//https://github.com/UE4-OceanProject/OceanProject/blob/Master-Environment-Project/LICENSE

#pragma once

#include "CoreMinimal.h"
#include "BuoyantDestructibleComponent.h"
#include "BuoyantDestructible.generated.h"


UCLASS()
class BUOYANCYPLUGIN_API ABuoyantDestructible : public AActor
{
	GENERATED_UCLASS_BODY()
 
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = BuoyantDestructible, meta = (ExposeFunctionCategories = "Destruction,Components|Destructible,Buoyancy Settings,Advanced", AllowPrivateAccess = "true"))
	UBuoyantDestructibleComponent* BuoyantDestructibleComponent;
};
