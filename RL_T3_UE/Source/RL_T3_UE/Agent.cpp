// Fill out your copyright notice in the Description page of Project Settings.


#include "Agent.h"
#include "Math/UnrealMathUtility.h"
#include "TargetPositioner.h"
#include "Async/Async.h"

#include "Windows/WindowsHWrapper.h"



FAIModelConnector::FAIModelConnector()
{
	Thread = FRunnableThread::Create(
		this, TEXT("FAIModelConnector")
	);

	MaxEpisode = 0;
	CurrEpisode = 0;
	MaxSteps = 0;
	CurrSteps = 0;
}

FAIModelConnector::~FAIModelConnector()
{
	if (Thread) {
		Thread->Kill(true);
		delete Thread;
		Thread = nullptr;
	}
}

bool FAIModelConnector::Init()
{
	UE_LOG(LogTemp, Log, TEXT("Worker Inited"));

	// 공유 메모리 연결

	/*
	SharedMemoryRegion = FPlatformMemory::MapNamedSharedMemoryRegion(
		TEXT("AgentMemory"),
		false,
		FPlatformMemory::ESharedMemoryAccess::Read | FPlatformMemory::ESharedMemoryAccess::Write,
		sizeof(FSharedMemoryType)
	);

	if (SharedMemoryRegion) {

		void* SharedMemoryAddr = SharedMemoryRegion->GetAddress();
		SharedMemory = static_cast<FSharedMemoryType*>(SharedMemoryAddr);

		UE_LOG(LogTemp, Log, TEXT("AIModelConnector::Init >> Success to connect SharedMemory 'AgentMemory' "));

		return true;
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("AIModelConnector::Init >> Failed to connect SharedMemory 'AgentMemory' "));
		return false;
	}

	*/

	return true;
}

