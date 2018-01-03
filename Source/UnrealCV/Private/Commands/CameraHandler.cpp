// #include "RealisticRendering.h"
#include "UnrealCVPrivate.h"
#include "CameraHandler.h"
#include "ViewMode.h"
#include "ImageUtils.h"
#include "ImageWrapper.h"
#include "GTCaptureComponent.h"
#include "PlayerViewMode.h"
#include "UE4CVServer.h"
#include "CaptureManager.h"
#include "CineCameraActor.h"
#include "ObjectPainter.h"
#include "ScreenCapture.h"
#include "Serialization.h"
#include "Kismet/KismetMathLibrary.h"

FExecStatus GetPngBinary(const TArray<FString>& Args, const FString& Mode);
FExecStatus GetDepthNpy(const TArray<FString>& Args);
FExecStatus GetNormalNpy(const TArray<FString>& Args);
FExecStatus GetImgBinary(const TArray<FString>& Args);

FString GetDiskFilename(FString Filename)
{
	const FString Dir = FPlatformProcess::BaseDir(); // TODO: Change this to screen capture folder
	// const FString Dir = FPaths::ScreenShotDir();
	FString FullFilename = FPaths::Combine(*Dir, *Filename);

	FString DiskFilename = IFileManager::Get().GetFilenameOnDisk(*FullFilename); // This is important
	return DiskFilename;
}

FString GenerateSeqFilename()
{
	static uint32 NumCaptured = 0;
	NumCaptured++;
	FString Filename = FString::Printf(TEXT("%08d.png"), NumCaptured);
	return Filename;
}

