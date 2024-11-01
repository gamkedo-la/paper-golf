// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/GameplayStatics.h" 

#include <concepts>

namespace SaveGameUtils
{
	template<typename T>
	concept SaveGameConcept = requires(T SaveGame)
	{
		{
			SaveGame.SlotName
		} -> std::convertible_to<FString>;

		{
			SaveGame.SlotIndex
		} -> std::convertible_to<int32>;

		{
			SaveGame.GetVersion()
		} -> std::convertible_to<int32>;

		{
			T::CurrentVersion
		} -> std::convertible_to<int32>;
	};

	template<SaveGameConcept T>
	const T* CreateSaveGameTemplateInstance();

	template<SaveGameConcept T>
	T* CreateSaveGameInstance();

	template<SaveGameConcept T>
	void SaveGame(T* SaveGame);

	template<SaveGameConcept T>
	bool DeleteSavedGame();

	template<SaveGameConcept T>
	T* GetSavedGame();

	template<SaveGameConcept T>
	bool CanLoadGame();

	template<SaveGameConcept T>
	bool DoesSaveGameExist();
}


// Template Definitions

namespace SaveGameUtils
{
	template<SaveGameConcept T>
	inline const T* CreateSaveGameTemplateInstance()
	{
		return CreateSaveGameInstance<T>();
	}

	template<SaveGameConcept T>
	inline T* CreateSaveGameInstance()
	{
		return Cast<T>(UGameplayStatics::CreateSaveGameObject(T::StaticClass()));
	}

	template<SaveGameConcept T>
	void SaveGameUtils::SaveGame(T* SaveGame)
	{
		if (!SaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("SaveGame is NULL"));
			return;
		}

		UGameplayStatics::AsyncSaveGameToSlot(SaveGame, SaveGame->SlotName, SaveGame->SlotIndex);
	}

	template<SaveGameConcept T>
	bool DeleteSavedGame()
	{
		if (!DoesSaveGameExist<T>())
		{
			return false;
		}

		const T* const SaveGameMetadata = CreateSaveGameTemplateInstance<T>();

		if (!SaveGameMetadata)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to create a saved game instance"));
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Deleting Saved Game %s:%d ..."), *SaveGameMetadata->SlotName, SaveGameMetadata->SlotIndex);

		bool bWasDeleted = UGameplayStatics::DeleteGameInSlot(SaveGameMetadata->SlotName, SaveGameMetadata->SlotIndex);

		if (bWasDeleted)
		{
			UE_LOG(LogTemp, Display, TEXT("Saved Game deleted successfully."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Saved Game could not be deleted."));
		}

		return bWasDeleted;
	}

	template<SaveGameConcept T>
	inline bool CanLoadGame()
	{
		return DoesSaveGameExist<T>() && GetSavedGame<T>() != nullptr;
	}

	template<SaveGameConcept T>
	bool DoesSaveGameExist()
	{
		const T* const SaveGameMetadata = CreateSaveGameTemplateInstance<T>();

		if (!SaveGameMetadata)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to create a saved game instance"));
			return false;
		}

		return UGameplayStatics::DoesSaveGameExist(SaveGameMetadata->SlotName, SaveGameMetadata->SlotIndex);
	}

	template<SaveGameConcept T>
	T* GetSavedGame()
	{
		if (!DoesSaveGameExist<T>())
		{
			return nullptr;
		}

		const T* const TemplateLoad = CreateSaveGameTemplateInstance<T>();

		if (!TemplateLoad)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to create a saved game instance"));
			return nullptr;
		}

		T* LoadedGame = Cast<T>(UGameplayStatics::LoadGameFromSlot(TemplateLoad->SlotName, TemplateLoad->SlotIndex));

		if (!LoadedGame)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to load saved game: SlotIndex=%d; SlotName=%s"), TemplateLoad->SlotIndex, *TemplateLoad->SlotName);
			return nullptr;
		}

		if (LoadedGame->GetVersion() != T::CurrentVersion)
		{
			UE_LOG(LogTemp, Warning, TEXT("Saved game version=%d is incompatible with current version=%d; SlotIndex=%d; SlotName=%s"),
				LoadedGame->GetVersion(), T::CurrentVersion,
				LoadedGame->SlotIndex, *LoadedGame->SlotName
			);
			return nullptr;
		}

		return LoadedGame;
	}
}
