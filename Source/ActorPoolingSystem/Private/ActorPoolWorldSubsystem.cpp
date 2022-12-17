// Fill out your copyright notice in the Description page of Project Settings.


#include "ActorPoolWorldSubsystem.h"
#include "PooledActorInterface.h"
#include "Core/ActorPoolingDeveloperSettings.h"


const TSoftObjectPtr<UDataTable> UActorPoolWorldSubsystem::DefaultActorPoolDataTable = 
	TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("DataTable'/ActorPoolingSystem/DT_DefaultPoolData.DT_DefaultPoolData'")));

bool UActorPoolWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UActorPoolWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UActorPoolWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UActorPoolWorldSubsystem* UActorPoolWorldSubsystem::GetActorPoolWorldSubsystem(const UObject* WorldContextObject)
{
	if(!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	return World ? World->GetSubsystem<UActorPoolWorldSubsystem>() : nullptr;
}

void UActorPoolWorldSubsystem::SetupActorPoolDefaults()
{
	// Get all of our soft object paths associated with our developer settings for default actor pools
	const TArray<TSoftObjectPtr<UDataTable>>& ActorPoolPaths =
		UActorPoolingDeveloperSettings::StaticClass()->GetDefaultObject<UActorPoolingDeveloperSettings>()->DefaultActorPoolPaths;


	/* For each path, load the object it is pointing to,
	 * get all rows that are a default actor pool data and create pools with the found rows */
	for(const TSoftObjectPtr<UDataTable>& Path: ActorPoolPaths)
	{
		if(const UDataTable* Table = Path.LoadSynchronous())
		{
			TArray<FDefaultActorPoolData*> PoolData;
			Table->GetAllRows("", PoolData);

			for(int i = 0; i < PoolData.Num(); ++i)
			{
				if(const FDefaultActorPoolData* Data = PoolData[i])
				{
					CreatePool(Data->ActorClass, Data->MinimumPoolSize, Data->MaximumPoolSize, Data->PoolSize);
				}
			}
		}
	}

	// Get all of our soft object paths associated with our developer settings for our actor pop settings
	const TArray<TSoftObjectPtr<UDataTable>>& SettingsPaths =
		UActorPoolingDeveloperSettings::StaticClass()->GetDefaultObject<UActorPoolingDeveloperSettings>()->ActorPoolSettingsPaths;

	/* For each of our found Actor Path Settings Tables, add the data to a map,
	 * mapping actor classes to the settings they will be configured to use when popping
	 * the actor out of the actor pool
	 */
	for(const TSoftObjectPtr<UDataTable>& Path : SettingsPaths)
	{
		if(const UDataTable* Table = Path.LoadSynchronous())
		{
			TArray<FPooledActorSettings*> SettingsData;
			Table->GetAllRows("", SettingsData);

			for(int i = 0; i < SettingsData.Num(); ++i)
			{
				// Find or Add the actor settings to our map of settings, overwrites actor settings that may have been set previously
				if(const FPooledActorSettings* ActorSettings = SettingsData[i])
				{
					ActorSettingsMap.FindOrAdd(ActorSettings->PooledActorClass, *ActorSettings);
				}
			}
		}
	}
}

AActor* UActorPoolWorldSubsystem::RequestActorFromPool(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData)
{
	if(!IsValidActorClass(ActorClass))
	{
		return nullptr;
	}

	if(AActor* Actor = PopActorOfType(ActorClass, PopData))
	{
		return Actor;
	}

	// We might not have had an actor available so force spawn and return one 
	if(PoolMap.Contains(ActorClass))
	{
		AActor* Actor = ForceSpawnActor(ActorClass);
		OnActorLeftPool(Actor, PopData);
		return Actor;
	}

	// Didn't contain a pool for the requested class, create a pool and return an actor of the pool is created
	CreatePool(ActorClass, DefaultMinimumPoolSize, DefaultPoolSize);
	return RequestActorFromPool(ActorClass, PopData);
}

TArray<AActor*> UActorPoolWorldSubsystem::RequestActorsFromPool(TSubclassOf<AActor> ActorClass,
	const FActorPopData& PopData)
{
	TArray<AActor*> NewArray;
	return NewArray;
}

bool UActorPoolWorldSubsystem::AddActorToPool(AActor* Actor)
{
	if(!Actor || !Actor->Implements<UPooledActorInterface>())
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor does not implement pooled actor interface and can not be added to a pool."))
		return false;
	}

	if(!PoolMap.Contains(Actor->GetClass()))
	{
		CreatePool(Actor->GetClass(), DefaultMinimumPoolSize, DefaultMaximumPoolSize, DefaultPoolSize);
	}

	if (const TSharedPtr<FActorPool>* PoolPointer = PoolMap.Find(Actor->GetClass()))
	{
		FActorPool* Pool = PoolPointer->Get();
		// Make sure our actor isn't already contained in the pool
		if(Pool->ContainsActor(Actor))
		{
			UE_LOG(LogTemp, Warning, TEXT("Trying to add actor %s that is already inside of pool!"), *Actor->GetName())
			return false;
		}
		
		// Make sure our pool is able to grow before trying to add the actor
		if(!Pool->CanGrow())
		{
			Actor->Destroy();
			return false;
		}

		Pool->Push(Actor);
		OnActorEnteredPool(Actor);
		return true;
	}

	return false;
}