void FCameraCommandHandler::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	FString Help;

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/[str] [str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetLitViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetLitViewMode);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit [str]", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetObjectInstanceMask);
	CommandDispatcher->BindCommand("vget /camera/[uint]/object_mask", Cmd, "Get snapshot from camera, the third parameter is optional");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetObjectInstanceMask);
	Help = "Get object mask from camera";
	CommandDispatcher->BindCommand("vget /camera/[uint]/object_mask [str]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetScreenshot);
	Help = "Get snapshot from camera";
	CommandDispatcher->BindCommand("vget /camera/[uint]/screenshot", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraLocation);
	Help = "Get camera location [x, y, z]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/location", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraRotation);
	Help = "Get camera rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/rotation", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraLocation);
	Help = "Teleport camera to location [x, y, z]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/location [float] [float] [float]", Cmd, Help);

	/** This is different from SetLocation (which is teleport) */
	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::MoveTo);
	Help = "Move camera to location [x, y, z], will be blocked by objects";
	CommandDispatcher->BindCommand("vset /camera/[uint]/moveto [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraRotation);
	Help = "Set rotation [pitch, yaw, roll] of camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/rotation [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraProjMatrix);
	Help = "Get projection matrix from camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/proj_matrix", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(&FPlayerViewMode::Get(), &FPlayerViewMode::SetMode);
	Help = "Set ViewMode to (lit, normal, depth, object_mask)";
	CommandDispatcher->BindCommand("vset /viewmode [str]", Cmd, Help); // Better to check the correctness at compile time

	Cmd = FDispatcherDelegate::CreateRaw(&FPlayerViewMode::Get(), &FPlayerViewMode::GetMode);
	Help = "Get current ViewMode";
	CommandDispatcher->BindCommand("vget /viewmode", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetActorLocation);
	Help = "Get actor location [x, y, z]";
	CommandDispatcher->BindCommand("vget /actor/location", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetActorRotation);
	Help = "Get actor rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand("vget /actor/rotation", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetActorTransform);
	Help = "Get actor transform [x, y, z, pitch, yaw, roll]";
	CommandDispatcher->BindCommand("vget /actor/transform", Cmd, Help);

	Help = "Return raw binary image data, instead of the image filename";
	Cmd = FDispatcherDelegate::CreateLambda([](const TArray<FString>& Args) { return GetPngBinary(Args, TEXT("lit")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit png", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateLambda([](const TArray<FString>& Args) { return GetPngBinary(Args, TEXT("depth")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth png", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateLambda([](const TArray<FString>& Args) { return GetPngBinary(Args, TEXT("normal")); });
	CommandDispatcher->BindCommand("vget /camera/[uint]/normal png", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateStatic(GetDepthNpy);
	CommandDispatcher->BindCommand("vget /camera/[uint]/depth npy", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateStatic(GetNormalNpy);
	CommandDispatcher->BindCommand("vget /camera/[uint]/normal npy", Cmd, Help);

	// for TEST
	Cmd = FDispatcherDelegate::CreateStatic(GetImgBinary);
	CommandDispatcher->BindCommand("vget /camera/[uint]/lit bin", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::GetCameraTransform);
	Help = "Get camera transform [x, y, z, pitch, yaw, roll] of camera [id]";
	CommandDispatcher->BindCommand("vget /camera/[uint]/transform", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::SetCameraTransform);
	Help = "Move camera to [x, y, z, pitch, yaw, roll] of camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/transform [float] [float] [float] [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::MoveCamera);
	Help = "Move camera with delta [x, y, z, pitch, yaw, roll] of camera [id]";
	CommandDispatcher->BindCommand("vset /camera/[uint]/move [float] [float] [float] [float] [float] [float]", Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FCameraCommandHandler::MoveActor);
	Help = "Move player actor with delta [x, y, z, pitch, yaw, roll], will be blocked by objects";
	CommandDispatcher->BindCommand("vset /actor/move [float] [float] [float] [float] [float] [float]", Cmd, Help);

	// TODO: object_mask will be handled differently
}

FExecStatus FCameraCommandHandler::GetCameraProjMatrix(const TArray<FString>& Args)
{
	// FMatrix& ProjMatrix = FSceneView::ViewProjectionMatrix;
	// this->Character->GetWorld()->GetGameViewport()->Viewport->
	// this->Character
	//Cast<UCameraComponent>
	
	/*
	APlayerController* PlayerController = FUE4CVServer::Get().GetGameWorld()->GetFirstPlayerController();
	
	UCameraComponent* camera = Cast<UCameraComponent>(PlayerController->GetViewTarget()->GetComponentByClass(UCameraComponent::StaticClass()));
	
	UE_LOG(LogUnrealCV, Error, TEXT("camera->GetName(): %s"), *camera->GetName());
	UE_LOG(LogUnrealCV, Error, TEXT("camera->FieldOfView: %f"), camera->FieldOfView);
	UE_LOG(LogUnrealCV, Error, TEXT("camera local location: (%f %f %f)"), camera->RelativeLocation.X, camera->RelativeLocation.Y, camera->RelativeLocation.Z);
	UE_LOG(LogUnrealCV, Error, TEXT("camera local ratation: (%f %f %f)"), camera->RelativeRotation.Roll, camera->RelativeRotation.Pitch, camera->RelativeRotation.Yaw);

	FVector world_loc = camera->GetComponentLocation();
	FRotator world_rot = camera->GetComponentRotation();
	UE_LOG(LogUnrealCV, Error, TEXT("camera local location: (%f %f %f)"), world_loc.X, world_loc.Y, world_loc.Z);
	UE_LOG(LogUnrealCV, Error, TEXT("camera local ratation: (%f %f %f)"), world_rot.Roll, world_rot.Pitch, world_rot.Yaw);
	*/
	/*
	TArray<UActorComponent*> cameras = (FUE4CVServer::Get().GetPawn()->GetComponentsByClass(UCameraComponent::StaticClass()));
	for (int32 idx = 0; idx < cameras.Num(); ++idx)
	{
		UE_LOG(LogUnrealCV, Error, TEXT("cameras[%d]: %s"), idx, *cameras[idx]->GetFullName());
		UE_LOG(LogUnrealCV, Error, TEXT("cameras[%d]: %s"), idx, *cameras[idx]->GetName());
		UE_LOG(LogUnrealCV, Error, TEXT("cameras[%d]: %s"), idx, *cameras[idx]->GetFullGroupName(false));
	}
	*/
	float FOV = 0.0;
	int32 Width = 0;
	int32 Height = 0;
	int32 CameraId = FCString::Atoi(*Args[0]);
	UGTCaptureComponent* Camera;
	Camera = FCaptureManager::Get().GetCamera(CameraId);
	if (Camera == nullptr)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
	}
	
	Camera->GetFieldOfView("lit", FOV);
	Camera->GetSize("lit", Width, Height);
	if (FOV == 0.0 || Width == 0 || Height == 0)
	{
		return FExecStatus::Error("Failed to read the field of view value.");
	}

	FString Message = FString::Printf(TEXT("%.3f %d %d"), FOV, Width, Height);

	return FExecStatus::OK(Message);
}

FExecStatus FCameraCommandHandler::MoveTo(const TArray<FString>& Args)
{
	/** The API for Character, Pawn and Actor are different */
	if (Args.Num() == 4) // ID, X, Y, Z
	{
		int32 CameraId = FCString::Atoi(*Args[0]); // TODO: Add support for multiple cameras
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);

		bool Sweep = true;
		// if sweep is true, the object can not move through another object
		// Check invalid location and move back a bit.
		bool Success = FUE4CVServer::Get().GetPawn()->SetActorLocation(Location, Sweep, NULL, ETeleportType::TeleportPhysics);

		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraLocation(const TArray<FString>& Args)
{
	/** The API for Character, Pawn and Actor are different */
	if (Args.Num() == 4) // ID, X, Y, Z
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);

		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		camera->SetRelativeLocation(Location);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 4) // ID, Pitch, Roll, Yaw
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float Pitch = FCString::Atof(*Args[1]), Yaw = FCString::Atof(*Args[2]), Roll = FCString::Atof(*Args[3]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);

		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		camera->SetRelativeRotation(Rotator);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::SetCameraTransform(const TArray<FString>& Args)
{
	if (Args.Num() == 7) // ID, Pitch, Roll, Yaw
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Location = FVector(X, Y, Z);
		float Pitch = FCString::Atof(*Args[4]), Yaw = FCString::Atof(*Args[5]), Roll = FCString::Atof(*Args[6]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);

		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

#if 0
		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		camera->SetRelativeTransform(FTransform(Rotator, Location));
#else
		GTCapturer->SetTargetPose(FTransform(Rotator, Location));
#endif
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::MoveCamera(const TArray<FString>& Args)
{
	if (Args.Num() == 7) // ID, Pitch, Roll, Yaw
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector Loc = FVector(X, Y, Z);
		float Pitch = FCString::Atof(*Args[4]), Yaw = FCString::Atof(*Args[5]), Roll = FCString::Atof(*Args[6]);
		FRotator Rot = FRotator(Pitch, Yaw, Roll);

		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		FTransform Velocity = FTransform(Rot, Loc);
		GTCapturer->SetVelocity(Velocity);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		FRotator CameraRotation = camera->GetRelativeTransform().Rotator();
		FVector loc = camera->GetComponentTransform().GetLocation();
		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraLocation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		FVector CameraLocation = camera->GetRelativeTransform().GetLocation();
		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z);
		return FExecStatus::OK(Message);
	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetCameraTransform(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		UGTCameraCaptureComponent* GTCapturer = Cast<UGTCameraCaptureComponent>(FCaptureManager::Get().GetCamera(CameraId));
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		UCameraComponent* camera = GTCapturer->GetCameraComponent();
		FVector CameraLocation = camera->GetRelativeTransform().GetLocation();
		FRotator CameraRotation = camera->GetRelativeTransform().Rotator();
		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f %.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z, CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
		return FExecStatus::OK(Message);

	}
	return FExecStatus::Error("Number of arguments incorrect");
}

FExecStatus FCameraCommandHandler::GetObjectInstanceMask(const TArray<FString>& Args)
{
	if (Args.Num() <= 2) // The first is camera id, the second is ViewMode
	{
		// Use command dispatcher is more universal
		FExecStatus ExecStatus = CommandDispatcher->Exec(TEXT("vset /viewmode object_mask"));
		if (ExecStatus != FExecStatusType::OK)
		{
			return ExecStatus;
		}

		ExecStatus = GetScreenshot(Args);
		return ExecStatus;
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetLitViewMode(const TArray<FString>& Args)
{
	if (Args.Num() <= 3)
	{
		// For this viewmode, The post-effect material needs to be explictly cleared
		FPlayerViewMode::Get().Lit();

		TArray<FString> ExtraArgs(Args);
		ExtraArgs.Insert(TEXT("lit"), 1);
		return GetCameraViewMode(ExtraArgs);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FCameraCommandHandler::GetCameraViewMode(const TArray<FString>& Args)
{
	if (Args.Num() <= 3) // The first is camera id, the second is ViewMode
	{
		int32 CameraId = FCString::Atoi(*Args[0]);
		FString ViewMode = Args[1];

		FString Filename;
		if (Args.Num() == 3)
		{
			Filename = Args[2];
		}
		else
		{
			Filename = GenerateSeqFilename();
		}

		UGTCaptureComponent* GTCapturer = FCaptureManager::Get().GetCamera(CameraId);
		if (GTCapturer == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
		}

		FAsyncRecord* AsyncRecord = GTCapturer->Capture(*ViewMode, *Filename); // Due to sandbox implementation of UE4, it is not possible to specify an absolute path directly.
		if (AsyncRecord == nullptr)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Unrecognized capture mode %s"), *ViewMode));
		}

		// TODO: Check IsPending is problematic.
		FPromiseDelegate PromiseDelegate = FPromiseDelegate::CreateLambda([Filename, AsyncRecord]()
		{
			if (AsyncRecord->bIsCompleted)
			{
				AsyncRecord->Destory();
				return FExecStatus::OK(GetDiskFilename(Filename));
			}
			else
			{
				return FExecStatus::Pending();
			}
		});
		FString Message = FString::Printf(TEXT("File will be saved to %s"), *Filename);
		return FExecStatus::AsyncQuery(FPromise(PromiseDelegate));
		// The filename here is just for message, not the fullname on the disk, because we can not know that due to sandbox issue.
	}
	return FExecStatus::InvalidArgument;
}

/** vget /camera/[id]/screenshot */
FExecStatus FCameraCommandHandler::GetScreenshot(const TArray<FString>& Args)
{
	int32 CameraId = FCString::Atoi(*Args[0]);

	FString Filename;
	if (Args.Num() > 2)
	{
		return FExecStatus::InvalidArgument;
	}
	if (Args.Num() == 1)
	{
		Filename = GenerateSeqFilename();
	}
	if (Args.Num() == 2)
	{
		Filename = Args[1];
	}

	if (Filename.ToLower() == TEXT("png"))
	{
		return ScreenCaptureAsyncByQuery(); // return the binary data
	}
	else
	{
		return ScreenCaptureAsyncByQuery(Filename);
	}
}

FExecStatus FCameraCommandHandler::GetActorRotation(const TArray<FString>& Args)
{
	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FRotator CameraRotation = Pawn->GetControlRotation();
	FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
	return FExecStatus::OK(Message);
}

FExecStatus FCameraCommandHandler::GetActorLocation(const TArray<FString>& Args)
{
	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FVector CameraLocation = Pawn->GetActorLocation();
	FString Message = FString::Printf(TEXT("%.3f %.3f %.3f"), CameraLocation.X, CameraLocation.Y, CameraLocation.Z);
	return FExecStatus::OK(Message);
}

FExecStatus FCameraCommandHandler::GetActorTransform(const TArray<FString>& Args)
{
	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FVector Loc = Pawn->GetActorTransform().GetLocation();
	FRotator Rot = Pawn->GetActorTransform().Rotator();
	FString Message = FString::Printf(TEXT("%.3f %.3f %.3f %.3f %.3f %.3f"), Loc.X, Loc.Y, Loc.Z, Rot.Pitch, Rot.Yaw, Rot.Roll);
	return FExecStatus::OK(Message);
}

FExecStatus ParseCamera(const TArray<FString>& Args, UGTCaptureComponent* &OutCamera)
{
	int32 CameraId = FCString::Atoi(*Args[0]);
	OutCamera = FCaptureManager::Get().GetCamera(CameraId);
	if (OutCamera == nullptr)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Invalid camera id %d"), CameraId));
	}
	return FExecStatus::OK();
}

FExecStatus GetPngBinary(const TArray<FString>& Args, const FString& Mode)
{
	UGTCaptureComponent* Camera;
	FExecStatus Status = ParseCamera(Args, Camera);
	if (Status != FExecStatusType::OK) return Status;

	TArray<FColor> ImageData;
	int32 Height = 0, Width = 0;
	Camera->CaptureImage(Mode, ImageData, Width, Height);
	if (ImageData.Num() == 0)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Failed to read %s data."), *Mode));
	}

	TArray<uint8> PngBinaryData = SerializationUtils::Image2Png(ImageData, Width, Height);
	return FExecStatus::Binary(PngBinaryData);
}

FExecStatus GetImgBinary(const TArray<FString>& Args)
{
	UGTCaptureComponent* Camera;
	FExecStatus Status = ParseCamera(Args, Camera);
	if (Status != FExecStatusType::OK) return Status;

	TArray<FColor> ImageData;
	int32 Height = 0, Width = 0;
	Camera->CaptureImage("lit", ImageData, Width, Height);
	if (ImageData.Num() == 0)
	{
		return FExecStatus::Error(FString::Printf(TEXT("Failed to read lit data.")));
	}

	// https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/TArrays/
	TArray<uint8> BinaryData;
	BinaryData.AddUninitialized(Width*Height*sizeof(FColor));
	FMemory::Memcpy(BinaryData.GetData(), ImageData.GetData(), Width*Height * sizeof(FColor));

	UE_LOG(LogUnrealCV, Log, TEXT("GetImgBinary %dx%d"), Width, Height);
	return FExecStatus::Binary(BinaryData);
}

FExecStatus GetDepthNpy(const TArray<FString>& Args)
{
	UGTCaptureComponent* Camera;
	FExecStatus Status = ParseCamera(Args, Camera);
	if (Status != FExecStatusType::OK) return Status;

	TArray<FFloat16Color> FloatImageData;
	int32 Height = 0, Width = 0;
	Camera->CaptureFloat16Image("depth", FloatImageData, Width, Height);
	if (FloatImageData.Num() == 0)
	{
		return FExecStatus::Error("Failed to read depth data.");
	}

	TArray<uint8> NpyBinaryData = SerializationUtils::Array2Npy(FloatImageData, Width, Height, 1);

	return FExecStatus::Binary(NpyBinaryData);
}

FExecStatus GetNormalNpy(const TArray<FString>& Args)
{
	UGTCaptureComponent* Camera;
	FExecStatus Status = ParseCamera(Args, Camera);
	if (Status != FExecStatusType::OK) return Status;

	TArray<FFloat16Color> FloatImageData;
	int32 Height = 0, Width = 0;
	Camera->CaptureFloat16Image("normal", FloatImageData, Width, Height);
	if (FloatImageData.Num() == 0)
	{
		return FExecStatus::Error("Failed to read normal data.");
	}

	TArray<uint8> NpyBinaryData = SerializationUtils::Array2Npy(FloatImageData, Width, Height, 3);

	return FExecStatus::Binary(NpyBinaryData);

}

FExecStatus FCameraCommandHandler::MoveActor(const TArray<FString>& Args)
{
	if (Args.Num() == 6) // X, Y, Z, Pitch, Yaw, Roll
	{
		float X = FCString::Atof(*Args[0]), Y = FCString::Atof(*Args[1]), Z = FCString::Atof(*Args[2]);
		float Pitch = FCString::Atof(*Args[3]), Yaw = FCString::Atof(*Args[4]), Roll = FCString::Atof(*Args[5]);
		FVector Offset = FVector(X, Y, Z);
		FRotator Rot = FRotator(Pitch, Yaw, Roll);

		bool Sweep = true;
		// if sweep is true, the object can not move through another object
		// Check invalid location and move back a bit.

		APawn* Pawn = FUE4CVServer::Get().GetPawn();
		FRotator CtrlRot = Pawn->GetControlRotation();
		FVector foward = UKismetMathLibrary::GetForwardVector(CtrlRot);
		FVector right = UKismetMathLibrary::GetRightVector(CtrlRot);
		FVector up = UKismetMathLibrary::GetUpVector(CtrlRot);

		FVector offset = foward*X + right*Y + up*Z;
		FVector direction;
		float norm;
		offset.ToDirectionAndLength(direction, norm);

		//Pawn->AddMovementInput(offset);
		//float modified_norm = FMath::Min<float>(norm, 5.0);
		//FVector NewLocation = Pawn->GetActorLocation() + direction*norm;
		//Pawn->SetActorLocation(NewLocation);

		URemoteMovementComponent* Remote = FCaptureManager::Get().GetActor(0);
		FTransform Velocity = FTransform(Rot, direction*norm);
		Remote->SetVelocityCmd(Velocity);

#if 0
		static const FName MoveAnimCountFromRemote(TEXT("MoveAnimCountFromRemote"));
		static const FName XVelocity(TEXT("XVelocity"));
		static const FName YawVelocity(TEXT("YawVelocity"));

		UClass* MyClass = Pawn->GetClass();

		for (UProperty* Property = MyClass->PropertyLink; Property; Property = Property->PropertyLinkNext)
		{
			UIntProperty* IntProperty = Cast<UIntProperty>(Property);
			UFloatProperty* FloatProperty = Cast<UFloatProperty>(Property);
			UE_LOG(LogUnrealCV, Log, TEXT("Property: %s"), *Property->GetName())
			if (IntProperty && Property->GetFName() == MoveAnimCountFromRemote)
			{
				// Need more work for arrays
				int32 MyIntValue = IntProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<int32>(Pawn));
				UE_LOG(LogUnrealCV, Log, TEXT("Before-Value is = %i"), MyIntValue);
				IntProperty->SetPropertyValue(Property->ContainerPtrToValuePtr<int32>(Pawn), PARAM_MOVE_ANIM_COUNT);
				//MyIntValue = IntProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<int32>(Pawn));
				//UE_LOG(LogUnrealCV, Log, TEXT("After-Value is = %i"), MyIntValue);
			}
			else if (FloatProperty && Property->GetFName() == XVelocity)
			{
				// Need more work for arrays
				FloatProperty->SetPropertyValue(Property->ContainerPtrToValuePtr<float>(Pawn), X);
				float value = FloatProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<float>(Pawn));
				UE_LOG(LogUnrealCV, Log, TEXT("XVelocity = %f"), value);
			}
			else if (FloatProperty && Property->GetFName() == YawVelocity)
			{
				// Need more work for arrays
				FloatProperty->SetPropertyValue(Property->ContainerPtrToValuePtr<float>(Pawn), Yaw);
				float value = FloatProperty->GetPropertyValue(Property->ContainerPtrToValuePtr<float>(Pawn));
				UE_LOG(LogUnrealCV, Log, TEXT("YawVelocity = %f"), value);
			}
		}
#endif

		FString Message = FString::Printf(TEXT("%.3f %.3f %.3f %.3f"), direction.X, direction.Y, direction.Z, norm);
		return FExecStatus::OK(Message);
	}
	return FExecStatus::InvalidArgument;
}