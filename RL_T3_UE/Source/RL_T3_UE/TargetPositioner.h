// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BillboardComponent.h"
#include "TargetPositioner.generated.h"

UCLASS(Blueprintable, BlueprintType)
class RL_T3_UE_API ATargetPositioner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATargetPositioner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> RootComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Display")
	TObjectPtr<class UBillboardComponent> Marker;
	
public:

	/** 랜덤한 위치로 위치시킵니다. */
	UFUNCTION(BlueprintCallable)
	void SetRandomPosition();
};
