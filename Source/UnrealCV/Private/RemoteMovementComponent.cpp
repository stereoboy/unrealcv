#include "UnrealCVPrivate.h"
#include "RemoteMovementComponent.h"

URemoteMovementComponent::URemoteMovementComponent()
	:bIsTicking(false)
{

}

URemoteMovementComponent* URemoteMovementComponent::Create(FName Name, APawn* Pawn)
{
	//URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)GetTransientPackage(), Name);
	//URemoteMovementComponent* remoteActor = Pawn->CreateDefaultSubobject<URemoteMovementComponent>(Name);
	URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)Pawn, Name);	
	remoteActor->RegisterComponent();

	remoteActor->Init();

	return remoteActor;
}

void URemoteMovementComponent::Init(void)
{
	this->bIsTicking = true;
}

void URemoteMovementComponent::SetVelocityCmd(const FTransform& InVelocityCmd)
{
	this->VelocityCmd = InVelocityCmd;
	this->MoveAnimCountFromRemote = PARAM_MOVE_ANIM_COUNT;
}

void URemoteMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!this->bIsTicking)
	{
		return;
	}
	//UE_LOG(LogUnrealCV, Log, TEXT("URemoteMovementComponent::Tick()"));
	APawn* OwningPawn = Cast<APawn>(this->GetOwner());
	const AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		if (this->MoveAnimCountFromRemote > 0)
		{
			FVector DeltaXYZ = this->VelocityCmd.GetLocation()*DeltaTime;
			FRotator Rot = this->VelocityCmd.Rotator();

			OwningPawn->AddMovementInput(DeltaXYZ);
			OwningPawn->AddControllerPitchInput(Rot.Pitch*DeltaTime);
			OwningPawn->AddControllerYawInput(Rot.Yaw*DeltaTime);
			OwningPawn->AddControllerRollInput(Rot.Roll*DeltaTime);

			this->MoveAnimCountFromRemote--;
		}
	}
}


void URemoteMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	/*
	// https://answers.unrealengine.com/questions/704875/cannot-set-max-fps-in-417.html
	// This code write the setting on GameUserSettings.ini
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	Settings->SetFrameRateLimit(60.0f);
	Settings->ApplySettings(true);
	*/
	/*
	// https://answers.unrealengine.com/questions/221428/console-exec-command-from-c-fails-to-run.html
	APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (Controller)
	{
		FString result = Controller->ConsoleCommand(TEXT("t.MaxFPS 60"));
	}
	*/
	/*
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	bool result = World->Exec(World, TEXT("t.MaxFPS 60"));
	*/
	//GEngine->Exec(GetWorld(), TEXT("t.MaxFPS 60"));
}
