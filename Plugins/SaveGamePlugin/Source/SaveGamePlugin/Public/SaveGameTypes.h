#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "SaveGameTypes.generated.h"

/**
* The save data of an individual level within a world
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
	TMap<TSoftObjectPtr<UWorld>, FLevelSaveData> SubLevels;
};
