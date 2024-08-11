// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "Interfaces/FocusableActor.h"
#include "GameFramework/Actor.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "GolfHole.generated.h"

class APaperGolfPawn;
class APaperGolfGameStateBase;

UENUM(BlueprintType)
enum class EGolfHoleState : uint8
{
	None,
	Active,
	Inactive
};

UCLASS()
class PGGAMEPLAY_API AGolfHole : public AActor, public IFocusableActor, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()
	
public:	
	AGolfHole();


#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

	UFUNCTION(BlueprintPure, Category = "Hole", meta = (DefaultToSelf = "WorldContextObject"))
	static AGolfHole* GetCurrentHole(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "Hole", meta = (DefaultToSelf = "WorldContextObject"))
	static TArray<AGolfHole*> GetAllWorldHoles(const UObject* WorldContextObject, bool bSort = true);

	virtual void Reset() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty >& OutLifetimeProps) const override;

protected:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetCollider(UPrimitiveComponent* InCollider);

	UFUNCTION(BlueprintImplementableEvent)
	void BlueprintOnActiveHoleChanged(bool bIsActiveHole);

private:

	int32 GetHoleNumber_Implementation() const;
	bool IsHole_Implementation() const { return true; }

	UFUNCTION()
	void OnComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void OnCheckScored();

	bool CheckedScored() const;

	void ClearTimer();

	void UpdateColliderRegistration();

	void RegisterCollider();
	void UnregisterCollider();

	UFUNCTION()
	void OnRep_GolfHoleState();

	void OnActiveHoleChanged();

private:
	FTimerHandle CheckScoredTimerHandle{};

	TWeakObjectPtr<APaperGolfPawn> OverlappingPaperGolfPawn{};

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> Collider{};

	UPROPERTY(EditAnywhere, Category = "Config")
	int32 HoleNumber{};

	// Set to true initially as by default the hole is active so need to detect the flipped condition
	UPROPERTY(ReplicatedUsing = OnRep_GolfHoleState)
	EGolfHoleState GolfHoleState{};
};

#pragma region Inline Definitions

FORCEINLINE int32 AGolfHole::GetHoleNumber_Implementation() const
{
	ensureAlwaysMsgf(HoleNumber > 0, TEXT("%s: HoleNumber is not set"), *GetName());

	return HoleNumber;
}

#pragma endregion Inline Definitions
