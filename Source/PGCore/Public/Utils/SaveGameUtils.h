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
			T::SlotName
		} -> std::convertible_to<FString>;
		{
			SaveGame.GetVersion()
		} -> std::convertible_to<int32>;

		{
			T::CurrentVersion
		} -> std::convertible_to<int32>;
	};

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
	inline T* CreateSaveGameInstance()
	{
		return Cast<T>(UGameplayStatics::CreateSaveGameObject(T::StaticClass()));
	}

	template<SaveGameConcept T>
	void SaveGame(T* SaveGame)
	{
		if (!SaveGame)
		{
			UE_LOG(LogTemp, Warning, TEXT("SaveGame is NULL"));
			return;
		}

		UGameplayStatics::AsyncSaveGameToSlot(SaveGame, T::SlotName, 0);
	}

	template<SaveGameConcept T>
	bool DeleteSavedGame()
	{
		if (!DoesSaveGameExist<T>())
		{
			return false;
		}

		UE_LOG(LogTemp, Display, TEXT("Deleting Saved Game %s:%d ..."), *T::SlotName, 0);

		bool bWasDeleted = UGameplayStatics::DeleteGameInSlot(T::SlotName, 0);

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
	inline bool DoesSaveGameExist()
	{
		return UGameplayStatics::DoesSaveGameExist(T::SlotName, 0);
	}

	template<SaveGameConcept T>
	T* GetSavedGame()
	{
		if (!DoesSaveGameExist<T>())
		{
			return nullptr;
		}

		T* LoadedGame = Cast<T>(UGameplayStatics::LoadGameFromSlot(T::SlotName, 0));

		if (!LoadedGame)
		{
			UE_LOG(LogTemp, Error, TEXT("Unable to load saved game: %s"), *T::SlotName);
			return nullptr;
		}

		if (LoadedGame->GetVersion() != T::CurrentVersion)
		{
			UE_LOG(LogTemp, Warning, TEXT("Saved game version=%d is incompatible with current version=%d; SlotName=%s"),
				LoadedGame->GetVersion(), T::CurrentVersion,
				*T::SlotName
			);
			return nullptr;
		}

		return LoadedGame;
	}
}
