// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "PoolTypes.generated.h"

/**
 * 
 */

/* Bitmask Flags we can set for settings we want to toggle for our pooled actor when leaving the pool */
UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EPooledActorToggles: uint8
{
	None				= 0 UMETA(Hidden),
	Tick				= 1 << 0,
	Replicates			= 1 << 1,
	HiddenInGame		= 1 << 2,
	CollisionEnabled	= 1 << 3,
};
ENUM_CLASS_FLAGS(EPooledActorToggles);

USTRUCT(BlueprintType)
struct FPooledActorSettings : public FTableRowBase
{
	GENERATED_BODY()

	// Default collision enabled on as this will be generally used by most pooled actors, but keep the rest off unless specified
	FPooledActorSettings()
	{
		QualityFlags = 0;
		QualityFlags |= static_cast<uint8>(EPooledActorToggles::CollisionEnabled);
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> PooledActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = EPooledActorToggles))
	int32 QualityFlags;

	bool ShouldUseTick() const { return QualityFlags & static_cast<uint8>(EPooledActorToggles::Tick); }
	bool ShouldReplicate() const { return QualityFlags & static_cast<uint8>(EPooledActorToggles::Replicates); }
	bool ShouldHideInGame() const { return QualityFlags & static_cast<uint8>(EPooledActorToggles::HiddenInGame); }
	bool ShouldEnableCollision() const { return QualityFlags & static_cast<uint8>(EPooledActorToggles::CollisionEnabled); }
};

USTRUCT(BlueprintType)
struct FDefaultActorPoolData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> ActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MinimumPoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int MaximumPoolSize;
};

USTRUCT(BlueprintType)
struct FActorPopData
{
	GENERATED_BODY()
	
	virtual ~FActorPopData() = default;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	AActor* Owner;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	APawn* Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	FVector Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	FRotator Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	const UObject* OptionalObject;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	const UObject* OptionalObject2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	FGameplayTagContainer OptionalGameplayTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	float Magnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool Pop Data")
	float OptionalMagnitude;

	virtual APawn* GetInstigator() const
	{
		return Instigator;
	}

	virtual AActor* GetOwner() const
	{
		return Owner;
	}

	virtual FVector GetLocation() const
	{
		return Location;
	}

	virtual FRotator GetRotator() const
	{
		return Rotation;
	}
};

USTRUCT(BlueprintType)
struct FActorPool
{
	GENERATED_BODY()

	FActorPool()
	{
		MinimumPoolSize = 1;
		MaximumPoolSize = 10;
	}

	FActorPool(int InMinimumPoolSize, int InMaximumPoolSize)
	{
		MinimumPoolSize = InMinimumPoolSize;
		MaximumPoolSize = InMaximumPoolSize;
	}

	virtual ~FActorPool()
	{
		UE_LOG(LogTemp, Display, TEXT("Actor Pool is being destroyed"));
	}


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool")
	TArray<AActor*> Pool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool")
	int MinimumPoolSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Actor Pool")
	int MaximumPoolSize;

	bool ShouldGrow() const;

	bool CanGrow() const;

	bool CanShrink() const;

	void Push(AActor* Actor);

	AActor* Pop();

	int Num() const;

	bool ContainsActor(AActor* Actor) const;

};