uint32 FAIModelConnector::Run()
{

	HANDLE SharedMemory_Handle = CreateFileMappingW(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		0,
		sizeof(FSharedMemoryType),
		L"Lcoal\\AgentMemory"
	);

	if (SharedMemory_Handle == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("CreateFileMappingW failed, Error : %d"), GetLastError());
		return 1;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		UE_LOG(LogTemp, Log, TEXT("CreateFileMappingW failed, already exist. Error : %d"), GetLastError());

		CloseHandle(SharedMemory_Handle);
		return 1;
	}

	void* SharedMemory_Addr = MapViewOfFile(
		SharedMemory_Handle,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(FSharedMemoryType)
	);

	if (SharedMemory_Addr == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("MapViewOfFile failed. Error : %d"), GetLastError());

		CloseHandle(SharedMemory_Handle);
		return 1;
	}

	FSharedMemoryType* SharedMemory = static_cast<FSharedMemoryType*>(SharedMemory_Addr);

	HANDLE BrainStartEvent = CreateEventW(nullptr, false, false, L"Lcoal\\BrainStartEvent");
	HANDLE RandomPosSettedEvent = CreateEvent(nullptr, false, false, nullptr);
	HANDLE StateSettedEvent = CreateEventW(nullptr, false, false, L"Lcoal\\StateSettedEvent");
	HANDLE ActionSettedEvent = CreateEventW(nullptr, false, false, L"Lcoal\\ActionSettedEvent");
	HANDLE ActionAdaptedEvent = CreateEvent(nullptr, false, false, L"Lcoal\\ActionAdaptedEvent");
	HANDLE RewardSettedEvent = CreateEventW(nullptr, false, false, L"Lcoal\\RewardSettedEvent");
	HANDLE TrainCompEvent = CreateEventW(nullptr, false, false, L"Lcoal\\TrainedCompEvent");

	HANDLE TrainPropCheckerEvent = CreateEvent(nullptr, false, false, nullptr);


	MaxEpisode = 0;
	CurrEpisode = 0;
	MaxSteps = 0;
	CurrSteps = 0;
	
	bool IsValid_NeedActors = false;

	float Distance = 0.0f;
	int32 State = 0;


	// Train 관련 속성 확인
	AsyncTask(ENamedThreads::GameThread, [this, TrainPropCheckerEvent,&IsValid_NeedActors]() {
	
		if (IsValid(this->ControlAgent) && IsValid(this->ControlAgent->TargetPositioner)) {
			IsValid_NeedActors = true;

			this->MaxEpisode = this->ControlAgent->MaxEpisode;
			this->MaxSteps = this->ControlAgent->MaxSteps;

			SetEvent(TrainPropCheckerEvent);
		}
	});

	WaitForSingleObject(TrainPropCheckerEvent, INFINITE);

	if (CurrEpisode < MaxEpisode) {

		// 먼저 시작 event 기다림.
		if (WaitForSingleObject(BrainStartEvent, INFINITE) == WAIT_OBJECT_0) {
			UE_LOG(LogTemp, Log, TEXT("AIModelConnector::Run >> BrainStart!"))
		}

		SharedMemory->IsTraining = 1;
		UE_LOG(LogTemp, Log, TEXT("FAIModelConnector::Run::While >> Begin"));

		while (!bStopRequested) {

			if (CurrEpisode > MaxEpisode) {
				break;
			}

			UE_LOG(LogTemp, Log, TEXT("Episode(%d,%d) in step(%d,%d) start. "), CurrEpisode, MaxEpisode, CurrSteps, MaxSteps)

			AsyncTask(ENamedThreads::GameThread, [this, RandomPosSettedEvent, &Distance, &State]() {
				
				if (IsValid(this->ControlAgent) && IsValid(this->ControlAgent->TargetPositioner)) 
				{
					if (this->CurrSteps == 0) { // 신규 에피소드 첫 시작이라면,
						this->ControlAgent->SetRandomPosition();
						this->ControlAgent->TargetPositioner->SetRandomPosition();
					}

					Distance = this->ControlAgent->CheckTargetPosition(this->ControlAgent->TargetPositioner);
					State = this->ControlAgent->DistanceToState(Distance);
				}
				else {
					UE_LOG(LogTemp,Warning, TEXT("ControlAgent is not valid"))
				}

				SetEvent(RandomPosSettedEvent);
			});

			WaitForSingleObject(RandomPosSettedEvent, INFINITE);
			
			// Random 한 위치에 위치시켰다.
		
			// State 설정
			SharedMemory->State = State;
			SetEvent(StateSettedEvent);

			// Action 기다림
			WaitForSingleObject(ActionSettedEvent,INFINITE);

			uint32 Action = SharedMemory->Action;

			// Action 설정됨

			float PrevDistance = 0.0f;
			float NextDistance = 0.0f;
			uint32 PrevState = 0;
			uint32 NextState = 0;

			// 솔직히 FRunnable 쓴 시점에서는 Event 보단 SharedMemory 에다가 Setted 넣고, Sleep 하는게 코드량이 적겠다.
			// FRunnable , SharedMemory ,FPlatforProcess::Sleep

			// 움직여!
			AsyncTask(ENamedThreads::GameThread, [this, ActionAdaptedEvent, &PrevDistance, &NextDistance, &PrevState, &NextState, Action]() {

	
				if (	IsValid(this->ControlAgent) && 
						IsValid(this->ControlAgent->TargetPositioner)
				) 
				{
					PrevDistance = this->ControlAgent->CheckTargetPosition(this->ControlAgent->TargetPositioner);
					PrevState = this->ControlAgent->DistanceToState(PrevDistance);
				
					this->ControlAgent->PlayAction(Action);

					NextDistance = this->ControlAgent->CheckTargetPosition(this->ControlAgent->TargetPositioner);
					NextState = this->ControlAgent->DistanceToState(PrevDistance);
				}

				else {

					// 없으면 사실상 안되긴 하다.
					UE_LOG(LogTemp, Warning, TEXT("ControlAgent is not valid"))

				}

				SetEvent(ActionAdaptedEvent);
			});

			WaitForSingleObject(ActionAdaptedEvent, INFINITE);
			// 액션 적용 되었음

			// 보상 설계
			if (FMath::IsNearlyEqual(PrevDistance, NextState, 1)) {
				// 안움직였다.

				if (NextState == 2 && Action == 1) {
					// 의도적으로 안움직였다. 
					SharedMemory->Reward = 20.0f;
				}
				else if (CurrSteps >= MaxSteps) {
					// 현재 마지막 스텝이고, 마지막 스텝 마저도 완료를 못했다면,
					SharedMemory->Reward = -10.0f;
				}
				else {
					SharedMemory->Reward = -1.0f;
				}
			}
			else if (FMath::Abs(PrevDistance) > FMath::Abs(NextState)) {
				// 더 가까워진 경우
				SharedMemory->Reward = 1.0f;
			}
			else {
				SharedMemory->Reward = -1.0f;
			}
		
			SharedMemory->State = PrevState;
			SharedMemory->NextState = NextState;

			SetEvent(RewardSettedEvent);

			if (FMath::Abs(SharedMemory->Reward) >= 10) {
				// 성공이든 실패든 아무튼 에피소드 종료

				if (CurrEpisode >= MaxEpisode) {
					// TODO 학습 종료

					bStopRequested = true;
					SharedMemory->IsTraining = false;
				}
				else {
					CurrEpisode++;
					CurrSteps = 0;

					// 새로운 상태 설정
				}
			}
			else {
				CurrSteps++;
			}

			WaitForSingleObject(TrainCompEvent, INFINITE);

			//FPlatformProcess::Sleep(0.1f);

			///*AActor* ControlActor = nullptr;*/
			//// 
			//AsyncTask(ENamedThreads::GameThread, [this]() {

			//	if (IsValid(this->ControlAgent)) {

			//	}

			//});
		}
	}
	
	UnmapViewOfFile(SharedMemory_Addr);
	CloseHandle(SharedMemory_Handle);

	CloseHandle(BrainStartEvent);
	CloseHandle(RandomPosSettedEvent);
	CloseHandle(StateSettedEvent);
	CloseHandle(ActionSettedEvent);
	CloseHandle(RewardSettedEvent);
	CloseHandle(TrainCompEvent);
	CloseHandle(TrainPropCheckerEvent);

	return 0;

}

