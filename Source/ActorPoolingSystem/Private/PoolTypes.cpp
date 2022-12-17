// Fill out your copyright notice in the Description page of Project Settings.


#include "PoolTypes.h"

bool FActorPool::ShouldGrow() const
{
	return Pool.Num() < MinimumPoolSize;
}

bool FActorPool::CanGrow() const
{
	return Pool.Num() < MaximumPoolSize;
}

bool FActorPool::CanShrink() const
{
	return Pool.Num() > MinimumPoolSize;
}

void FActorPool::Push(AActor* Actor)
{
	Pool.Push(Actor);
}

AActor* FActorPool::Pop()
{
	if(!Pool.IsEmpty())
	{
		return Pool.Pop();
	}

	return nullptr;
}

int FActorPool::Num() const
{
	return Pool.Num();
}

bool FActorPool::ContainsActor(AActor* Actor) const
{
	return Pool.Contains(Actor);
}
