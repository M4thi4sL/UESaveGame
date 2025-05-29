// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#include "SaveGameSubsystem.h"

#include "SaveGameFunctionLibrary.h"
#include "SaveGameObject.h"
#include "SaveGameSerializer.h"

#include "EngineUtils.h"
#include "SaveGameSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogSaveGameSubsystem, Log, All);

using namespace UE::Tasks;

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &USaveGameSubsystem::OnWorldInitialized);
	FWorldDelegates::OnWorldInitializedActors.AddUObject(this, &USaveGameSubsystem::OnActorsInitialized);
	FWorldDelegates::OnWorldCleanup.AddUObject(this, &USaveGameSubsystem::OnWorldCleanup);

	FWorldDelegates::LevelAddedToWorld.AddUObject(this, &USaveGameSubsystem::OnLevelAddedToWorld);
	FWorldDelegates::PreLevelRemovedFromWorld.AddUObject(this, &USaveGameSubsystem::OnPreLevelRemovedFromWorld);
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &USaveGameSubsystem::OnPreWorldDestroyed);

	/** Might wanne get the developer settings and cache them for easy lookup */
	SaveGameSettings = GetSaveGameSubsystemSettings();

	OnWorldInitialized(GetWorld(), UWorld::InitializationValues());

	/** Start the auto timer if autosaving is enabled */
	if (SaveGameSettings->bEnableAutoSaveTimer)
	{
		// TODO:  AutoSave();
	}
}

void USaveGameSubsystem::Deinitialize()
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	FWorldDelegates::OnWorldInitializedActors.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);

	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FWorldDelegates::PreLevelRemovedFromWorld.RemoveAll(this);
}

void USaveGameSubsystem::Save(FString SaveName)
{
	constexpr TCHAR RegionName[] = TEXT("SaveGame[Save]");
	UE_LOG(LogSaveGameSubsystem, Log, TEXT("%s: Begin"), RegionName);
	TRACE_BEGIN_REGION(RegionName);

	OnSaveStart.Broadcast(); // Notify save start

	TSharedPtr<TSaveGameSerializer<false>> Serializer = MakeShared<TSaveGameSerializer<false>>(this, SaveName);
	SaveGamePipe.Launch(UE_SOURCE_LOCATION, [this, Serializer]
	{
		FTask Previous = Serializer->DoOperation();

		AddNested(Launch(UE_SOURCE_LOCATION, [this, Serializer]() mutable
		{
			Serializer.Reset();
			TRACE_END_REGION(RegionName);
			UE_LOG(LogSaveGameSubsystem, Log, TEXT("%s: End"), RegionName);

			OnSaveDone.Broadcast(); // Notify save completion
		}, Previous));
	});
}

void USaveGameSubsystem::Load(FString SaveName)
{
	constexpr TCHAR RegionName[] = TEXT("SaveGame[Load]");
	UE_LOG(LogSaveGameSubsystem, Log, TEXT("%s: Begin"), RegionName);
	TRACE_BEGIN_REGION(RegionName);

	OnLoadStart.Broadcast();

	TSharedPtr<TSaveGameSerializer<true>> Serializer = MakeShared<TSaveGameSerializer<true>>(this, SaveName);

	/** update the timestamp of the last loaded savegame */
	SaveGamePipe.Launch(UE_SOURCE_LOCATION, [this, Serializer]
	{
		FTask Previous = Serializer->DoOperation();
		AddNested(Launch(UE_SOURCE_LOCATION, [this, Serializer]() mutable
		{
			Serializer.Reset();
			TRACE_END_REGION(RegionName);
			UE_LOG(LogSaveGameSubsystem, Log, TEXT("%s: End"), RegionName);

			OnLoadDone.Broadcast();
		}, Previous));
	});
}

void USaveGameSubsystem::OnPreWorldDestroyed(UWorld* World)
{
	if (!IsValid(World) || GetWorld() != World)
	{
		return;
	}
	SaveGameActors.Reset();
	DestroyedLevelActors.Reset();
}

void USaveGameSubsystem::OnLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	if (SaveGameSettings->bPrintDebug) UE_LOG(LogSaveGameSubsystem, Log, TEXT("level '%s' added to world '%s'"),
	                                          (Level->GetOuter() ? *Level->GetOuter()->GetName() : TEXT("NULL")),
	                                          (World ? *World->GetName() : TEXT("NULL")));

	if (!IsValid(Level) || !IsValid(World) || GetWorld() != World)
	{
		return;
	}

	/** We should check the archive in memory and update the actors part of the level thats being added with the loaded save data */

	/** Then we check the level for actors we want to save for later reference */
	for (AActor* Actor : Level->Actors)
	{
		OnActorPreSpawn(Actor);
	}
}

void USaveGameSubsystem::OnPreLevelRemovedFromWorld(ULevel* Level, UWorld* World)
{
	if (SaveGameSettings->bPrintDebug) UE_LOG(LogSaveGameSubsystem, Log, TEXT("level '%s' Removed from world '%s'"),
	                                          (Level->GetOuter() ? *Level->GetOuter()->GetName() : TEXT("NULL")),
	                                          (World ? *World->GetName() : TEXT("NULL")));

	if (!IsValid(Level) || !IsValid(World) || GetWorld() != World)
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		if (IsValid(Actor))
		{
			SaveGameActors.Remove(Actor);
		}
	}
}

bool USaveGameSubsystem::IsLoadingSaveGame() const
{
	return SaveGamePipe.HasWork();
}

void USaveGameSubsystem::OnWorldInitialized(UWorld* World, const UWorld::InitializationValues)
{
	if (!IsValid(World) || GetWorld() != World)
	{
		return;
	}

	World->AddOnActorPreSpawnInitialization(
		FOnActorSpawned::FDelegate::CreateUObject(this, &ThisClass::OnActorPreSpawn));
	World->AddOnActorDestroyedHandler(FOnActorDestroyed::FDelegate::CreateUObject(this, &ThisClass::OnActorDestroyed));
}

void USaveGameSubsystem::OnActorsInitialized(const FActorsInitializedParams& Params)
{
	if (SaveGameSettings->bPrintDebug) UE_LOG(LogSaveGameSubsystem, Log,
	                                          TEXT("Actors have been initialized in world: %s"),
	                                          *Params.World->GetName());

	if (!IsValid(Params.World) || GetWorld() != Params.World)
	{
		return;
	}

	for (TActorIterator<AActor> It(Params.World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->Implements<USaveGameObject>())
		{
			SaveGameActors.Add(Actor);
		}
	}
}

void USaveGameSubsystem::OnWorldCleanup(UWorld* World, bool, bool)
{
	if (!IsValid(World) || GetWorld() != World)
	{
		return;
	}

	SaveGameActors.Reset();
	DestroyedLevelActors.Reset();
}

void USaveGameSubsystem::OnActorPreSpawn(AActor* Actor)
{
	if (IsValid(Actor) && Actor->Implements<USaveGameObject>())
	{
		SaveGameActors.Add(Actor);
	}
}

void USaveGameSubsystem::OnActorDestroyed(AActor* Actor)
{
	SaveGameActors.Remove(Actor);

	if (USaveGameFunctionLibrary::WasObjectLoaded(Actor))
	{
		DestroyedLevelActors.Add(Actor);
	}
}

USaveGameSettings* USaveGameSubsystem::GetSaveGameSubsystemSettings()
{
	USaveGameSettings* Settings = GetMutableDefault<USaveGameSettings>();
	if (Settings == nullptr)
	{
		UE_LOG(LogSaveGameSubsystem, Error, TEXT("Failed to get developer settings CDO"));
	}
	return Settings;
}