bool UActorPoolWorldSubsystem::CreatePool(TSubclassOf<AActor> ActorClass, int MinimumPoolSize, int MaximumPoolSize, int Amount)
{
	if(!IsValidActorClass(ActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor Class is invalid, could not create pool."))
		return false;
	}

	if(PoolMap.Find(ActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Pool already exists for class, could not create pool."))
		return false;
	}

	// Verify our values for the pool are proper, don't allow people to create negative pool sizes or make the min and max values out of order
	MinimumPoolSize = FMath::Max(MinimumPoolSize, 1);
	Amount = FMath::Max(MinimumPoolSize, Amount);
	MaximumPoolSize = FMath::Max(Amount, MaximumPoolSize);

	// Create a shared pointer to a new ActorPool, fill that pool with the specified amount of actors, and add it to our map so we can keep track of its lifetime
	const TSharedPtr<FActorPool> ActorPool = MakeShared<FActorPool>(MinimumPoolSize, MaximumPoolSize);
	FillPool(ActorClass, *ActorPool, Amount);
	PoolMap.Add(ActorClass, ActorPool);
	return true;
}

bool UActorPoolWorldSubsystem::RemovePool(TSubclassOf<AActor> ActorClass)
{
	if (!IsValidActorClass(ActorClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("Actor Class is invalid, could not remove pool of an invalid type."))
		return false;
	}

	// Make sure a pool exists for the class
	if(TSharedPtr<FActorPool>* ActorPool = PoolMap.Find(ActorClass))
	{
		ReleaseFromPool(*ActorPool->Get(), ActorPool->Get()->Pool.Num());
		PoolMap.Remove(ActorClass);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("Actor Pool does not exist, can not remove a pool that does not exist."))
	return false;
}

bool UActorPoolWorldSubsystem::ModifyPoolMinimumSize(TSubclassOf<AActor> ActorClass, const int NewMinimumPoolSize)
{
	return false;
}

AActor* UActorPoolWorldSubsystem::PopActorOfType(TSubclassOf<AActor> ActorClass, const FActorPopData& PopData)
{
	if (TSharedPtr<FActorPool>* PoolPointer = PoolMap.Find(ActorClass))
	{
		FActorPool* Pool = PoolPointer->Get();
		AActor* Actor = Pool->Pop();
		OnActorLeftPool(Actor, PopData);
		if (Pool->ShouldGrow())
		{
			FillPool(ActorClass, *Pool, Pool->MinimumPoolSize - Pool->Num());
		}
		return Actor;
	}

	return nullptr;
}

AActor* UActorPoolWorldSubsystem::ForceSpawnActor(TSubclassOf<AActor> ActorClass) const
{
	UWorld* World = GetWorld();

	if(!World || !ActorClass)
	{
		return nullptr;
	}
	
	AActor* NewActor = World->SpawnActor(ActorClass, &PoolingLocation);
	return NewActor;
}

bool UActorPoolWorldSubsystem::IsValidActorClass(const TSubclassOf<AActor>& ActorClass)
{
	return ActorClass && ActorClass->ImplementsInterface(UPooledActorInterface::StaticClass());
}

void UActorPoolWorldSubsystem::FillPool(UClass* Class, FActorPool& ActorPool, const int ActorSpawnAmount) const
{
	for(int i = 0; i < ActorSpawnAmount; i++)
	{
		AActor* Actor = ForceSpawnActor(Class);
		OnActorEnteredPool(Actor);
		ActorPool.Push(Actor);
	}
}

void UActorPoolWorldSubsystem::ReleaseFromPool(FActorPool& ActorPool, const int ActorRemoveAmount)
{
	if(ActorPool.Pool.IsEmpty())
	{
		return;
	}

	for (int i = 0; i < ActorRemoveAmount; i++)
	{
		if(AActor* Actor = ActorPool.Pop())
		{
			// Try destroying our actor if we are pointing to a valid object and it isn't already being destroyed
			if(Actor && !Actor->IsActorBeingDestroyed())
			{
				Actor->Destroy();
			}

			if(ActorPool.Num() <= 0)
			{
				break;
			}
		}
	}
}

void UActorPoolWorldSubsystem::OnActorLeftPool(AActor* Actor, const FActorPopData& PopData) const
{
	// Will use the found settings for our actors class or use the default settings
	const FPooledActorSettings Settings = ActorSettingsMap.FindRef(Actor->GetClass());
	
	// Turn on replication
	Actor->SetReplicates(Settings.ShouldReplicate());

	// Setup Actor based on passed in Pop Data
	Actor->SetActorLocation(PopData.GetLocation());
	Actor->SetOwner(PopData.GetOwner());
	Actor->SetInstigator(PopData.GetInstigator());
	Actor->SetActorRotation(PopData.GetRotator());

	// Performance and pool specific settings turned off when leaving pool
	Actor->SetActorTickEnabled(Settings.ShouldUseTick());
	Actor->SetActorHiddenInGame(Settings.ShouldHideInGame());

	IPooledActorInterface::Execute_OnPoolLeft(Actor, PopData);
	
	// Iterate over all actor components that implement pooled actor interface for initial setup
	TArray<UActorComponent*> ActorPooledInterfaceComponents = Actor->GetComponentsByInterface(UPooledActorInterface::StaticClass());
	for(int i = 0; i < ActorPooledInterfaceComponents.Num(); i++)
	{
		UActorComponent* ActorComponent = ActorPooledInterfaceComponents[i];
		IPooledActorInterface::Execute_OnPoolLeft(ActorComponent, PopData);
	}

	// Enable collision after calling everything else, that way we won't call overlap events until setup is complete
	Actor->SetActorEnableCollision(Settings.ShouldEnableCollision());
}

void UActorPoolWorldSubsystem::OnActorEnteredPool(AActor* Actor) const
{
	IPooledActorInterface::Execute_OnPoolEntered(Actor);

	// Iterate over all actor components that implement pooled actor interface for de-initialization
	TArray<UActorComponent*> ActorPooledInterfaceComponents = Actor->GetComponentsByInterface(UPooledActorInterface::StaticClass());
	for(int i = 0; i < ActorPooledInterfaceComponents.Num(); i++)
	{
		UActorComponent* ActorComponent = ActorPooledInterfaceComponents[i];
		IPooledActorInterface::Execute_OnPoolEntered(ActorComponent);
	}

	/* Interface functions are called before doing pool optimizations,
	 * this way if the actor still needed access to things such as the end location for the interface functions, it still has relevant data
	*/

	// Disable collision
	Actor->SetActorEnableCollision(false);

	// Set location of our actor to the pooling location, null out the owning actor and instigator
	Actor->SetActorLocation(PoolingLocation);
	Actor->SetOwner(nullptr);
	Actor->SetInstigator(nullptr);

	// Performance and pool specific settings turned on when entering pool
	Actor->SetActorTickEnabled(false);
	Actor->SetActorHiddenInGame(true);
	Actor->SetReplicates(false);
}
