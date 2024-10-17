// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.


#include "PGGameInstance.h"

#include "PaperGolfLogging.h"
#include "Logging/LoggingUtils.h"
#include "Utils/VisualLoggerUtils.h"
#include "VisualLogger/VisualLogger.h"

#include "Input/InputCharacteristics.h"

#include "MultiplayerSessionsSubsystem.h"

#include "MoviePlayer.h"

#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "GenericPlatform/GenericApplication.h" 
#include "SlateBasics.h"

#include "Settings/PGGameUserSettings.h"
#include "Settings/PGAudioOptionSettings.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"

#include "Kismet/GameplayStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(PGGameInstance)


namespace
{
	constexpr int32 BuildUniqueId = -1443884023;
}

void UPGGameInstance::Init()
{

// start single session recording when not in editor mode - in shipping builds this will be a no-op so only applies to development builds
#if !WITH_EDITOR
	PG::VisualLoggerUtils::StartAutomaticRecording(this);
#endif

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Init"), *GetName());

	Super::Init();

	InitGamepadAvailable();
	InitLoadingScreen();
}

void UPGGameInstance::Shutdown()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: Shutdown"), *GetName());

	Super::Shutdown();

	// start single session recording when not in editor mode - in shipping builds this will be a no-op so only applies to development builds
#if !WITH_EDITOR
	PG::VisualLoggerUtils::StopAutomaticRecording(this);
#endif
}

#pragma region Loading Screen

void UPGGameInstance::InitLoadingScreen()
{
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::EndLoadingScreen);
}

void UPGGameInstance::BeginLoadingScreen(const FString& MapName)
{
	if (IsRunningDedicatedServer())
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: BeginLoadingScreen: %s - Skip since on dedicated server"), *GetName(), *MapName);
		return;
	}

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: BeginLoadingScreen: %s"), *GetName(), *MapName);

	DoLoadingScreen();
}

void UPGGameInstance::EndLoadingScreen(UWorld* InLoadedWorld)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: EndLoadingScreen: %s"), *GetName(), *LoggingUtils::GetName(InLoadedWorld));

	InitSoundVolumes();
}

void UPGGameInstance::DoLoadingScreen()
{
	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = false;
	LoadingScreen.bAllowInEarlyStartup = true;
	LoadingScreen.WidgetLoadingScreen = FLoadingScreenAttributes::NewTestLoadingScreenWidget();

	GetMoviePlayer()->SetupLoadingScreen(LoadingScreen);
}

#pragma endregion Loading Screen

#pragma region Gamepad Support

void UPGGameInstance::InitGamepadAvailable()
{
	/* See:
	https://couchlearn.com/how-to-use-the-game-instance-in-unreal-engine-4/
	https://github.com/Noesis/UE4-ShooterGame/blob/master/Source/ShooterGame/Public/ShooterGameInstance.h
	https://github.com/Noesis/UE4-ShooterGame/blob/master/Source/ShooterGame/Private/ShooterGameInstance.cpp
	*/

	auto& PlatformInputDeviceMapper = IPlatformInputDeviceMapper::Get();
	PlatformInputDeviceMapper.GetOnInputDeviceConnectionChange().AddUObject(this, &ThisClass::HandleControllerConnectionChange);
	PlatformInputDeviceMapper.GetOnInputDevicePairingChange().AddUObject(this, &ThisClass::HandleControllerPairingChanged);

	// See https://answers.unrealengine.com/questions/142358/question-is-there-a-way-to-detect-a-gamepad.html?childToView=706040#answer-706040
	// for a solution as ControllerId will be 0 for player 1 regardless if the "controller" is a gamepad and the connection change only fires if connecting after game starts

	// See also https://answers.unrealengine.com/questions/463722/how-do-you-detect-a-second-gamepad-for-splitscreen.html
	// also https://answers.unrealengine.com/questions/291285/index.html

	// See  https://answers.unrealengine.com/questions/142358/view.html for below

	auto genericApplication = FSlateApplication::Get().GetPlatformApplication();
	bool bGamepadAvailable = genericApplication.IsValid() && genericApplication->IsGamepadAttached();

	PG::FInputCharacteristics::SetGamepadAvailable(bGamepadAvailable);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: InitGamepadAvailable - controller gamepad available=%s"), *GetName(), LoggingUtils::GetBoolString(bGamepadAvailable));
}

void UPGGameInstance::HandleControllerConnectionChange(EInputDeviceConnectionState InputDeviceConnectionState, FPlatformUserId UserId, FInputDeviceId ControllerId)
{
	const bool bConnected = InputDeviceConnectionState == EInputDeviceConnectionState::Connected;

	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: HandleControllerConnectionChange - bConnected=%s;InputDeviceConnectionState=%d;UserId=%d;ControllerId=%d"),
		*GetName(), LoggingUtils::GetBoolString(bConnected), InputDeviceConnectionState, UserId.GetInternalId(), ControllerId.GetId());

	PG::FInputCharacteristics::SetGamepadAvailable(bConnected);
}

void UPGGameInstance::HandleControllerPairingChanged(FInputDeviceId ControllerId, FPlatformUserId NewUserId, FPlatformUserId OldUserId)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Display, TEXT("%s: HandleControllerPairingChanged - bConnected=TRUE; ControllerIndex=%d;NewUserId=%d;OldUserId=%d"),
		*GetName(), ControllerId.GetId(), NewUserId.GetInternalId(), OldUserId.GetInternalId());

	PG::FInputCharacteristics::SetGamepadAvailable(true);
}

