// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "PaperGolfTypes.generated.h"

UENUM(BlueprintType)
enum class EShotType : uint8
{
	Default	UMETA(Hidden),
	Full,
	Medium,
	Close,
	MAX UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EShotFocusType : uint8
{
	Hole,
	Focus
};

UENUM(BlueprintType)
enum class EHoleStartType : uint8
{
	Default	UMETA(Hidden),
	Start,
	InProgress,
	MAX UMETA(Hidden)
};

USTRUCT()
struct FShotFocusScores
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<AActor> FocusActor{};

	float Score{};
};

USTRUCT(BlueprintType)
struct PGPAWN_API FFlickParams
{
	GENERATED_BODY()

	// Must be marked uproperty in order to replicate in an RPC call

	UPROPERTY(Transient, BlueprintReadWrite)
	EShotType ShotType{};

	UPROPERTY(Transient, BlueprintReadWrite)
	float LocalZOffset{};

	UPROPERTY(Transient, BlueprintReadWrite)
	float PowerFraction{ 1.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float Accuracy{ 0.0f };

	void Clamp();

	FString ToString() const;
};

USTRUCT(BlueprintType)
struct PGPAWN_API FFlickPredictParams
{
	GENERATED_BODY()

	UPROPERTY(Transient, BlueprintReadWrite)
	float MaxSimTime{ 30.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float SimFrequency{ 30.0f };

	UPROPERTY(Transient, BlueprintReadWrite)
	float CollisionRadius{ 3.0f };

	UPROPERTY(TRansient, BlueprintReadWrite)
	FRotator AdditionalWorldRotation{ EForceInit::ForceInitToZero };
};

bool operator ==(const FFlickParams& First, const FFlickParams& Second);

#pragma region Inline Definitions

FORCEINLINE bool operator ==(const FFlickParams& First, const FFlickParams& Second)
{
	return First.ShotType == Second.ShotType &&
		FMath::IsNearlyEqual(First.LocalZOffset, Second.LocalZOffset) &&
		FMath::IsNearlyEqual(First.PowerFraction, Second.PowerFraction) &&
		FMath::IsNearlyEqual(First.Accuracy, Second.Accuracy);
}

#pragma endregion Inline Definitions
