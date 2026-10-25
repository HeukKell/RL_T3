// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetPositioner.h"
#include "Math/UnrealMathUtility.h"


// Sets default values
ATargetPositioner::ATargetPositioner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));

	Marker = CreateDefaultSubobject<UBillboardComponent>(TEXT("Marker"));

	Marker->SetupAttachment(RootComponent);

	if (Marker) {

		static ConstructorHelpers::FObjectFinder<UTexture2D> TextureFinder(TEXT("/Engine/EditorResources/S_Actor.S_Actor"));
		if (TextureFinder.Succeeded()) {
			Marker->Sprite = TextureFinder.Object;
		}

		Marker->SetEditorScale(1.0f);

		Marker->bHiddenInGame = false;
		Marker->SetVisibility(true);
	}
	

}

// Called when the game starts or when spawned
void ATargetPositioner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATargetPositioner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATargetPositioner::SetRandomPosition()
{
	float RandomX = FMath::RandRange(-1000.0f, 1000.0f);

	SetActorLocation(FVector(RandomX, 0.0f, 0.0f));
}

