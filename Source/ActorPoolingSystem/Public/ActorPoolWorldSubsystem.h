// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PoolTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolWorldSubsystem.generated.h"

/**
 * 
 */

UCLASS()
class ACTORPOOLINGSYSTEM_API UActorPoolWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

private:

	/* Soft Object Pointer to a Data Table containing default actor pool data that can be used on setup */
	static const TSoftObjectPtr<UDataTable> DefaultActorPoolDataTable;
	
	TMap<UClass*, TSharedPtr<FActorPool>> PoolMap;

	TMap<UClass*, FPooledActorSettings> ActorSettingsMap;

protected:

	UPROPERTY()
	int DefaultPoolSize = 10;

	UPROPERTY()
	int DefaultMinimumPoolSize = 5;

	UPROPERTY()
	int DefaultMaximumPoolSize = 20;

	UPROPERTY()
	FVector PoolingLocation = FVector(0.f, 0.f, -10000.f);

// Subsystem overrides
public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

// End of Subsystem overrides

public:

	// Static helper function for getting the actor pool world subsystem for our world
	UFUNCTION()
	static UActorPoolWorldSubsystem* GetActorPoolWorldSubsystem(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	void SetupActorPoolDefaults();
	
	template<class T>
	T* RequestActorFromPool(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData)
	{
		return Cast<T>(RequestActorFromPool(ActorClass, PopData));
	}

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	AActor* RequestActorFromPool(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	TArray<AActor*> RequestActorsFromPool(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	bool AddActorToPool(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	bool CreatePool(TSubclassOf<AActor> ActorClass, int MinimumPoolSize = 5, int MaximumPoolSize = 10, int Amount = 10);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	bool RemovePool(TSubclassOf<AActor> ActorClass);

	UFUNCTION(BlueprintCallable, Category = "Actor Pool World Subsystem")
	bool ModifyPoolMinimumSize(TSubclassOf<AActor> ActorClass, const int NewMinimumPoolSize);

private:

	UFUNCTION()
	AActor* PopActorOfType(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData);

	UFUNCTION()
	AActor* ForceSpawnActor(TSubclassOf<AActor> ActorClass) const;

	UFUNCTION()
	static bool IsValidActorClass(const TSubclassOf<AActor>& ActorClass);

	UFUNCTION()
	void FillPool(UClass* Class, FActorPool& ActorPool, const int ActorSpawnAmount) const;

	UFUNCTION()
	void ReleaseFromPool(FActorPool& ActorPool, const int ActorRemoveAmount);

	UFUNCTION()
	void OnActorLeftPool(AActor* Actor, const FActorPopData& PopData) const;

	UFUNCTION()
	void OnActorEnteredPool(AActor* Actor) const;
	
};
