// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "Interfaces/MultiplayerMenu.h"

#include "Menu.generated.h"


class UMultiplayerMenuHelper;

class UButton;
class UCheckBox;
class UEditableText;
class UComboBoxString;
/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget, public IMultiplayerMenu
{
	GENERATED_BODY()

public:

	virtual void MenuSetup_Implementation(const TMap<FString, FString>& MatchTypesToDisplayMap, const TArray<FString>& Maps, const FString& LobbyPath,
		int32 MinPlayers = 2, int32 MaxPlayers = 4, int32 DefaultNumPlayers = 2, bool bDefaultLANMatch = true, bool bDefaultAllowBots = false) override;

protected:

	virtual bool Initialize() override;

	// OnLevelRemovedFromWorld was removed in Unreal 5.1, we can use "NativeDestruct" instead to do the equivalent cleanup
	virtual void NativeDestruct() override;

	virtual void HostButtonClicked_Implementation() override;

	virtual void JoinButtonClicked_Implementation() override;

	virtual void LanMatchChanged_Implementation(bool bIsChecked) override;

private:
	void MenuTeardown();

private:

	UPROPERTY(Transient)
	TObjectPtr<UMultiplayerMenuHelper> MenuHelper{};

	// Button variable in widget blueprint is bound also in C++ - name needs to match exactly
	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> BtnHost{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> BtnJoin{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UCheckBox> ChkLanMatch{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UCheckBox> ChkAllowBots{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UEditableText> TxtLanIpAddress{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> CboAvailableMatchTypes{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> CboMaxNumberOfPlayers{};

	UPROPERTY(Category = "Menu", BlueprintReadOnly, meta = (BindWidget, AllowPrivateAccess = "true"))
	TObjectPtr<UComboBoxString> CboAvailableMaps{};
};
