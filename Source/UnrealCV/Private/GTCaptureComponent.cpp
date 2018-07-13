#include "UnrealCVPrivate.h"
#include "GTCaptureComponent.h"
#include "ViewMode.h"
#include "Serialization.h"
#include "UE4ROSBridgeManager.h"
#include "Serialization.h"
#include "std_msgs/Float32.h"
#include "std_msgs/Header.h"
#include "geometry_msgs/TransformStamped.h"
#include "sensor_msgs/CameraInfo.h"
#include "sensor_msgs/CompressedImage.h"
#include "ROSHelper.h"
#include "ROSDebug.h"

DECLARE_CYCLE_STAT(TEXT("SaveExr"), STAT_SaveExr, STATGROUP_UnrealCV);
DECLARE_CYCLE_STAT(TEXT("SavePng"), STAT_SavePng, STATGROUP_UnrealCV);
DECLARE_CYCLE_STAT(TEXT("SaveFile"), STAT_SaveFile, STATGROUP_UnrealCV);
DECLARE_CYCLE_STAT(TEXT("ReadPixels"), STAT_ReadPixels, STATGROUP_UnrealCV);
DECLARE_CYCLE_STAT(TEXT("ImageWrapper"), STAT_ImageWrapper, STATGROUP_UnrealCV);
DECLARE_CYCLE_STAT(TEXT("GetResource"), STAT_GetResource, STATGROUP_UnrealCV);

void InitCaptureComponent(USceneCaptureComponent2D* CaptureComponent)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	// Can not use ESceneCaptureSource::SCS_SceneColorHDR, this option will disable post-processing
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	CaptureComponent->TextureTarget = NewObject<UTextureRenderTarget2D>();
	FServerConfig& Config = FUE4CVServer::Get().Config;
	CaptureComponent->TextureTarget->InitAutoFormat(Config.Width, Config.Height);

	/*
	UGameViewportClient* GameViewportClient = World->GetGameViewport();
	CaptureComponent->TextureTarget->InitAutoFormat(GameViewportClient->Viewport->GetSizeXY().X,  GameViewportClient->Viewport->GetSizeXY().Y); // TODO: Update this later
	*/

	CaptureComponent->RegisterComponentWithWorld(World); // What happened for this?
	// CaptureComponent->AddToRoot(); This is not necessary since it has been attached to the Pawn.
}


void SaveExr(UTextureRenderTarget2D* RenderTarget, FString Filename)
{
	SCOPE_CYCLE_COUNTER(STAT_SaveExr)
	int32 Width = RenderTarget->SizeX, Height = RenderTarget->SizeY;
	TArray<FFloat16Color> FloatImage;
	FloatImage.AddZeroed(Width * Height);
	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	RenderTargetResource->ReadFloat16Pixels(FloatImage);

	TArray<uint8> ExrData = SerializationUtils::Image2Exr(FloatImage, Width, Height);
	FFileHelper::SaveArrayToFile(ExrData, *Filename);
}

void SavePng(UTextureRenderTarget2D* RenderTarget, FString Filename)
{
	SCOPE_CYCLE_COUNTER(STAT_SavePng);

	int32 Width = RenderTarget->SizeX, Height = RenderTarget->SizeY;
	TArray<FColor> Image;
	FTextureRenderTargetResource* RenderTargetResource;
	Image.AddZeroed(Width * Height);
	{
		SCOPE_CYCLE_COUNTER(STAT_GetResource);
		RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	}
	{
		SCOPE_CYCLE_COUNTER(STAT_ReadPixels);
		FReadSurfaceDataFlags ReadSurfaceDataFlags;
		ReadSurfaceDataFlags.SetLinearToGamma(false); // This is super important to disable this!
		// Instead of using this flag, we will set the gamma to the correct value directly
		RenderTargetResource->ReadPixels(Image, ReadSurfaceDataFlags);
	}
	TArray<uint8> ImgData = SerializationUtils::Image2Png(Image, Width, Height);
	FFileHelper::SaveArrayToFile(ImgData, *Filename);
}

TSharedPtr<sensor_msgs::CameraInfo> BuildROSCameraInfo(std_msgs::Header Header,int32 Width, int32 Height, double FOV)
{
	TArray<double> D;
	TArray<double> K;
	TArray<double> R;
	TArray<double> P;
	sensor_msgs::RegionOfInterest ROI(0, 0, Height, Width, false);

	double fx = 0.5*Width/FMath::Tan(FMath::DegreesToRadians(FOV)*0.5);
	double fy = fx;
	double cx = 0.5*Width;
	double cy = 0.5*Height;
	double ArrayD[] = {0.0, 0.0, 0.0, 0.0, 0.0,};
	double ArrayK[] = {	fx, 0.0, cx,
						0.0, fy, cy,
						0.0, 0.0, 1.0};
	double ArrayR[] = {	1.0, 0.0, 0.0,
						0.0, 1.0, 0.0,
						0.0, 0.0, 1.0};
	double ArrayP[] = { fx, 0.0, cx, 0.0,
						0.0, fy, cy, 0.0,
						0.0, 0.0, 1.0, 0.0 };

	D.Append(ArrayD, ARRAY_COUNT(ArrayD));
	K.Append(ArrayK, ARRAY_COUNT(ArrayK));
	R.Append(ArrayR, ARRAY_COUNT(ArrayR));
	P.Append(ArrayP, ARRAY_COUNT(ArrayP));

	TSharedPtr<sensor_msgs::CameraInfo> OutCameraInfo = MakeShareable(new sensor_msgs::CameraInfo(
		Header, Height, Width, TEXT("plumb_bob"),  D, K, R, P, 0, 0, ROI
	));

	return OutCameraInfo;
}

