#include "UnrealCVPrivate.h"
#include "RemoteMovementComponent.h"
#include "UE4ROSBridgeManager.h"
#include "rosgraph_msgs/Clock.h"
#include "geometry_msgs/TransformStamped.h"
#include "nav_msgs/Odometry.h"
#include "ROSHelper.h"

URemoteMovementComponent::FROSTwistSubScriber::FROSTwistSubScriber(const FString& InTopic,  URemoteMovementComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("geometry_msgs/Twist"))
{
	this->Component = Component;
}

URemoteMovementComponent::FROSTwistSubScriber::~FROSTwistSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> URemoteMovementComponent::FROSTwistSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<geometry_msgs::Twist> TwistMessage = MakeShareable<geometry_msgs::Twist>(new geometry_msgs::Twist());
	TwistMessage->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(TwistMessage);
}

void URemoteMovementComponent::FROSTwistSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<geometry_msgs::Twist> TwistMessage = StaticCastSharedPtr<geometry_msgs::Twist>(InMsg);

	// do something with the message
	FVector	 Linear = FROSHelper::ConvertVectorROSToUE4(TwistMessage->GetLinear());
	FRotator Angular = FROSHelper::ConvertEulerAngleROSToUE4(TwistMessage->GetAngular());
	FTransform Vel(Angular, Linear);
	this->Component->SetVelocityCmd(Vel);
	UE_LOG(LogUnrealCV, Log, TEXT("Message received! Content: %s"), *TwistMessage->ToString());

	return;
}

URemoteMovementComponent::URemoteMovementComponent()
	:bIsTicking(false)
{
	this->PrevLinear = FVector(0.0, 0.0, 0.0);
	this->PrevAngular = FRotator(0.0, 0.0, 0.0);
}

URemoteMovementComponent* URemoteMovementComponent::Create(FName Name, APawn* Pawn)
{
	//URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)GetTransientPackage(), Name);
	//URemoteMovementComponent* remoteActor = Pawn->CreateDefaultSubobject<URemoteMovementComponent>(Name);
	URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)Pawn, Name);	
	remoteActor->AddToRoot();
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

void URemoteMovementComponent::ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// publish clock
	float GameTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	uint64 GameSeconds = (int)GameTime;
	uint64 GameUseconds = (GameTime - GameSeconds) * 1000000000;
	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable
	(new rosgraph_msgs::Clock(FROSTime(GameSeconds, GameUseconds)));
	Handler->PublishMsg("/clock", Clock);

	// publish tf and odometry
	APawn* OwningPawn = Cast<APawn>(this->GetOwner());
	std_msgs::Header Header(0, FROSTime(), "odom");

	FTransform Transform = OwningPawn->GetActorTransform();

	// publish tf
	geometry_msgs::Transform transform = FROSHelper::ConvertTransformUE4ToROS(Transform);

	TSharedPtr<geometry_msgs::TransformStamped> odomtrans = MakeShareable(new geometry_msgs::TransformStamped(Header, "base_footprint", transform));
	Handler->PublishMsg("/tf", odomtrans);

	// publish odometry
	geometry_msgs::Vector3 vec = transform.GetTranslation();
	geometry_msgs::Quaternion quat = transform.GetRotation();
	geometry_msgs::Pose pose(geometry_msgs::Point(vec.GetX(), vec.GetY(), vec.GetZ()), quat);
	TArray<double> posecov = {
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	};
	geometry_msgs::PoseWithCovariance	posewithcov(pose, posecov);

	geometry_msgs::Vector3 linear	= FROSHelper::ConvertVectorUE4ToROS((Transform.GetLocation() - PrevLinear)/DeltaTime);
	FRotator AngularVel = FRotator(	(Transform.Rotator().Pitch	- PrevAngular.Pitch	)/ DeltaTime,
									(Transform.Rotator().Yaw	- PrevAngular.Yaw	)/ DeltaTime,
									(Transform.Rotator().Roll	- PrevAngular.Roll	)/ DeltaTime);
	geometry_msgs::Vector3 angular	= FROSHelper::ConvertEulerAngleUE4ToROS(AngularVel);
	TArray<double> twistcov = {
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	};
	geometry_msgs::TwistWithCovariance	twistwithcov(geometry_msgs::Twist(linear, angular), twistcov);

	TSharedPtr<nav_msgs::Odometry> odometry = MakeShareable(new nav_msgs::Odometry(Header, "base_footprint", posewithcov, twistwithcov));
	Handler->PublishMsg("/ue4/odom", odometry);

	Handler->Process();

	PrevLinear = Transform.GetLocation();
	PrevAngular = Transform.Rotator();
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

		ProcessUROSBridge(DeltaTime, TickType, ThisTickFunction);
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
	GEngine->Exec(GetWorld(), TEXT("t.MaxFPS 30"));

	// Set websocket server address to ws://127.0.0.1:9001
	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_MASTER_PORT));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****
	Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/clock"), TEXT("rosgraph_msgs/Clock")));
	Handler->AddPublisher(Publisher);

	OdomTfPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/tf"), TEXT("geometry_msgs/TransformStamped")));
	Handler->AddPublisher(OdomTfPublisher);
	OdomPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/odom"), TEXT("nav_msgs/Odometry")));
	Handler->AddPublisher(OdomPublisher);

	// Add topic subscribers and publishers
	TSharedPtr<FROSTwistSubScriber> Subscriber = MakeShareable<FROSTwistSubScriber>(new FROSTwistSubScriber(TEXT("/ue4/robot/ctrl"), this));
	Handler->AddSubscriber(Subscriber);

	// Connect to ROSBridge Websocket server.
	Handler->Connect();
}

void URemoteMovementComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	// Disconnect the handler before parent ends
	Handler->Disconnect();

	Super::EndPlay(Reason);
}
