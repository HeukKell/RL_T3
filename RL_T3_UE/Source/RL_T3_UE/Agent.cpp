// Fill out your copyright notice in the Description page of Project Settings.


#include "Agent.h"
#include "Math/UnrealMathUtility.h"
#include "TargetPositioner.h"
#include "Async/Async.h"

#include "Windows/WindowsHWrapper.h"

FAIModelConnector::FAIModelConnector()
{

}

FAIModelConnector::FAIModelConnector(AAgent* NewControlAgent)
{
	Thread = FRunnableThread::Create(
		this, TEXT("FAIModelConnector")
	);

	ControlAgent = NewControlAgent;

	MaxEpisode = ControlAgent->MaxEpisode;
	CurrEpisode = 0;
	MaxSteps = ControlAgent->MaxSteps;
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
		L"Local\\AgentMemory"
	);

	if (SharedMemory_Handle == nullptr) {
		UE_LOG(LogTemp, Log, TEXT("CreateFileMappingW failed, Error : %d"), GetLastError());
		
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			UE_LOG(LogTemp, Log, TEXT("CreateFileMappingW failed, already exist. Error : %d"), GetLastError());
			UE_LOG(LogTemp, Log, TEXT("go. no problem"));
		}
		else {

			CloseHandle(SharedMemory_Handle);
			return 1;
		}
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

	HANDLE BrainStartEvent = CreateEventW(nullptr, false, false, L"Local\\BrainStartEvent");
	HANDLE RandomPosSettedEvent = CreateEvent(nullptr, false, false, nullptr);
	HANDLE StateSettedEvent = CreateEventW(nullptr, false, false, L"Local\\StateSettedEvent");
	HANDLE ActionSettedEvent = CreateEventW(nullptr, false, false, L"Local\\ActionSettedEvent");
	HANDLE ActionAdaptedEvent = CreateEvent(nullptr, false, false, L"Local\\ActionAdaptedEvent");
	HANDLE RewardSettedEvent = CreateEventW(nullptr, false, false, L"Local\\RewardSettedEvent");
	HANDLE TrainedCompEvent = CreateEventW(nullptr, false, false, L"Local\\TrainedCompEvent");

	HANDLE TrainPropCheckerEvent = CreateEvent(nullptr, false, false, nullptr);


	auto CloseEvent = [BrainStartEvent, RandomPosSettedEvent, StateSettedEvent, ActionSettedEvent, ActionAdaptedEvent, RewardSettedEvent, TrainedCompEvent, TrainPropCheckerEvent]() {
		CloseHandle(BrainStartEvent);
		CloseHandle(RandomPosSettedEvent);
		CloseHandle(StateSettedEvent);
		CloseHandle(ActionSettedEvent);
		CloseHandle(RewardSettedEvent);
		CloseHandle(TrainedCompEvent);
		CloseHandle(TrainPropCheckerEvent);
	};

	auto CloseSharedMemory = [SharedMemory_Handle, SharedMemory_Addr]() {
		UnmapViewOfFile(SharedMemory_Addr);
		CloseHandle(SharedMemory_Handle);
	};

	float Distance = 0.0f;
	int32 State = 0;

	/*
	bool IsValid_NeedActors = false;
	// Train 관련 속성 확인
	AsyncTask(ENamedThreads::GameThread, [this, TrainPropCheckerEvent,&IsValid_NeedActors]() {
	
		if (IsValid(this->ControlAgent) && IsValid(this->ControlAgent->TargetPositioner)) {
			IsValid_NeedActors = true;

			this->MaxEpisode = this->ControlAgent->MaxEpisode;
			this->MaxSteps = this->ControlAgent->MaxSteps;

		}
		SetEvent(TrainPropCheckerEvent);
	});

	WaitForSingleObject(TrainPropCheckerEvent, INFINITE); */

	if (CurrEpisode < MaxEpisode) {


		// Brain 시작 기다림
		while (true) {

			DWORD Result = WaitForSingleObject(BrainStartEvent, 5000);
			if (Result == WAIT_OBJECT_0) {
				UE_LOG(LogTemp, Log, TEXT("AIModelConnector::Run >> BrainStart!"))
				// 정상 수신
				break;
			}
			else if (Result == WAIT_FAILED) {
				CloseEvent();
				CloseSharedMemory();

				return 0;
			}

			UE_LOG(LogTemp, Log, TEXT("Waiting for brain."));
		}


		SharedMemory->IsTraining = 1;

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
			/*WaitForSingleObject(ActionSettedEvent,INFINITE);*/

			// 이렇게 하는 이유는 ActionSettedEvent 를 무한정 기다리면서도, 도중에 bStopRequested 가 호출되면 끝내기 위해.
			while (!bStopRequested) {

				DWORD Result = WaitForSingleObject(ActionSettedEvent, 1000);
				if (Result == WAIT_OBJECT_0) {
					break;
				}
				else if (Result == WAIT_FAILED) {
					bStopRequested = true;
					UE_LOG(LogTemp, Warning, TEXT("failed to Waiting for ActionSetted...Error : %d"), GetLastError());
					break;
				}

				UE_LOG(LogTemp, Log, TEXT("Waiting for ActionSetted..."));
			}
			if (bStopRequested) {
				break;
			}

			uint32 Action = SharedMemory->Action;
			UE_LOG(LogTemp, Log, TEXT("Action Setted! (%d)"), Action);


			// Action 설정됨

			float PrevDistance = 0.0f;
			float NextDistance = 0.0f;
			uint32 PrevState = 0;
			uint32 NextState = 0;

			// 솔직히 FRunnable 쓴 시점에서는 Event 보단 SharedMemory 에다가 Setted 넣고, Sleep 하는게 코드량이 적겠다.
			// FRunnable , SharedMemory ,FPlatforProcess::Sleep
			UE_LOG(LogTemp, Log, TEXT("Begin to move by action"));
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

			UE_LOG(LogTemp, Log, TEXT("Moved!"));

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
			else if (FMath::Abs(PrevDistance) > FMath::Abs(NextDistance)) {
				// 더 가까워진 경우
				SharedMemory->Reward = 1.0f;
			}
			else {
				SharedMemory->Reward = -1.0f;
			}
		
			SharedMemory->State = PrevState;
			SharedMemory->NextState = NextState;

			SetEvent(RewardSettedEvent);

			UE_LOG(LogTemp, Log, TEXT("Reward Setted (%f)"), SharedMemory->Reward);

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

			
			while (!bStopRequested) {

				DWORD Result = WaitForSingleObject(TrainedCompEvent, 1000);

				if (Result == WAIT_OBJECT_0) {

					UE_LOG(LogTemp, Log, TEXT("TrainComp!"));
					break;

				}
				else if (Result == WAIT_FAILED) {
					bStopRequested = true;
					UE_LOG(LogTemp, Warning, TEXT("Failed to waiting for TrainCompEvent. Error(%d)"), GetLastError());
					break;
				}

				UE_LOG(LogTemp, Log, TEXT("Waiting for TrainCompEvent ... "))
			}
			if (bStopRequested) {
				break;
			}

			//FPlatformProcess::Sleep(0.1f);

			///*AActor* ControlActor = nullptr;*/
			//// 
			//AsyncTask(ENamedThreads::GameThread, [this]() {

			//	if (IsValid(this->ControlAgent)) {

			//	}

			//});
		}
	}
	


	CloseSharedMemory();
	/*UnmapViewOfFile(SharedMemory_Addr);
	CloseHandle(SharedMemory_Handle);*/

	CloseEvent();
	/*CloseHandle(BrainStartEvent);
	CloseHandle(RandomPosSettedEvent);
	CloseHandle(StateSettedEvent);
	CloseHandle(ActionSettedEvent);
	CloseHandle(RewardSettedEvent);
	CloseHandle(TrainCompEvent);
	CloseHandle(TrainPropCheckerEvent);*/

	UE_LOG(LogTemp, Log, TEXT("FAIModelConnector::Run >> finish while roop"));

	AsyncTask(ENamedThreads::GameThread, [this]() {
		if (IsValid(this->ControlAgent)) {
			this->ControlAgent->EndedTrain();
		}
	});

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


	MaxEpisode = 10;
	MaxSteps = 50;
}

// Called when the game starts or when spawned
void AAgent::BeginPlay()
{
	Super::BeginPlay();

}

void AAgent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (nullptr != ModelConnector) {
		ModelConnector->Stop();
	}

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

	ModelConnector = MakeUnique<FAIModelConnector, AAgent*>(this);

}

void AAgent::EndedTrain_Implementation()
{
	ModelConnector = nullptr;
}
