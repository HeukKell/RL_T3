// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"

//#include "HAL/PlatformMemory.h"
//#include "HAL/UnrealMemory.h"

#include "Agent.generated.h"


class AAgent;
class ATargetPositioner;

struct FSharedMemoryType {
	
	uint32 State;
	uint32 NextState;
	uint32 Action;
	float Reward;

	uint32 IsTraining;
};

/**
	python 의 AI Model 의 응답을 기다리는 클래스.
	그리고 AI Model 의 요청에 따라 Character 을 조작할것이다.
*/
class FAIModelConnector : public FRunnable
{
public:

	FAIModelConnector();

	virtual ~FAIModelConnector();

	virtual bool Init() override;

	virtual uint32 Run() override;

	virtual void Stop() override;

	virtual void Exit() override;

private:

	FRunnableThread* Thread = nullptr;

	FThreadSafeBool bStopRequested = false;

private:
	// 공유 메모리

	//FPlatformMemory::FSharedMemoryRegion* SharedMemoryRegion = nullptr;

	//FSharedMemoryType* SharedMemory;

public:
	TObjectPtr<AAgent> ControlAgent;

	int32 MaxEpisode;
	int32 CurrEpisode;
	int32 MaxSteps;
	int32 CurrSteps;
};

UCLASS(BlueprintType, Blueprintable)
class RL_T3_UE_API AAgent : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAgent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	FAIModelConnector ModelConnector;

	/** 목표지점 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<ATargetPositioner> TargetPositioner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxEpisode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CurrEpisode;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 MaxSteps;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 CurrSteps;
public:

	/* 랜덤한 위치에 위치시킵니다. 
		위치는 x 축 -1000.0f ~ 1000.0f 사이에 위치시킵니다.
	*/
	UFUNCTION(BlueprintCallable)
	void SetRandomPosition();

	/** 액션을 수행합니다. */
	UFUNCTION(BlueprintCallable)
	void PlayAction(int32 ActionId);

	/** 목표지점과의 거리를 반환합니다.
	*/
	UFUNCTION(BlueprintCallable)
	float CheckTargetPosition(ATargetPositioner* Target);

	/** 거리를 이산화하여 state 로 반환합니다. */
	UFUNCTION(BlueprintCallable)
	int32 DistanceToState(float Distance);

	UFUNCTION(BlueprintCallable)
	void BeginTrain(int32 NewMaxEpisode, int32 NewMaxSteps);

	UFUNCTION(BleuprintAssignableEvent, BlueprintCallable)
	void EndTrain();
};
