#include "UnrealCVPrivate.h"
#include "UE4ROSBridgeManager.h"
#include "UE4ROSBaseRobot.h"
#include "UE4ROSBaseCharacter.h"
#include "geometry_msgs/PolygonStamped.h"
#include "rosgraph_msgs/Clock.h"

AUE4ROSBridgeManager::AUE4ROSBridgeManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

AUE4ROSBridgeManager::FROSResetSubScriber::FROSResetSubScriber(const FString& InTopic,  AUE4ROSBridgeManager* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("std_msgs/Float32"))
{
	this->Component = Component;
}

AUE4ROSBridgeManager::FROSResetSubScriber::~FROSResetSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> AUE4ROSBridgeManager::FROSResetSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<std_msgs::Float32> Float32Message = MakeShareable<std_msgs::Float32>(new std_msgs::Float32());
	Float32Message->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(Float32Message);
}

void AUE4ROSBridgeManager::FROSResetSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<std_msgs::Float32> Float32Message = StaticCastSharedPtr<std_msgs::Float32>(InMsg);

	this->Component->ResetCharacterPoses();
	this->Component->SetEpisodeDone(Float32Message->GetData());
	UE_LOG(LogUnrealCV, Log, TEXT("Message received! Content: %s"), *Float32Message->ToString());

	return;
}

/*
 * https://docs.unrealengine.com/en-us/Programming/UnrealArchitecture/Actors/ActorLifecycle
 */

void AUE4ROSBridgeManager::BeginPlay()
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
	/*
	 * MaxFPS Setup is very important on LINUX. If it setup too high value for fast rendering, data order can be mixed up.
	 *
	 */
	UE_LOG(LogUnrealCV, Warning, TEXT("AUE4ROSBridgeManager::BeginPlay()"));
	// setup for CustomDepthStencil buffer for label images
	GEngine->Exec(GetWorld(), TEXT("r.CustomDepth 3"));
	// FPS limitation for publishing stability
	//GEngine->Exec(GetWorld(), TEXT("stat FPS"));
	GEngine->Exec(GetWorld(), TEXT("t.MaxFPS 10"));

	// Set websocket server address to ws://127.0.0.1:9001
	ROSHandler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_MASTER_PORT));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****
	TSharedPtr<FROSBridgePublisher> Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/clock"), TEXT("rosgraph_msgs/Clock")));
	ROSHandler->AddPublisher(Publisher);

	TSharedPtr<FROSBridgePublisher> RewardPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/status"), TEXT("geometry_msgs/PolygonStamped")));
	ROSHandler->AddPublisher(RewardPublisher);

	TSharedPtr<FROSResetSubScriber> Subscriber = MakeShareable<FROSResetSubScriber>(new FROSResetSubScriber(TEXT("/ue4/reset"), this));
	ROSHandler->AddSubscriber(Subscriber);

	// Connect to ROSBridge Websocket server.
	ROSHandler->Connect();
}

void AUE4ROSBridgeManager::HandleHit()
{
	IsHit = true;
	Reward = -1.0f;
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("[INFO] Publish Reward!"));
}

void SetCharacterInitialPoses(APawn* Pawn, FTransform InitialPlayerPoses, TArray<FTransform> InitialPoses)
{

}

void AUE4ROSBridgeManager::ResetCharacterPoses()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Blue, TEXT("[INFO] ResetCharacterPoses()"));

	TArray<AActor *> Robots;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUE4ROSBaseRobot::StaticClass(), Robots);
	for (auto Elem : Robots)
	{
		auto Robot = Cast<AUE4ROSBaseRobot>(Elem);
		Robot->ResetPose();
	}

	TArray<AActor *> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUE4ROSBaseCharacter::StaticClass(), Characters);
	for (auto Elem : Characters)
	{
		auto Character = Cast<AUE4ROSBaseCharacter>(Elem);
		Character->ResetPose();
	}
}

