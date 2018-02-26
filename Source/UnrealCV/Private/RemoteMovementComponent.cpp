#include "UnrealCVPrivate.h"
#include "CaptureManager.h"
#include "RemoteMovementComponent.h"
#include "UE4ROSBridgeManager.h"
#include "rosgraph_msgs/Clock.h"
#include "geometry_msgs/TransformStamped.h"
#include "tf2_msgs/TFMessage.h"
#include "sensor_msgs/JointState.h"
#include "nav_msgs/Odometry.h"
#include "ROSHelper.h"
#include "Kismet/KismetMathLibrary.h"

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

	// Convert Relative Translation to Global Translation
	FVector  Linear = FROSHelper::ConvertVectorROSToUE4(TwistMessage->GetLinear());
	FRotator Angular = FROSHelper::ConvertEulerAngleROSToUE4(TwistMessage->GetAngular());

	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FRotator CtrlRot = Pawn->GetControlRotation();
	FVector foward = UKismetMathLibrary::GetForwardVector(CtrlRot);
	FVector right = UKismetMathLibrary::GetRightVector(CtrlRot);
	FVector up = UKismetMathLibrary::GetUpVector(CtrlRot);

	FVector GlobalLinear = foward * Linear.X + right * Linear.Y + up * Linear.Z;
	FVector Direction;
	float Norm;
	float Factor = 1.0;
	GlobalLinear.ToDirectionAndLength(Direction, Norm);

	URemoteMovementComponent* Remote = FCaptureManager::Get().GetActor(0);
	FTransform Velocity = FTransform(Angular, Direction*Norm*Factor);
	this->Component->SetVelocityCmd(Velocity);
	UE_LOG(LogUnrealCV, Log, TEXT("Message received! Content: %s"), *TwistMessage->ToString());

	return;
}

URemoteMovementComponent::FROSJointStateSubScriber::FROSJointStateSubScriber(const FString& InTopic, URemoteMovementComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("sensor_msgs/JointState"))
{
	this->Component = Component;
}

URemoteMovementComponent::FROSJointStateSubScriber::~FROSJointStateSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> URemoteMovementComponent::FROSJointStateSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<sensor_msgs::JointState> Message = MakeShareable<sensor_msgs::JointState>(new sensor_msgs::JointState());
	Message->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(Message);
}

void URemoteMovementComponent::FROSJointStateSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<sensor_msgs::JointState> Message = StaticCastSharedPtr<sensor_msgs::JointState>(InMsg);

	TArray<FString> Names = Message->GetName();
	TArray<double> Positions = Message->GetPosition();

	// do something with the message
	for (UGTCameraCaptureComponent* Elem : this->Component->CaptureComponentList)
	{
		bool bSet = false;
		FString ROSName = Elem->GetROSName();
		FTransform InitialTransform = Elem->GetInitialTransform();
		//float Z = InitialTransform.GetLocation().Z , Yaw = InitialTransform.Rotator().Yaw, Pitch = InitialTransform.Rotator().Pitch;
		float Z = InitialTransform.GetLocation().Z, Yaw = 0.0, Pitch = 0.0;
		for (int i = 0; i < Names.Num(); i++)
		{
			FString Name = Names[i];
			double Position = Positions[i];
			if (Name.Compare(ROSName + TEXT("_base_joint")) == 0) {
				Z += FROSHelper::ConvertVectorROSToUE4(geometry_msgs::Vector3(0.0, 0.0, Position)).Z;
				bSet = true;
			}
			else if (Name.Compare(ROSName + TEXT("_pan_joint")) == 0) {
				Yaw += FROSHelper::ConvertEulerAngleROSToUE4(geometry_msgs::Vector3(0.0, 0.0, Position)).Yaw;
				bSet = true;
			}
			else if (Name.Compare(ROSName + TEXT("_tilt_joint")) == 0) {
				Pitch += FROSHelper::ConvertEulerAngleROSToUE4(geometry_msgs::Vector3(0.0, Position, 0.0)).Pitch;
				bSet = true;
			}
		}
		if (bSet) {
			Elem->SetTargetPose(FTransform(FRotator(Pitch, Yaw, 0.0), FVector(0.0, 0.0, Z)));
			UE_LOG(LogUnrealCV, Log, TEXT("Pitch: %f, Yaw: %f, Z: %f"), Pitch, Yaw, Z);
		}
	}
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

