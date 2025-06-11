#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "SaveGameTypes.generated.h"

/**
* The save data of an individual level within a world
* We store all the actors that implemented the save game interface
* We also keep track of the actors that get destroyed at runtime so we can destroy them again upon loading the save data.
 */
USTRUCT(BlueprintType)
struct FLevelSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveSystem")
	TSet<TSoftObjectPtr<AActor>> SaveGameActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveSystem")
	TSet<FSoftObjectPath> DestroyedLevelActors;
};

/**
 * The save data of all levels within a world
 */
USTRUCT(BlueprintType)
struct FWorldSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveSystem")
	TMap<FName, FLevelSaveData> SubLevels;
};