#pragma endregion Gamepad Support

#pragma region Audio Settings

void UPGGameInstance::InitSoundVolumes()
{
	const auto Settings = UPGGameUserSettings::GetInstance();
	if (!ensureMsgf(Settings, TEXT("UPGGameUserSettings was NULL")))
	{
		return;
	}

	if (!ensureMsgf(AudioOptionSettings, TEXT("AudioOptionSettings is not set")))
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: AudioOptionSettings is not set"), *GetName());

		return;
	}

	if (AudioOptionSettings->SfxSoundClass)
	{
		ApplyMixToSoundClass(AudioOptionSettings->SfxSoundClass, Settings->GetSfxVolume());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No SfxSoundClass set on %s!"), *GetName(), *LoggingUtils::GetName(AudioOptionSettings));
	}

	if (AudioOptionSettings->MusicSoundClass)
	{
		ApplyMixToSoundClass(AudioOptionSettings->MusicSoundClass, Settings->GetMusicVolume());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No MusicSoundClass set on %s!"), *GetName(), *LoggingUtils::GetName(AudioOptionSettings));
	}

	if (AudioOptionSettings->AmbienceSoundClass)
	{
		ApplyMixToSoundClass(AudioOptionSettings->AmbienceSoundClass, Settings->GetAmbienceVolume());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No AmbienceSoundClass set on %s!"), *GetName(), *LoggingUtils::GetName(AudioOptionSettings));
	}

	if (AudioOptionSettings->MainSoundClass)
	{
		ApplyMixToSoundClass(AudioOptionSettings->MainSoundClass, Settings->GetMainVolume());
	}
	else
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No MainSoundClass set on %s!"), *GetName(), *LoggingUtils::GetName(AudioOptionSettings));
	}
}

void UPGGameInstance::ApplyMixToSoundClass(USoundClass* SoundClass, float Volume)
{
	if (!AudioOptionSettings)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: AudioOptionSettings is not set"), *GetName());

		return;
	}

	if (!AudioOptionSettings->VolumeChangeMix)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Error, TEXT("%s: No VolumeChangeMix set on %s!"), *GetName(), *LoggingUtils::GetName(AudioOptionSettings));
		return;
	}

	if (!ensure(SoundClass))
	{
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(GetWorld(), AudioOptionSettings->VolumeChangeMix, SoundClass, Volume, 1.0f, 0.0f, true);
	UGameplayStatics::PushSoundMixModifier(GetWorld(), AudioOptionSettings->VolumeChangeMix);

	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Changed volume of %s to %f"), *GetName(), *SoundClass->GetName(), Volume);
}

#if WITH_EDITOR

FGameInstancePIEResult UPGGameInstance::StartPlayInEditorGameInstance(ULocalPlayer* LocalPlayer, const FGameInstancePIEParameters& Params)
{
	auto Result = Super::StartPlayInEditorGameInstance(LocalPlayer, Params);

	InitSoundVolumes();

	return Result;
}

#endif

#pragma endregion Audio Settings

void UPGGameInstance::InitMultiplayerSessionsSubsystem()
{
	auto Subsystem = GetSubsystem<UMultiplayerSessionsSubsystem>();
	if (!ensure(Subsystem))
	{
		return;
	}

	Subsystem->SetBuildId(BuildUniqueId);
}

void UPGGameInstance::ReturnToMainMenu()
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: ReturnToMainMenu"), *GetName());

	Super::ReturnToMainMenu();

	// Redundant with OnWorldChanged but we cannot rely on just destroying the session here as ReturnToMainMenu only called if server ends the game
	// If we quit from the pause menu then the MainMenu world is just loaded and so this function never gets called
	// To handle all cases, we need to check if the MainMenu world is loading
	// DestroyAnyActiveOnlineSession();
}

void UPGGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: OnWorldChanged: OldWorldMap=%s;NewWorldMap=%s"), *GetName(), 
		OldWorld ? *OldWorld->GetMapName() : TEXT("NULL"),
		NewWorld ? *NewWorld->GetMapName() : TEXT("NULL"));

	Super::OnWorldChanged(OldWorld, NewWorld);

	if (!NewWorld)
	{
		return;
	}

	// Hack: Ensure that the multiplayer session subsystem is properly destroyed when we return to the main menu

	FString MapName = NewWorld->GetMapName();
	MapName.RemoveFromStart(NewWorld->StreamingLevelsPrefix); // Remove any prefix if necessary

	// How to strip off the name of the map from the path?
	if (MapName == MainMenuMapName)
	{
		UE_VLOG_UELOG(this, LogPaperGolfGame, Log, TEXT("%s: Detected change to MainMenu map=%s - Destroying any active online session"), *GetName(), *MainMenuMapName);

		DestroyAnyActiveOnlineSession();
	}
}

void UPGGameInstance::DestroyAnyActiveOnlineSession()
{
	// Make sure we clean up any active game session when a multiplayer game ends before potentially joining a new one
	// Technically there is a race condition between when the session is actually destroyed and when we can join/host another game; however, it will take time for player to make a decisino
	// so this will rarely happen in practice
	// If it is still an issue we can revisit this later
	if (auto Subsystem = GetSubsystem<UMultiplayerSessionsSubsystem>(); Subsystem && Subsystem->IsSessionActive())
	{
		Subsystem->DestroySession();
	}
}