UMaterial* UGTCaptureComponent::GetMaterial(FString InModeName = TEXT(""))
{
	// Load material for visualization
	static TMap<FString, FString>* MaterialPathMap = nullptr;
	if (MaterialPathMap == nullptr)
	{
		MaterialPathMap = new TMap<FString, FString>();
		MaterialPathMap->Add(TEXT("depth"), TEXT("Material'/UnrealCV/SceneDepthWorldUnits.SceneDepthWorldUnits'"));
		MaterialPathMap->Add(TEXT("vis_depth"), TEXT("Material'/UnrealCV/SceneDepth.SceneDepth'"));
		MaterialPathMap->Add(TEXT("debug"), TEXT("Material'/UnrealCV/debug.debug'"));
		// MaterialPathMap->Add(TEXT("object_mask"), TEXT("Material'/UnrealCV/VertexColorMaterial.VertexColorMaterial'"));
		MaterialPathMap->Add(TEXT("normal"), TEXT("Material'/UnrealCV/WorldNormal.WorldNormal'"));

		FString OpaqueMaterialName = "Material'/UnrealCV/OpaqueMaterial.OpaqueMaterial'";
		MaterialPathMap->Add(TEXT("opaque"), OpaqueMaterialName);
	}

	static TMap<FString, UMaterial*>* StaticMaterialMap = nullptr;
	if (StaticMaterialMap == nullptr)
	{
		StaticMaterialMap = new TMap<FString, UMaterial*>();
		for (auto& Elem : *MaterialPathMap)
		{
			FString ModeName = Elem.Key;
			FString MaterialPath = Elem.Value;
			ConstructorHelpers::FObjectFinder<UMaterial> Material(*MaterialPath); // ConsturctorHelpers is only available for UObject.

			if (Material.Object != NULL)
			{
				StaticMaterialMap->Add(ModeName, (UMaterial*)Material.Object);
			}
		}
	}

	UMaterial* Material = StaticMaterialMap->FindRef(InModeName);
	if (Material == nullptr)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Can not recognize visualization mode %s"), *InModeName);
	}
	return Material;
}

/**
Attach a GTCaptureComponent to a pawn
 */
UGTCaptureComponent* UGTCaptureComponent::Create(FName Name, APawn* InPawn, TArray<FString> Modes)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	UGTCaptureComponent* GTCapturer = NewObject<UGTCaptureComponent>((UObject *)GetTransientPackage(), Name);

	GTCapturer->bIsActive = true;
	// check(GTCapturer->IsComponentTickEnabled() == true);
	GTCapturer->Pawn = InPawn; // This GTCapturer should depend on the Pawn and be released together with the Pawn.

	// This snippet is from Engine/Source/Runtime/Engine/Private/Components/SceneComponent.cpp, AttachTo
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
	ConvertAttachLocation(EAttachLocation::KeepRelativeOffset, AttachmentRules.LocationRule, AttachmentRules.RotationRule, AttachmentRules.ScaleRule);
	GTCapturer->AttachToComponent(InPawn->GetRootComponent(), AttachmentRules);
	// GTCapturer->AddToRoot();
	GTCapturer->RegisterComponentWithWorld(World);
	GTCapturer->SetTickableWhenPaused(true);

	for (FString Mode : Modes)
	{
		// DEPRECATED_FORGAME(4.6, "CaptureComponent2D should not be accessed directly, please use GetCaptureComponent2D() function instead. CaptureComponent2D will soon be private and your code will not compile.")
		USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>();

		CaptureComponent->bIsActive = false; // Disable it by default for performance consideration
		GTCapturer->CaptureComponents.Add(Mode, CaptureComponent);

		// CaptureComponent needs to be attached to somewhere immediately, otherwise it will be gc-ed

		CaptureComponent->AttachToComponent(GTCapturer, AttachmentRules);
		InitCaptureComponent(CaptureComponent);

		UMaterial* Material = GetMaterial(Mode);
		if (Mode == "lit") // For rendered images
		{
			// FViewMode::Lit(CaptureComponent->ShowFlags);
			CaptureComponent->TextureTarget->TargetGamma = GEngine->GetDisplayGamma();
			// float DisplayGamma = SceneViewport->GetDisplayGamma();
		}
		else if (Mode == "default")
		{
			continue;
		}
		else // for ground truth
		{
			CaptureComponent->TextureTarget->TargetGamma = 1;
			if (Mode == "object_mask") // For object mask
			{
				// FViewMode::Lit(CaptureComponent->ShowFlags);
				FViewMode::VertexColor(CaptureComponent->ShowFlags);
			}
			else if (Mode == "wireframe") // For object mask
			{
				FViewMode::Wireframe(CaptureComponent->ShowFlags);
			}
			else
			{
				check(Material);
				// GEngine->GetDisplayGamma(), the default gamma is 2.2
				// CaptureComponent->TextureTarget->TargetGamma = 2.2;
				FViewMode::PostProcess(CaptureComponent->ShowFlags);

				CaptureComponent->PostProcessSettings.AddBlendable(Material, 1);
				// Instead of switching post-process materials, we create several SceneCaptureComponent, so that we can capture different GT within the same frame.
			}
		}
	}
	return GTCapturer;
}

