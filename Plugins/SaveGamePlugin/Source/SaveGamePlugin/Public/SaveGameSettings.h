// Copyright Alex Stevens (@MilkyEngineer). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SaveGameSettings.generated.h"

USTRUCT(BlueprintType, BlueprintInternalUseOnly)
struct FSaveGameVersionInfo
{
	GENERATED_BODY()

public:
	FSaveGameVersionInfo()
		: ID(FGuid::NewGuid())
		  , Enum(nullptr)
	{
	}

	/** A unique ID for this version, used by the Custom Version Container in a save game archive. Do not change! */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay)
	FGuid ID;

	/** The enum to use for versioning. System will use last value as the "latest version" number. Do not change! */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UEnum> Enum;
};

/**
 * Manages save game-specific settings including versioning and debug options.
 * Derived from UDeveloperSettings to allow configuration through project settings.
 */
UCLASS(DefaultConfig, Config = Game, DisplayName = "Save System")
class SAVEGAMEPLUGIN_API USaveGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

	/**
	 * Retrieves the unique identifier (GUID) associated with a specific versioning enum.
	 *
	 * This function ensures synchronization when accessing the cached version mapping
	 * and computes the mapping of enums to their respective GUIDs if not already cached.
	 *
	 * @param VersionEnum The enum used for versioning. It should represent valid versioning data.
	 * @return The GUID associated with the specified version enum. Returns an invalid GUID if
	 *         the enum is not found or mapping is not possible.
	 */
public:
	/** Get the current project version ID */
	FGuid GetVersionId(const UEnum* VersionEnum) const;

	/** Determines whether debug information will be printed. Can be configured to enable or disable debug logs for diagnostics and development purposes. */
	UPROPERTY(EditAnywhere, Config, Category = "Debug")
	bool bPrintDebug = true;

	/** Enables or disables the auto-save timer functionality */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "AutoSave", meta = (InlineEditConditionToggle))
	bool bEnableAutoSaveTimer = false;

	/** The number of autosave slots (default is 3) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "AutoSave", meta = (EditCondition = "bEnableAutoSaveTimer"))
	int32 NumAutosaveSlots = 3;

	/** Timer interval in seconds for auto-save (default 300 seconds, i.e., 5 minutes) */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, DisplayName ="AutoSave (Timer (s))", Category = "AutoSave", meta = (EditCondition = "bEnableAutoSaveTimer"))
	float AutoSaveInterval = 60;

	/** The slot name to use when automatically saving/loading on map enter/exit
	 *  The system will append "_#" based on the amount of autosave slots.
	 */
	UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "AutoSave", meta = (EditCondition = "bEnableAutoSaveTimer"))
	FString AutoSaveSlotName = TEXT("Autosave");

#if WITH_EDITOR
	/**
	 * Handles changes made to properties in the editor.
	 * Overrides the base class implementation to provide custom behavior when
	 * specific properties are modified, ensuring thread safety and resetting
	 * cached version data when necessary.
	 *
	 * @param PropertyChangedEvent Information about the property that was changed,
	 *                             including which property was modified.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/**
	 * A collection of versioning information that maps enumerations to unique IDs for use in save game archives.
	 * Each entry in this array corresponds to a version entry, where the version enum defines the versioning system,
	 * and its associated unique ID is used by the Custom Version Container.
	 */
	UPROPERTY(EditAnywhere, Config, Category=Version)
	TArray<FSaveGameVersionInfo> Versions;

private:
	mutable FCriticalSection VersionsSection;
	/**
	 * A mutable map that caches associations between versioning enums and their unique IDs.
	 * This is used internally to improve performance by avoiding redundant lookups or recalculations
	 * of versioning relationships during runtime.
	 * Thread safety is ensured using the VersionsSection critical section.
	 */
	mutable TMap<TObjectPtr<UEnum>, FGuid> CachedVersions;
};