void AUE4ROSBridgeManager::AttachCaptureComponentToCamera(APawn* Pawn)
{
	ActorList.Empty();
	CaptureComponentList.Empty();

	URemoteMovementComponent* actor = URemoteMovementComponent::Create(FName(TEXT("Main_Actor")), Pawn);
	ActorList.Add(actor);
	AddTickPrerequisiteComponent(actor);

	UE_LOG(LogUnrealCV, Log, TEXT("Camera Component List"));
	TArray<UActorComponent*> cameras = (Pawn->GetComponentsByClass(UCameraComponent::StaticClass()));
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));
	for (int32 idx = 0; idx < cameras.Num(); ++idx)
	{
		UCameraComponent* camera = Cast<UCameraComponent>(cameras[idx]);
		if (camera->GetName().Compare(TEXT("Camera")) == 0)
		{
			TArray<FString> SupportedModes = {TEXT("lit"), TEXT("object_mask")};
			UGTCameraCaptureComponent* camCom = UGTCameraCaptureComponent::Create(FName(*camera->GetName()), Pawn, camera->GetOwner(), camera, SupportedModes);
			camCom->SetUROSBridge(TEXT("/ue4"), TEXT("main_cam"), TEXT("main_img_link"));
			CaptureComponentList.Add(camCom);
			actor->AddCaptureComponent(camCom);
			AddTickPrerequisiteComponent(camCom);
		}
		else if (camera->GetName().Compare(TEXT("RGBDCamRGB")) == 0)
		{
			TArray<FString> SupportedModes = {TEXT("lit")};
			UGTCameraCaptureComponent* camCom = UGTCameraCaptureComponent::Create(FName(*camera->GetName()), Pawn, camera->GetOwner(), camera, SupportedModes);
			camCom->SetUROSBridge(TEXT("/ue4"), TEXT("rgbd_cam"), TEXT("rgbd_rgb_img_link"));
			CaptureComponentList.Add(camCom);
			actor->AddCaptureComponent(camCom);
			AddTickPrerequisiteComponent(camCom);
		}
		else if (camera->GetName().Compare(TEXT("RGBDCamDepth")) == 0)
		{
			TArray<FString> SupportedModes = {TEXT("depth")};
			UGTCameraCaptureComponent* camCom = UGTCameraCaptureComponent::Create(FName(*camera->GetName()), Pawn, camera->GetOwner(), camera, SupportedModes);
			camCom->SetUROSBridge(TEXT("/ue4"), TEXT("rgbd_cam"), TEXT("rgbd_depth_img_link"));
			CaptureComponentList.Add(camCom);
			actor->AddCaptureComponent(camCom);
			AddTickPrerequisiteComponent(camCom);
		}
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetFullName());
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetName());
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetFullGroupName(false));
	}
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));

	// Put color for segmentation
	uint32 ObjectIndex = 1; // 0 for Non-SkeletalMeshComponent
	for (AActor* Actor : Pawn->GetLevel()->Actors)
	{
		if (Actor)
		{
			TArray<UMeshComponent*> PaintableComponents;
			Actor->GetComponents<UMeshComponent>(PaintableComponents);
			if (PaintableComponents.Num() == 0)
			{
				continue;
			}
			for (auto MeshComponent : PaintableComponents)
			{
				if (UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(MeshComponent))
				{
					UE_LOG(LogUnrealCV, Log, TEXT("Paint StaticMeshComponent: %s"), *Actor->GetHumanReadableName());

					StaticMeshComponent->SetRenderCustomDepth(true);
					StaticMeshComponent->SetCustomDepthStencilValue(0);
				}
				if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
				{
					UE_LOG(LogUnrealCV, Log, TEXT("Paint SkeletalMeshComponent: %s (%d)"), *Actor->GetHumanReadableName(), ObjectIndex);

					SkeletalMeshComponent->SetRenderCustomDepth(true);
					SkeletalMeshComponent->SetCustomDepthStencilValue(ObjectIndex);
					ObjectIndex = (ObjectIndex + 1)%256;
				}
			}
		}
	}
	// end
}

void AUE4ROSBridgeManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	//UE_LOG(LogUnrealCV, Log, TEXT("AUE4ROSBridgeManager::Tick([%d-%d])"), GFrameCounter, GFrameNumber);

	// Do something

	// publish clock
	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable(new rosgraph_msgs::Clock(GetROSSimTime()));
	ROSHandler->PublishMsg("/clock", Clock);

	ROSHandler->Process();

	if (IsHit) {
		this->ResetCharacterPoses();
		IsHit = false;
		EpisodeDone = 1.0;
	}
	std_msgs::Header Header(0, GetROSSimTime(), "odom");
	TArray<geometry_msgs::Point32> Points = {geometry_msgs::Point32(static_cast<float>(GFrameCounter), EpisodeDone, Reward),};
	TSharedPtr<geometry_msgs::PolygonStamped> status = MakeShareable(new geometry_msgs::PolygonStamped(Header, Points));
	ROSHandler->PublishMsg("/ue4/status", status);
	Reward = 0.0f;
	EpisodeDone = 0.0f;
}

void AUE4ROSBridgeManager::EndPlay(const EEndPlayReason::Type Reason)
{
	UE_LOG(LogUnrealCV, Log, TEXT("AUE4ROSBridgeManager::::EndPlay()"));

	for (auto Elem : ActorList)
	{
		Elem->RemoveFromRoot();
		Elem->ConditionalBeginDestroy();
	}
	ActorList.Empty();
	for (auto Elem : CaptureComponentList)
	{
		Elem->RemoveFromRoot();
		Elem->ConditionalBeginDestroy();
	}
	CaptureComponentList.Empty();

	ROSHandler->Disconnect();
	// Disconnect the handler before parent ends

	GEngine->Exec(GetWorld(), TEXT("t.MaxFPS 0"));

	Super::EndPlay(Reason);
}