UGTCaptureComponent::UGTCaptureComponent()
{
	GetMaterial(); // Initialize the TMap
	PrimaryComponentTick.bCanEverTick = true;
	// bIsTicking = false;

	// Create USceneCaptureComponent2D
	// Design Choice: do I need one capture component with changing post-process materials or multiple components?
	// USceneCaptureComponent2D* CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("GTCaptureComponent"));
}

// Each GTCapturer can serve as one camera of the scene

FAsyncRecord* UGTCaptureComponent::Capture(FString Mode, FString InFilename)
{
	// Flush location and rotation

	check(CaptureComponents.Num() != 0);
	USceneCaptureComponent2D* CaptureComponent = CaptureComponents.FindRef(Mode);
	if (CaptureComponent == nullptr)
		return nullptr;

	const FRotator PawnViewRotation = Pawn->GetViewRotation();
	if (!PawnViewRotation.Equals(CaptureComponent->GetComponentRotation()))
	{
		CaptureComponent->SetWorldRotation(PawnViewRotation);
	}

	FAsyncRecord* AsyncRecord = FAsyncRecord::Create();
	FGTCaptureTask GTCaptureTask = FGTCaptureTask(Mode, InFilename, GFrameCounter, AsyncRecord);
	this->PendingTasks.Enqueue(GTCaptureTask);

	return AsyncRecord;
}

void UGTCaptureComponent::CaptureImage(const FString& Mode, TArray<FColor>& OutImageData, int32& OutWidth, int32& OutHeight)
{
	// Flush location and rotation
	USceneCaptureComponent2D* CaptureComponent = CaptureComponents.FindRef(Mode);
	if (CaptureComponent == nullptr)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Can not find a camera to capture %s"), *Mode);
		OutWidth = 0; OutHeight = 0;
		return;
	}

	UTextureRenderTarget2D* RenderTarget = CaptureComponent->TextureTarget;
	FTextureRenderTargetResource* RenderTargetResource;
	OutImageData.Empty();
	OutImageData.AddZeroed(RenderTarget->SizeX * RenderTarget->SizeY);
	RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

	FReadSurfaceDataFlags ReadSurfaceDataFlags;
	ReadSurfaceDataFlags.SetLinearToGamma(false); // This is super important to disable this!
	// Instead of using this flag, we will set the gamma to the correct value directly
	if (!RenderTargetResource->ReadPixels(OutImageData, ReadSurfaceDataFlags))
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("ReadPixels() failed: %s"), *Mode);
		OutWidth = 0; OutHeight = 0;
		return;
	}
	OutWidth = RenderTarget->SizeX;
	OutHeight = RenderTarget->SizeY;
}

void UGTCaptureComponent::CaptureFloat16Image(const FString& Mode, TArray<FFloat16Color>& OutImageData, int32& OutWidth, int32& OutHeight)
{
	USceneCaptureComponent2D* CaptureComponent = CaptureComponents.FindRef(Mode);
	if (CaptureComponent == nullptr)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Can not find a camera to capture %s"), *Mode);
		OutWidth = 0; OutHeight = 0;
		return;
	}

	UTextureRenderTarget2D* RenderTarget = CaptureComponent->TextureTarget;

	FTextureRenderTargetResource* RenderTargetResource;
	OutImageData.Empty();
	OutImageData.AddZeroed(RenderTarget->SizeX * RenderTarget->SizeY);
	RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();

	RenderTargetResource->ReadFloat16Pixels(OutImageData);
	OutWidth = RenderTarget->SizeX;
	OutHeight = RenderTarget->SizeY;
	return;
}

void UGTCaptureComponent::GetFieldOfView(const FString& Mode, float& FOV)
{
	USceneCaptureComponent2D* CaptureComponent = CaptureComponents.FindRef(Mode);
	if (CaptureComponent == nullptr)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Can not find a camera to capture %s"), *Mode);
		return;
	}

	FOV = CaptureComponent->FOVAngle;
	return;
}

void UGTCaptureComponent::SetTargetFieldOfView(const float TargetFOV)
{
	this->TargetFOVAngle = FMath::Clamp<float>(TargetFOV, PARAM_MIN_CAM_ZOOM_FOV, PARAM_MAX_CAM_ZOOM_FOV);
	this->IsZoomToTarget = true;
}

void UGTCaptureComponent::GetSize(const FString& Mode, int32& Width, int32& Height)
{
	USceneCaptureComponent2D* CaptureComponent = CaptureComponents.FindRef(Mode);
	if (CaptureComponent == nullptr)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("Can not find a camera to capture %s"), *Mode);
		return;
	}

	Width = CaptureComponent->TextureTarget->SizeX;
	Height = CaptureComponent->TextureTarget->SizeY;
	return;
}

void UGTCaptureComponent::SetVelocity(const FTransform& InVelocity)
{
	this->Velocity = InVelocity;
	this->MoveAnimCountFromRemote = PARAM_MOVE_ANIM_COUNT;
}

void UGTCaptureComponent::SetTargetPose(const FTransform& InTargetPose)
{
	this->TargetPose = InTargetPose;
	this->IsMoveToTarget = true;
}

void UGTCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// void UGTCaptureComponent::Tick(float DeltaTime) // This tick function should be called by the scene instead of been
{
	// Render pixels out in the next tick. To allow time to render images out.

	// Update rotation of each frame
	// from ab237f46dc0eee40263acbacbe938312eb0dffbb:CameraComponent.cpp:232
	check(this->Pawn); // this GTCapturer should be released, if the Pawn is deleted.
	const APawn* OwningPawn = this->Pawn;
	const AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		if (this->MoveAnimCountFromRemote > 0)
		{
			FVector DeltaXYZ = this->Velocity.GetLocation()*DeltaTime;
			FRotator Rot = this->Velocity.Rotator();

			this->Pawn->AddMovementInput(DeltaXYZ);
			this->Pawn->AddControllerPitchInput(Rot.Pitch*DeltaTime);
			this->Pawn->AddControllerYawInput(Rot.Yaw*DeltaTime);
			this->Pawn->AddControllerRollInput(Rot.Roll*DeltaTime);

			this->MoveAnimCountFromRemote--;
		}

		const FRotator PawnViewRotation = OwningPawn->GetViewRotation();
		for (auto Elem : CaptureComponents)
		{
			USceneCaptureComponent2D* CaptureComponent = Elem.Value;
			if (!PawnViewRotation.Equals(CaptureComponent->GetComponentRotation()))
			{
				CaptureComponent->SetWorldRotation(PawnViewRotation);
			}
		}
	}

	while (!PendingTasks.IsEmpty())
	{
		FGTCaptureTask Task;
		PendingTasks.Peek(Task);
		uint64 CurrentFrame = GFrameCounter;

		int32 SkipFrame = 1;
		if (!(CurrentFrame > Task.CurrentFrame + SkipFrame)) // TODO: This is not an elegant solution, fix it later.
		{ // Wait for the rendering thread to catch up game thread.
			break;
		}

		PendingTasks.Dequeue(Task);
		USceneCaptureComponent2D* CaptureComponent = this->CaptureComponents.FindRef(Task.Mode);
		if (CaptureComponent == nullptr)
		{
			UE_LOG(LogUnrealCV, Warning, TEXT("Unrecognized capture mode %s"), *Task.Mode);
		}
		else
		{
			FString LowerCaseFilename = Task.Filename.ToLower();
			if (LowerCaseFilename.EndsWith("png"))
			{
				SavePng(CaptureComponent->TextureTarget, Task.Filename);
			}
			else if (LowerCaseFilename.EndsWith("exr"))
			{
				SaveExr(CaptureComponent->TextureTarget, Task.Filename);
			}
			else
			{
				UE_LOG(LogUnrealCV, Warning, TEXT("Unrecognized image file extension %s"), *LowerCaseFilename);
			}
		}
		Task.AsyncRecord->bIsCompleted = true;
	}
}

UGTCameraCaptureComponent::FROSFloat32SubScriber::FROSFloat32SubScriber(const FString& InTopic, UGTCameraCaptureComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("std_msgs/Float32"))
{
	this->Component = Component;
}

UGTCameraCaptureComponent::FROSFloat32SubScriber::~FROSFloat32SubScriber()
{
};

TSharedPtr<FROSBridgeMsg> UGTCameraCaptureComponent::FROSFloat32SubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<std_msgs::Float32> Message = MakeShareable<std_msgs::Float32>(new std_msgs::Float32());
	Message->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(Message);
}

void UGTCameraCaptureComponent::FROSFloat32SubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<std_msgs::Float32> Message = StaticCastSharedPtr<std_msgs::Float32>(InMsg);

	// do something with the message
	float FOV = Message->GetData();
	this->Component->SetTargetFieldOfView(FOV);
	return;
}

