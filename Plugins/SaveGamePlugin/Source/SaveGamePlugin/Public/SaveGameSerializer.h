// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "Templates/ChooseClass.h"
#include "Tasks/Task.h"

class USaveGameSubsystem;

template <bool bIsLoading>
class TSaveGameArchive;

class FSaveGameSerializer : public TSharedFromThis<FSaveGameSerializer>
{
public:
	virtual ~FSaveGameSerializer() = default;

	virtual bool IsLoading() const = 0;
	virtual UE::Tasks::FTask DoOperation() = 0;
};

/**
 * WorldSerializationManager
 *
 * Manages serialization of the world data. The archive includes:
 * 
 *  ─ Header
 *     • VERSION_OFFSET
 *     • ENGINE_VERSION
 *     • PACKAGE_VERSION
 * 
 *  ─ Data
 *     • LastVisitedMapName
 *     • Timestamp
 *     • WorldData
 *       ◦ Level1
 *         ▪ DestroyedActors
 *         ▪ Actors
 *           › ActorName
 *           › Class (if spawned)
 *           › SpawnID (if implements ISaveGameSpawnActor)
 *           › SaveGameProperties
 *         ▪ SubLevel1
 *         ▪ SubLevel2
 *         ...
 *       ◦ Level2
 *       ...
 * 
 *  ─ Versions
 *     • VersionID
 *     • VersionNumber
 */
inline constexpr int VERSION_OFFSET_INDEX = 0;
inline constexpr int ENGINE_VERSION_INDEX = 1;
inline constexpr int PACKAGE_VERSION_INDEX = 2;

template <bool bIsLoading>
class TSaveGameSerializer final : public FSaveGameSerializer
{
	using TSaveGameMemoryArchive = typename TChooseClass<bIsLoading, FMemoryReader, FMemoryWriter>::Result;

public:
	TSaveGameSerializer(USaveGameSubsystem* InSaveGameSubsystem, FString InSaveName);
	virtual ~TSaveGameSerializer() override;

	virtual bool IsLoading() const override { return bIsLoading; }
	virtual UE::Tasks::FTask DoOperation() override;

	/** Set the archive save name */
	void SetSaveName(const FString& InSaveName) { SaveName = InSaveName; }

	/** Get the archive save name */
	FString GetSaveName() const { return SaveName; }

private:
	struct FActorInfo;
	struct FLevelInfo;
	struct FWorldInfo;

	void SerializeVersionOffset();

	/** Serializes information about the archive, like Map Name, and position of versioning information */
	void SerializeHeader();

	/**
	 * Serializes all the actors that the SaveGameSubsystem is keeping track of.
	 * On load, it will also pre-spawn any actors and map any actors with Spawn IDs
	 * before running the actual serialization step.
	 */
	void SerializeActors();

	void InitializeActor(int32 ActorIdx);
	void SerializeActor(int32 ActorIdx);

	void MergeSaveData();

	/** Serializes any destroyed level actors. On load, level actors will exist again, so this will re-destroy them */
	void SerializeDestroyedActors();

	/**
	 * Serialized at the end of the archive, the versions are useful for marshaling old data.
	 * These also contain the versions added by USaveGameFunctionLibrary::UseCustomVersion.
	 */
	void SerializeVersions();

	USaveGameSubsystem* Subsystem;
	TArray<uint8> Data;
	TSaveGameMemoryArchive Archive;
	TMap<FSoftObjectPath, FSoftObjectPath> Redirects;
	TSaveGameArchive<bIsLoading>* SaveArchive;

	FTopLevelAssetPath LevelAssetPath;
	TArray<uint64> ActorOffsets;
	TArray<TWeakObjectPtr<AActor>> SaveGameActors;
	TArray<FActorInfo> ActorData;
	TMap<FGuid, TWeakObjectPtr<AActor>> SpawnIDs;

	FString LastVisitedMap;
	uint64 ActorOffsetsOffset;
	uint64 VersionOffset;
	uint64 ActorsOffset;

	FString SaveName;
};
