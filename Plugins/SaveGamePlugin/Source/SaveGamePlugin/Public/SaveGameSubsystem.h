// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Tasks/Pipe.h"

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGameSubsystem.generated.h"

class USaveGameSettings;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveLoadStart);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaveLoadDone);

/**
 * Subsystem responsible for managing game save operations.
 * Provides functionality for saving and loading game data across levels and sessions.
 * Coordinates save game workflows and ensures data persistence.
 */
UCLASS()
class SAVEGAMEPLUGIN_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Called when the system starts saving a level */
	UPROPERTY(BlueprintAssignable)
	FSaveLoadStart OnSaveStart;

	/** Called when the system starts loading a level */
	UPROPERTY(BlueprintAssignable)
	FSaveLoadStart OnLoadStart;

	/** Called when the system finished saving a level */
	UPROPERTY(BlueprintAssignable)
	FSaveLoadDone OnSaveDone;

	/** Called when the system finished loading a level */
	UPROPERTY(BlueprintAssignable)
	FSaveLoadDone OnLoadDone;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Save the game data to an archive
	 * @param SaveName - the name to use when saving the Archive
	 */
	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Save")
	void Save(FString SaveName);

	/** Load the game data from an archive
	 * @param SaveName - the name to use when loading the Archive
	 */
	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Load")
	void Load(FString SaveName);

	UFUNCTION(BlueprintCallable, Category="SaveGamePlugin|Load")
	bool IsLoadingSaveGame() const;

	/** Get the last known Savetime of the save archive
	 * Time is expressed in UTC time, convert to local time if needed
	 */
	UFUNCTION(BlueprintPure, Category="SaveGamePlugin")
	FDateTime GetLastSaveTimestamp() const { return LastSaveTimestamp; }

	void SetLastSaveTimestamp(FDateTime Timestamp) { LastSaveTimestamp = Timestamp; }

protected:
	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues);
	void OnActorsInitialized(const FActorsInitializedParams& Params);
	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);
	void OnPreLevelRemovedFromWorld(ULevel* Level, UWorld* World);
	void OnWorldCleanup(UWorld* World, bool, bool);
	void OnPreWorldDestroyed(UWorld* World);

	void OnActorPreSpawn(AActor* Actor);
	void OnActorDestroyed(AActor* Actor);

	static USaveGameSettings* GetSaveGameSubsystemSettings();

	UPROPERTY()
	USaveGameSettings* SaveGameSettings;

private:
	template <bool>
	friend class TSaveGameSerializer;
	UE::Tasks::FPipe SaveGamePipe = UE::Tasks::FPipe(TEXT("SaveGameSubsystem"));

	TSet<FSoftObjectPath> DestroyedLevelActors;
	TSet<TWeakObjectPtr<AActor>> SaveGameActors;

	/** Holds the last known timestamp for saving/loading */
	UPROPERTY(VisibleAnywhere, Category="Save Game")
	FDateTime LastSaveTimestamp;
};