UGTCameraCaptureComponent* UGTCameraCaptureComponent::Create(FName Name, APawn* InPawn, AActor* InCameraActor, UCameraComponent* InCameraComp, TArray<FString> Modes)
{
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	// in general cases, InPawn == InCameraActor if the camera compoment is a child of main pawn
	// camera capture component should be a child of camera component
	// without Outer setup BeginPlay() will not be called
	UGTCameraCaptureComponent* GTCapturer = NewObject<UGTCameraCaptureComponent>(InCameraComp, FName(*(Name.ToString() + TEXT("_Capture"))));

	AActor * test = GTCapturer->GetOwner();
	//GTCapturer->bIsActive = true;
	GTCapturer->SetActive(true);
	// check(GTCapturer->IsComponentTickEnabled() == true);
	GTCapturer->Pawn = InPawn;
	GTCapturer->CameraActor = InCameraActor; // This GTCapturer should depend on the Pawn and be released together with the Pawn.
	GTCapturer->CameraComponent = InCameraComp;

	// This snippet is from Engine/Source/Runtime/Engine/Private/Components/SceneComponent.cpp, AttachTo
	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, false);
	ConvertAttachLocation(EAttachLocation::KeepRelativeOffset, AttachmentRules.LocationRule, AttachmentRules.RotationRule, AttachmentRules.ScaleRule);
	GTCapturer->AttachToComponent(InCameraActor->GetRootComponent(), AttachmentRules);

	GTCapturer->AddToRoot();
	GTCapturer->RegisterComponent();
	GTCapturer->SetTickableWhenPaused(true);

	for (FString Mode : Modes)
	{
		// DEPRECATED_FORGAME(4.6, "CaptureComponent2D should not be accessed directly, please use GetCaptureComponent2D() function instead. CaptureComponent2D will soon be private and your code will not compile.")
		USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>();

		CaptureComponent->bIsActive = false; // Disable it by default for performance consideration
		//CaptureComponent->SetVisibility(false);
		GTCapturer->CaptureComponents.Add(Mode, CaptureComponent);

		// CaptureComponent needs to be attached to somewhere immediately, otherwise it will be gc-ed

		CaptureComponent->AttachToComponent(GTCapturer, AttachmentRules);
		InitCaptureComponent(CaptureComponent);

		UMaterial* Material = GetMaterial(Mode);
		if (Mode == "lit") // For rendered images
		{
			// FViewMode::Lit(CaptureComponent->ShowFlags);
			CaptureComponent->TextureTarget->TargetGamma = GEngine->GetDisplayGamma();
			// float DisplayGamma = SceneViewport->GetDisplayGamma();
		}
		else if (Mode == "default")
		{
			continue;
		}
		else // for ground truth
		{
			CaptureComponent->TextureTarget->TargetGamma = 1;
			if (Mode == "object_mask") // For object mask
			{
#if 0
				// FViewMode::Lit(CaptureComponent->ShowFlags);
				FViewMode::VertexColor(CaptureComponent->ShowFlags);
#else
				check(Material);
				// GEngine->GetDisplayGamma(), the default gamma is 2.2
				// CaptureComponent->TextureTarget->TargetGamma = 2.2;
				FViewMode::PostProcess(CaptureComponent->ShowFlags);

				CaptureComponent->PostProcessSettings.AddBlendable(Material, 1);
				// Instead of switching post-process materials, we create several SceneCaptureComponent, so that we can capture different GT within the same frame.
#endif
			}
			else if (Mode == "wireframe") // For object mask
			{
				FViewMode::Wireframe(CaptureComponent->ShowFlags);
			}
			else
			{
				check(Material);
				// GEngine->GetDisplayGamma(), the default gamma is 2.2
				// CaptureComponent->TextureTarget->TargetGamma = 2.2;
				FViewMode::PostProcess(CaptureComponent->ShowFlags);

				CaptureComponent->PostProcessSettings.AddBlendable(Material, 1);
				// Instead of switching post-process materials, we create several SceneCaptureComponent, so that we can capture different GT within the same frame.
			}
		}
	}

	// Initialize Rotation, Location, FOV
	const FRotator CameraViewRotation = GTCapturer->CameraComponent->GetComponentRotation();
	const FVector CameraViewLocation = GTCapturer->CameraComponent->GetComponentLocation();
	const float CameraFieldOfView = GTCapturer->CameraComponent->FieldOfView;
	for (auto Elem : GTCapturer->CaptureComponents)
	{
		USceneCaptureComponent2D* CaptureComponent = Elem.Value;
		if (!CameraViewRotation.Equals(CaptureComponent->GetComponentRotation()))
		{
			CaptureComponent->SetWorldRotation(CameraViewRotation);
		}
		if (!CameraViewLocation.Equals(CaptureComponent->GetComponentLocation()))
		{
			CaptureComponent->SetWorldLocation(CameraViewLocation);
		}
		if (CameraFieldOfView != CaptureComponent->FOVAngle)
		{
			CaptureComponent->FOVAngle = CameraFieldOfView;
		}

	}
	UE_LOG(LogUnrealCV, Log, TEXT("%s initialized!"), *GTCapturer->CameraComponent->GetName());
	return GTCapturer;
}

void UGTCameraCaptureComponent::SetUROSBridge(FString InROSNamespace, FString InROSName, FString InROSFrameId)
{
	ROSNamespace = InROSNamespace;
	ROSName = InROSName;
	ROSFrameId = InROSFrameId;

	ROSTopic = InROSNamespace + TEXT("/") + ROSName;

	// Set websocket server address to ws://127.0.0.1:9001
	ROSHandler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_MASTER_PORT));
	ROSFastHandler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_FAST_MASTER_PORT));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****
	//TSharedPtr<FROSBridgePublisher> TfPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic + TEXT("/loc"), TEXT("geometry_msgs/TransformStamped")));
	//ROSHandler->AddPublisher(TfPublisher);

	for (auto Elem : CaptureComponents)
	{
		if (Elem.Key.Compare(TEXT("lit")) == 0)
		{
			TSharedPtr<FROSBridgePublisher> ImageCameraInfoPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic + TEXT("/rgb/camera_info"), TEXT("sensor_msgs/CameraInfo")));
			ROSHandler->AddPublisher(ImageCameraInfoPublisher);
			//TSharedPtr<FROSBridgePublisher> ImagePublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic + TEXT("/rgb/image/compressed"), TEXT("sensor_msgs/CompressedImage")));
			//ROSHandler->AddPublisher(ImagePublisher);

		}
		else if (Elem.Key.Compare(TEXT("depth")) == 0)
		{
			TSharedPtr<FROSBridgePublisher> DepthCameraInfoPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic + TEXT("/depth/camera_info"), TEXT("sensor_msgs/CameraInfo")));
			ROSHandler->AddPublisher(DepthCameraInfoPublisher);
			//TSharedPtr<FROSBridgePublisher> DepthPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(ROSTopic + TEXT("/depth/image"), TEXT("sensor_msgs/Image")));
			//ROSHandler->AddPublisher(DepthPublisher);
		}
	}

	// Add topic subscribers and publishers
	TSharedPtr<FROSFloat32SubScriber> Subscriber = MakeShareable<FROSFloat32SubScriber>(new FROSFloat32SubScriber(ROSTopic + TEXT("/ctrl/fov"), this));
	ROSHandler->AddSubscriber(Subscriber);

	// Connect to ROSBridge Websocket server.
	ROSHandler->Connect();
	ROSFastHandler->Connect();

	// Setup Initial Tranform for remote control in the future
	InitialTransform = this->CameraComponent->GetRelativeTransform();
}

void UGTCameraCaptureComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UGTCameraCaptureComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ROSHandler->Disconnect();
	ROSFastHandler->Disconnect();
	// Disconnect the ROSHandler before parent ends

	Super::EndPlay(Reason);
}

void ConvertByte(TArray<uint8>& OutBinaryArray, FString Message)
{
	FTCHARToUTF8 Convert(*Message);
	OutBinaryArray.Empty();
	OutBinaryArray.Append((UTF8CHAR*)Convert.Get(), Convert.Length());
}

void PackFastMsgStrEntity(TArray<uint8>& ByteData, FString Elem, int32 Size)
{
	TArray<uint8> TypeData;
	TArray<uint8> Temp;
	TypeData.AddZeroed(Size);
	ConvertByte(Temp, Elem);
	FMemory::Memcpy(TypeData.GetData(), Temp.GetData(), FMath::Min(Size, Temp.Num()));
	ByteData.Append(TypeData);
}

void UGTCameraCaptureComponent::PackFastMsgHeader(TArray<uint8> &ByteData, FString Topic, FString MsgType, FString Type, FROSTime Stamp, FString Name, FString FrameId, uint32 Height, uint32 Width)
{
	if (!this->ROSFastMsgHeaderCache.Contains(Type))
	{
		TArray<uint8> TempData;
		PackFastMsgStrEntity(TempData, Topic, 64);
		PackFastMsgStrEntity(TempData, MsgType, 32);
		PackFastMsgStrEntity(TempData, Type, 32);
		PackFastMsgStrEntity(TempData, Name, 32);
		PackFastMsgStrEntity(TempData, FrameId, 32);

		TempData.Append((uint8*)&Height, sizeof(uint32));
		TempData.Append((uint8*)&Width, sizeof(uint32));

		this->ROSFastMsgHeaderCache.Add(Type, TempData);
	}
	
	ByteData.Append(ROSFastMsgHeaderCache[Type]);
	ByteData.Append((uint8*)&Stamp.Secs, sizeof(uint32));
	ByteData.Append((uint8*)&Stamp.NSecs, sizeof(uint32));
}

void UGTCameraCaptureComponent::PublishImage(void)
{
	ROSDBG_TC_INIT();

	ROSDBG_TC_BEGIN();
	double FOV = this->CameraComponent->FieldOfView;

	FROSTime ROSTime = FROSTime();
	TArray<FColor> ImageData;
	int32 Height = 0, Width = 0;
	this->CaptureImage("lit", ImageData, Width, Height);
	ROSDBG_TC_END(LogUnrealCV, *ROSTopic, " RGB Capturing");

	if (ImageData.Num() != 0)
	{
		// publish camera_info
		std_msgs::Header Header(0, ROSTime, ROSFrameId);

		TSharedPtr<sensor_msgs::CameraInfo> CameraInfo = BuildROSCameraInfo(Header, Width, Height, FOV);
		ROSHandler->PublishMsg(ROSTopic + TEXT("/rgb/camera_info"), CameraInfo);

    //UE_LOG(LogUnrealCV, Error, TEXT("RGB COLOR: R(%x) G(%x) B(%x) A(%x)"), ImageData[0].R, ImageData[0].G, ImageData[0].B, ImageData[0].A);

		TArray<uint8> ByteData;
		PackFastMsgHeader(ByteData, ROSTopic + TEXT("/rgb/image"), TEXT("Image"), TEXT("RGB"), ROSTime, this->ROSName, this->ROSFrameId, Height, Width);
		ByteData.Append((uint8*)ImageData.GetData(), sizeof(FColor)*ImageData.Num());

		ROSFastHandler->PublishFastMsg(ROSTopic + TEXT("/rgb/image"), ByteData);

		//UE_LOG(LogUnrealCV, Error, TEXT("RGB Num: %d, ByteData: %x %x %x %x"), ByteData.Num(), ByteData[0], ByteData[1], ByteData[2], ByteData[3]);

		// publish images
#if 0
		// https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/TArrays/
		TArray<uint8> BinaryData;
		BinaryData.AddUninitialized(Width*Height * sizeof(FColor));
		FMemory::Memcpy(BinaryData.GetData(), ImageData.GetData(), Width*Height * sizeof(FColor));

		// Record end time
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;
		UE_LOG(LogUnrealCV, Error, TEXT("Img Capturing elapsed time: %fs"), elapsed.count());

		// start time
		start = std::chrono::high_resolution_clock::now();
#if 0
		TSharedPtr<sensor_msgs::Image> Image =
			MakeShareable(new sensor_msgs::Image(
				Header, Height, Width, TEXT("rgba8")/*4Channel*/, true,
				Width * 4, BinaryData)
			);
#else
		TArray<uint8> PngBinaryData = SerializationUtils::Image2Png(ImageData, Width, Height);
		TSharedPtr<sensor_msgs::CompressedImage> Image = MakeShareable(new sensor_msgs::CompressedImage(Header, TEXT("png"), PngBinaryData));
#endif

		// end time
		finish = std::chrono::high_resolution_clock::now();
		elapsed = finish - start;
		UE_LOG(LogUnrealCV, Error, TEXT("ROS Msg Building elapsed time: %fs"), elapsed.count());

		// start time
		start = std::chrono::high_resolution_clock::now();

		//ROSHandler->PublishMsg(ROSTopic + TEXT("/rgb/image/compressed"), Image);

		// end time
		finish = std::chrono::high_resolution_clock::now();
		elapsed = finish - start;
		UE_LOG(LogUnrealCV, Error, TEXT("ROS Msg Publishing elapsed time: %fs"), elapsed.count());
#endif
	}
}

