// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerVolume.h"
#include "BasePaperGolfVolume.generated.h"

class UGolfEventsSubsystem;
class APaperGolfPawn;
class UOverlapConditionComponent;


UENUM(BlueprintType)
enum class EPaperGolfVolumeOverlapType : uint8
{
	/*
	* Condition triggers on any overlap with the volume.
	*/
	Any,

	/*
	* Condition only triggers when the player ends their turn still in the volume.
	*/
	End
};

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType, ShowCategories = (Brush))
class PGGAMEPLAY_API ABasePaperGolfVolume : public ATriggerVolume
{
	GENERATED_BODY()

public:
	ABasePaperGolfVolume();

protected:

	virtual void OnConditionTriggered(APaperGolfPawn& PaperGolfPawn, UGolfEventsSubsystem& GolfEvents) {}

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Condition Triggered"))
	void ReceiveConditionTriggered(APaperGolfPawn* PaperGolfPawn, UGolfEventsSubsystem* GolfEvents);

	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintNativeEvent, Category = "Condition")
	bool CheckEndCondition(const APaperGolfPawn* PaperGolfPawn) const;

	virtual bool CheckEndCondition_Implementation(const APaperGolfPawn* PaperGolfPawn) const { return true;  }

private:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override final;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override final;

	void NotifyConditionTriggered(APaperGolfPawn& PaperGolfPawn);
	bool IsConditionTriggered(const APaperGolfPawn& PaperGolfPawn) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Overlap Type")
	EPaperGolfVolumeOverlapType Type{ EPaperGolfVolumeOverlapType::Any };

private:

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UOverlapConditionComponent> OverlapConditionComponent{};
};

#pragma region Inline Definitions

FORCEINLINE bool ABasePaperGolfVolume::IsConditionTriggered(const APaperGolfPawn& PaperGolfPawn) const
{
	return CheckEndCondition(&PaperGolfPawn);
}

#pragma endregion Inline Definitions