void URemoteMovementComponent::ROSPublishOdom(float DeltaTime)
{
	APawn* OwningPawn = Cast<APawn>(this->GetOwner());
	std_msgs::Header Header(0, FROSTime(), "odom");

	FTransform Transform = OwningPawn->GetActorTransform();

	// publish tf
	geometry_msgs::Transform transform = FROSHelper::ConvertTransformUE4ToROS(Transform);

	TArray<geometry_msgs::TransformStamped> transforms = { geometry_msgs::TransformStamped(Header, "base_footprint", transform) };
	TSharedPtr<tf2_msgs::TFMessage> odomtrans = MakeShareable(new tf2_msgs::TFMessage(transforms));
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

	geometry_msgs::Vector3 linear = FROSHelper::ConvertVectorUE4ToROS((Transform.GetLocation() - PrevLinear) / DeltaTime);
	FRotator AngularVel = FRotator((Transform.Rotator().Pitch - PrevAngular.Pitch) / DeltaTime,
		(Transform.Rotator().Yaw - PrevAngular.Yaw) / DeltaTime,
		(Transform.Rotator().Roll - PrevAngular.Roll) / DeltaTime);
	geometry_msgs::Vector3 angular = FROSHelper::ConvertEulerAngleUE4ToROS(AngularVel);
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

void URemoteMovementComponent::ROSPublishJointState(float DeltaTime)
{
	std_msgs::Header Header(0, FROSTime(), "");
	TArray<FString> Names = { TEXT("main_cam_base_joint"), TEXT("main_cam_pan_joint"), TEXT("main_cam_tilt_joint") };
	TArray<double> Positions;
	TArray<double> Velocities;
	TArray<double> Efforts;

	Positions.Empty();
	Velocities.Empty();
	Efforts.Empty();

	for (UGTCameraCaptureComponent* Elem : this->CaptureComponentList)
	{
		FString ROSName = Elem->GetROSName();
		FTransform InitialTransform = Elem->GetInitialTransform();
		//float Z = InitialTransform.GetLocation().Z , Yaw = InitialTransform.Rotator().Yaw, Pitch = InitialTransform.Rotator().Pitch;
		float Z = InitialTransform.GetLocation().Z, Yaw = 0.0, Pitch = 0.0;
		for (int i = 0; i < Names.Num(); i++)
		{
			FString Name = Names[i];
			UCameraComponent* CameraComponent = Elem->GetCameraComponent();

			if (Name.Compare(ROSName + TEXT("_base_joint")) == 0) {
				Z = CameraComponent->GetRelativeTransform().GetLocation().Z - Z;
				Positions.Add(FROSHelper::ConvertVectorUE4ToROS(FVector(0.0, 0.0, Z)).GetZ());
			}
			else if (Name.Compare(ROSName + TEXT("_pan_joint")) == 0) {
				Yaw = CameraComponent->GetRelativeTransform().Rotator().Yaw - Yaw;
				Positions.Add(FROSHelper::ConvertEulerAngleUE4ToROS(FRotator(0.0, Yaw, 0.0)).GetZ());
			}
			else if (Name.Compare(ROSName + TEXT("_tilt_joint")) == 0) {
				Pitch = CameraComponent->GetRelativeTransform().Rotator().Pitch - Pitch;
				Positions.Add(FROSHelper::ConvertEulerAngleUE4ToROS(FRotator(Pitch, 0.0, 0.0)).GetY());
			}
		}
	}

	TSharedPtr<sensor_msgs::JointState> jointstates = MakeShareable(new sensor_msgs::JointState(Header, Names, Positions, Velocities, Efforts));
	Handler->PublishMsg("/ue4/robot/joint_states", jointstates);
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

	ROSPublishOdom(DeltaTime);

	ROSPublishJointState(DeltaTime);
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
	TSharedPtr<FROSBridgePublisher> Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/clock"), TEXT("rosgraph_msgs/Clock")));
	Handler->AddPublisher(Publisher);

	TSharedPtr<FROSBridgePublisher> OdomTfPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/tf"), TEXT("tf2_msgs/TFMessage")));
	Handler->AddPublisher(OdomTfPublisher);
	TSharedPtr<FROSBridgePublisher> OdomPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/odom"), TEXT("nav_msgs/Odometry")));
	Handler->AddPublisher(OdomPublisher);

	TSharedPtr<FROSBridgePublisher> JointStatePublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/robot/joint_states"), TEXT("sensor_msgs/JointState")));
	Handler->AddPublisher(JointStatePublisher);

	// Add topic subscribers
	TSharedPtr<FROSTwistSubScriber> TwistSubscriber = MakeShareable<FROSTwistSubScriber>(new FROSTwistSubScriber(TEXT("/ue4/robot/ctrl/move"), this));
	Handler->AddSubscriber(TwistSubscriber);
	TSharedPtr<FROSJointStateSubScriber> JointStateSubscriber = MakeShareable<FROSJointStateSubScriber>(new FROSJointStateSubScriber(TEXT("/ue4/robot/ctrl/joint_states"), this));
	Handler->AddSubscriber(JointStateSubscriber);

	// Connect to ROSBridge Websocket server.
	Handler->Connect();
}

void URemoteMovementComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	// Disconnect the handler before parent ends
	Handler->Disconnect();

	Super::EndPlay(Reason);
}
