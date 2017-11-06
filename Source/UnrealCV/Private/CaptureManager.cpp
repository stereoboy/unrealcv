#include "UnrealCVPrivate.h"
#include "CaptureManager.h"

/**
  * Where to put cameras
  * For each camera in the scene, attach SceneCaptureComponent2D to it
  * So that ground truth can be generated, invoked when a new actor is created
  */
void FCaptureManager::AttachGTCaptureComponentToCamera(APawn* Pawn)
{
	// TODO: Only support one camera at the beginning
	// TODO: Make this automatic from material loader.
	TArray<FString> SupportedModes;
	SupportedModes.Add(TEXT("lit")); // This is lit
	SupportedModes.Add(TEXT("depth"));
	// SupportedModes.Add(TEXT("debug"));
	SupportedModes.Add(TEXT("object_mask"));
	SupportedModes.Add(TEXT("normal"));
	// SupportedModes.Add(TEXT("wireframe"));
	SupportedModes.Add(TEXT("default"));
	// TODO: Get the list from GTCaptureComponent

	CaptureComponentList.Empty();

	UGTCaptureComponent* Capturer = UGTCaptureComponent::Create(Pawn, SupportedModes);
	CaptureComponentList.Add(Capturer);

	UGTCaptureComponent* RightEye = UGTCaptureComponent::Create(Pawn, SupportedModes);
	RightEye->SetRelativeLocation(FVector(0, 40, 0));
	// RightEye->AddLocalOffset(FVector(0, 40, 0)); // TODO: make this configurable
	CaptureComponentList.Add(RightEye);

	UE_LOG(LogUnrealCV, Log, TEXT("Camera Component List"));
	TArray<UActorComponent*> cameras = (Pawn->GetComponentsByClass(UCameraComponent::StaticClass()));
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));
	for (int32 idx = 0; idx < cameras.Num(); ++idx)
	{
		UCameraComponent* camera = Cast<UCameraComponent>(cameras[idx]);
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetFullName());
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetName());
		UE_LOG(LogUnrealCV, Log, TEXT("cameras[%d]: %s"), idx, *camera->GetFullGroupName(false));

		UGTCaptureComponent* camCom = UGTCameraCaptureComponent::Create(Pawn, camera->GetOwner(), camera, SupportedModes);
		// RightEye->AddLocalOffset(FVector(0, 40, 0)); // TODO: make this configurable
		CaptureComponentList.Add(camCom);
	}
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));
}

UGTCaptureComponent* FCaptureManager::GetCamera(int32 CameraId)
{
	if (CameraId < CaptureComponentList.Num() && CameraId >= 0)
	{
		return CaptureComponentList[CameraId];
	}
	else
	{
		return nullptr;
	}
}