void FAIModelConnector::Stop()
{
	bStopRequested = true;
}

void FAIModelConnector::Exit()
{
	UE_LOG(LogTemp, Log, TEXT("Worker Exit"));

	/*if (!SharedMemoryRegion) {
		return;
	}

	FPlatformMemory::UnmapNamedSharedMemoryRegion(SharedMemoryRegion);
	SharedMemoryRegion = nullptr;
	SharedMemory = nullptr;*/
}

// Sets default values
AAgent::AAgent()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	MaxEpisode = 0;
	CurrEpisode = 0;

	CurrSteps = 0;
	MaxSteps = 0;
}

// Called when the game starts or when spawned
void AAgent::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AAgent::SetRandomPosition()
{

	float RandomX = FMath::RandRange(1000.0f, -1000.0f);
	SetActorLocation(FVector(RandomX, 0.0f, 0.0f));
}

void AAgent::PlayAction(int32 ActionId)
{
	switch (ActionId) {
	case 0:

		
		// 왼쪽 50unit 이동
		AddActorLocalOffset(FVector(-50.0f, 0.0f, 0.0f));
		break;

	case 1:
		
		// 정지
		break;

	case 2:
		AddActorLocalOffset(FVector(+50.0f, 0.0f, 0.0f));
		// 오른쪽 50 unit 이동
		break;
	}
}

float AAgent::CheckTargetPosition(ATargetPositioner* Target)
{
	float Distance = (Target->GetActorLocation() - GetActorLocation()).X;

	return Distance;
}

int32 AAgent::DistanceToState(float Distance)
{
	
	if (Distance <= -500.0f) {
		return 0;
	}
	else if (Distance <= -100.0f) {
		return 1;
	}
	else if (Distance <= 100.0f) {
		return 2;
	}
	else if (Distance <= 500.0f) {
		return 3;
	}
	else {
		return 4;
	}
}

void AAgent::BeginTrain(int32 NewMaxEpisode, int32 NewMaxSteps)
{
	ModelConnector.ControlAgent = self;

	ModelContainer.
}
