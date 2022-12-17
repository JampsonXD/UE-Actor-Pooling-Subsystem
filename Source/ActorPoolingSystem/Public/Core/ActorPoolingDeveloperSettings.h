// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "ActorPoolingDeveloperSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Actor Pooling System Settings"))
class ACTORPOOLINGSYSTEM_API UActorPoolingDeveloperSettings : public UDeveloperSettings
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Default Actor Pools")
	TArray<TSoftObjectPtr<UDataTable>> DefaultActorPoolPaths;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Actor Pool Settings")
	TArray<TSoftObjectPtr<UDataTable>> ActorPoolSettingsPaths;
	
};