void UGTCameraCaptureComponent::PublishDepth(void)
{
	ROSDBG_TC_INIT();

	ROSDBG_TC_BEGIN();
	float FOV = this->CameraComponent->FieldOfView;

	FROSTime ROSTime = FROSTime();
	TArray<FFloat16Color> FloatImageData;
	int32 Height = 0, Width = 0;
	this->CaptureFloat16Image("depth", FloatImageData, Width, Height);
	ROSDBG_TC_END(LogUnrealCV, *ROSTopic, "Depth Capturing");

	if (FloatImageData.Num() != 0)
	{
		// publish camera_info
		std_msgs::Header Header(0, ROSTime, ROSFrameId);

		TSharedPtr<sensor_msgs::CameraInfo> CameraInfo = BuildROSCameraInfo(Header, Width, Height, FOV);
		ROSHandler->PublishMsg(ROSTopic + TEXT("/depth/camera_info"), CameraInfo);

		//UE_LOG(LogUnrealCV, Error, TEXT("ByteData: %f %f %f %f"), FloatImageData[0], FloatImageData[1], FloatImageData[2], FloatImageData[3]);

		TArray<uint8> ByteData;
		PackFastMsgHeader(ByteData, ROSTopic + TEXT("/depth/image"), TEXT("Image"), TEXT("Depth"), ROSTime, this->ROSName, this->ROSFrameId, Height, Width);
		ByteData.Append((uint8*)FloatImageData.GetData(), sizeof(FFloat16Color)*FloatImageData.Num());

		ROSFastHandler->PublishFastMsg(ROSTopic + TEXT("/depth/image"), ByteData);

		//UE_LOG(LogUnrealCV, Error, TEXT("sizeof(FFloat16Color): %d"), sizeof(FFloat16Color));
		//UE_LOG(LogUnrealCV, Error, TEXT("Depth Num: %d ByteData: %x %x %x %x"), ByteData.Num(), ByteData[0], ByteData[1], ByteData[2], ByteData[3]);
		//ROSHandler->PublissMsg(ROSTopic + TEXT("/depth/image"), Image);
	}
}

void UGTCameraCaptureComponent::PublishLabel(void)
{
	ROSDBG_TC_INIT();

	ROSDBG_TC_BEGIN();
	double FOV = this->CameraComponent->FieldOfView;

	FROSTime ROSTime = FROSTime();
	TArray<FColor> ImageData;
	int32 Height = 0, Width = 0;
	this->CaptureImage("object_mask", ImageData, Width, Height);
	ROSDBG_TC_END(LogUnrealCV, *ROSTopic, " Label Capturing");

	if (ImageData.Num() != 0)
	{
		// publish camera_info
		std_msgs::Header Header(0, ROSTime, ROSFrameId);

		TArray<uint8> ByteData;
		PackFastMsgHeader(ByteData, ROSTopic + TEXT("/label/image"), TEXT("Image"), TEXT("Label"), ROSTime, this->ROSName, this->ROSFrameId, Height, Width);
		ByteData.Append((uint8*)ImageData.GetData(), sizeof(FColor)*ImageData.Num());

		ROSFastHandler->PublishFastMsg(ROSTopic + TEXT("/label/image"), ByteData);

	}
}

void UGTCameraCaptureComponent::ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// publish to ROS MASTER

	// publish tf
	std_msgs::Header Header(0, FROSTime(), "img_link");
	FTransform Transform = this->CameraComponent->GetRelativeTransform();
	//FVector loc = current.GetLocation();
	//FQuat quat = current.GetRotation();

	//geometry_msgs::Transform Transform(geometry_msgs::Vector3(loc.X, loc.Y, loc.Z), geometry_msgs::Quaternion(quat.X, quat.Y, quat.Z, quat.W));
	//geometry_msgs::Transform transform = FROSHelper::ConvertTransformUE4ToROS(Transform);
	//TSharedPtr<geometry_msgs::TransformStamped> Tf = MakeShareable(new geometry_msgs::TransformStamped(Header, ROSFrameId, transform));
	//ROSHandler->PublishMsg(ROSTopic + TEXT("/loc"), Tf);

	for (auto Elem : CaptureComponents)
	{
		USceneCaptureComponent2D* CaptureComponent = Elem.Value;

		if (Elem.Key.Compare(TEXT("lit")) == 0)
		{
			PublishImage();
		}
		else if (Elem.Key.Compare(TEXT("depth")) == 0)
		{
			PublishDepth();
		}
		else if (Elem.Key.Compare(TEXT("object_mask")) == 0)
		{
			PublishLabel();
		}
	}

	ROSHandler->Process();
}

void UGTCameraCaptureComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
// void UGTCaptureComponent::Tick(float DeltaTime) // This tick function should be called by the scene instead of been
{
	// Render pixels out in the next tick. To allow time to render images out.

	// Update rotation of each frame
	// from ab237f46dc0eee40263acbacbe938312eb0dffbb:CameraComponent.cpp:232
	check(this->Pawn); // this GTCapturer should be released, if the Pawn is deleted.
	check(this->CameraActor);
	check(this->CameraComponent); // this GTCapturer should be released, if the Pawn is deleted.
	const APawn* OwningPawn = this->Pawn;
	const AActor* OwningCamera = this->CameraActor;
	const AController* OwningController = OwningCamera ? OwningPawn->GetController() : nullptr;
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		if (this->MoveAnimCountFromRemote > 0)
		{
			FVector DeltaXYZ = this->Velocity.GetLocation()*DeltaTime;
			FRotator DeltaRot = this->Velocity.Rotator()*DeltaTime;

			this->CameraComponent->AddRelativeLocation(DeltaXYZ);
			this->CameraComponent->AddRelativeRotation(DeltaRot);

			this->MoveAnimCountFromRemote--;
		}
		else if (this->IsMoveToTarget) {
			FTransform current = this->CameraComponent->GetRelativeTransform();
			FTransform target  = this->TargetPose;
			FVector linear_offset, new_loc;
			FVector direction, delta_linear;
			FRotator new_rot;
			FRotator angular_diff, delta_angular;
			FVector  angular_diff_vec, temp;
			float norm, delta;

			linear_offset = target.GetLocation() - current.GetLocation();
			linear_offset.ToDirectionAndLength(direction, norm);
			delta = FMath::Min<float>(norm, DeltaTime*PARAM_MAX_CAM_LINEAR_SPEED);
			delta_linear = delta*direction;
			new_loc = current.GetLocation() + delta_linear;
			this->CameraComponent->AddRelativeLocation(delta_linear);

			angular_diff = target.Rotator() - current.Rotator();
			angular_diff_vec = FVector(angular_diff.Pitch, angular_diff.Yaw, angular_diff.Roll);
			angular_diff_vec.ToDirectionAndLength(direction, norm);
			delta = FMath::Min<float>(norm, DeltaTime*PARAM_MAX_CAM_ANGULAR_SPEED);
			temp = delta*direction;
			delta_angular = FRotator(temp.X, temp.Y, temp.Z);
			new_rot = current.Rotator() + delta_angular;
			this->CameraComponent->AddRelativeRotation(delta_angular);
			UE_LOG(LogUnrealCV, Log, TEXT("MoveToTargetPose(%f): (%f %f %f), (%f %f %f)"), DeltaTime, new_loc.X, new_loc.Y, new_loc.Z,
																						new_rot.Pitch, new_rot.Yaw, new_rot.Roll);
			if (FVector::PointsAreSame(new_loc, target.GetLocation()) && target.Rotator().Equals(new_rot)) {
				/* arrived at the target destinatoin */
				this->IsMoveToTarget = false;
				this->CameraComponent->SetRelativeTransform(target);
			}
		}

		if (this->IsZoomToTarget) {
			float current_fov = this->CameraComponent->FieldOfView;
			float fov_diff = this->TargetFOVAngle - current_fov;
			float direction = (fov_diff > 0) ? 1.0 : -1.0;
			float delta = FMath::Min<float>(FMath::Abs(fov_diff), DeltaTime*PARAM_MAX_CAM_ZOOM_SPEED);
			float delta_fov = delta * direction;
			float new_fov = current_fov + delta_fov;
			this->CameraComponent->SetFieldOfView(new_fov);
			UE_LOG(LogUnrealCV, Log, TEXT("MoveToTargetPose(%f) delta(%f): (%f)->(%f)"), DeltaTime, delta, current_fov, new_fov);

			if (FMath::IsNearlyZero(delta)) {
				this->IsZoomToTarget = false;
				this->CameraComponent->SetFieldOfView(this->TargetFOVAngle);
			}
		}

		const FRotator CameraViewRotation = this->CameraComponent->GetComponentRotation();
		const FVector CameraViewLocation = this->CameraComponent->GetComponentLocation();
		const float CameraFieldOfView = this->CameraComponent->FieldOfView;
		//UE_LOG(LogUnrealCV, Log, TEXT("%s update!"), *this->CameraComponent->GetName());
		for (auto Elem : CaptureComponents)
		{
			USceneCaptureComponent2D* CaptureComponent = Elem.Value;
			if (!CameraViewRotation.Equals(CaptureComponent->GetComponentRotation()))
			{
				CaptureComponent->SetWorldRotation(CameraViewRotation);
			}
			if (!CameraViewLocation.Equals(CaptureComponent->GetComponentLocation()))
			{
				CaptureComponent->SetWorldLocation(CameraViewLocation);
			}
			if (CameraFieldOfView != CaptureComponent->FOVAngle)
			{
				CaptureComponent->FOVAngle = CameraFieldOfView;
			}

		}

		ProcessUROSBridge(DeltaTime, TickType, ThisTickFunction);
	}
}
