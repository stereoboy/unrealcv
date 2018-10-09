#include "UnrealCVPrivate.h"
#include "UE4ROSBridgeManager.h"
#include "rosgraph_msgs/Clock.h"

void AUE4ROSBridgeManager::BeginPlay()
{
	Super::BeginPlay();
	// Set websocket server address to ws://127.0.0.1:9001
	ROSHandler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_MASTER_PORT));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****

	// Connect to ROSBridge Websocket server.
	ROSHandler->Connect();
}

void AUE4ROSBridgeManager::PublishReward()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Blue, TEXT("[INFO] Publish Reward!"));
}

void AUE4ROSBridgeManager::AttachCaptureComponentToCamera(APawn* Pawn)
{
	// TODO: Only support one camera at the beginning
	// TODO: Make this automatic from material loader.
	// TODO: Get the list from GTCaptureComponent

	ActorList.Empty();
	CaptureComponentList.Empty();

	URemoteMovementComponent* actor = URemoteMovementComponent::Create(FName(TEXT("Main_Actor")), Pawn);
	ActorList.Add(actor);

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
		}
		else if (camera->GetName().Compare(TEXT("RGBDCamRGB")) == 0)
		{
			TArray<FString> SupportedModes = {TEXT("lit")};
			UGTCameraCaptureComponent* camCom = UGTCameraCaptureComponent::Create(FName(*camera->GetName()), Pawn, camera->GetOwner(), camera, SupportedModes);
			camCom->SetUROSBridge(TEXT("/ue4"), TEXT("rgbd_cam"), TEXT("rgbd_rgb_img_link"));
			CaptureComponentList.Add(camCom);
			actor->AddCaptureComponent(camCom);
		}
		else if (camera->GetName().Compare(TEXT("RGBDCamDepth")) == 0)
		{
			TArray<FString> SupportedModes = {TEXT("depth")};
			UGTCameraCaptureComponent* camCom = UGTCameraCaptureComponent::Create(FName(*camera->GetName()), Pawn, camera->GetOwner(), camera, SupportedModes);
			camCom->SetUROSBridge(TEXT("/ue4"), TEXT("rgbd_cam"), TEXT("rgbd_depth_img_link"));
			CaptureComponentList.Add(camCom);
			actor->AddCaptureComponent(camCom);
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
	// Do something

//	float GameTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
//	uint64 GameSeconds = (int)GameTime;
//	uint64 GameUseconds = (GameTime - GameSeconds) * 1000000000;
//	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable
//	(new rosgraph_msgs::Clock(FROSTime(GameSeconds, GameUseconds)));
//	ROSHandler->PublishMsg("clock", Clock);

	ROSHandler->Process();
}

void AUE4ROSBridgeManager::EndPlay(const EEndPlayReason::Type Reason)
{
	for (auto Elem : ActorList)
	{
		Elem->ConditionalBeginDestroy();
	}
	ActorList.Empty();
	for (auto Elem : CaptureComponentList)
	{
		Elem->ConditionalBeginDestroy();
	}
	CaptureComponentList.Empty();

	ROSHandler->Disconnect();
	// Disconnect the handler before parent ends

	Super::EndPlay(Reason);
}
