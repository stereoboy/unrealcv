#pragma once
#include "CommandHandler.h"
#include "GTCaptureComponent.h"

class FCameraCommandHandler : public FCommandHandler
{
public:
	FCameraCommandHandler(FCommandDispatcher* InCommandDispatcher) : FCommandHandler(InCommandDispatcher)
	{}
	void RegisterCommands();

	/** vget /camera/location */
	FExecStatus GetCameraLocation(const TArray<FString>& Args);
	/** vset /camera/location */
	FExecStatus	SetCameraLocation(const TArray<FString>& Args);
	/** vset /camera/moveto */
	FExecStatus	MoveTo(const TArray<FString>& Args);
	/** vget /camera/[id]/rotation */
	FExecStatus GetCameraRotation(const TArray<FString>& Args);
	/** vset /camera/[id]/rotation */
	FExecStatus	SetCameraRotation(const TArray<FString>& Args);

	/** vget /camera/[id]/transform */
	FExecStatus GetCameraTransform(const TArray<FString>& Args);
	/** vset /camera/[id]/transform */
	FExecStatus	SetCameraTransform(const TArray<FString>& Args);

	/** vset /camera/[id]/move */
	FExecStatus	MoveCamera(const TArray<FString>& Args);

	/** vget /actor/rotation, follow the concept of actor in RL */
	FExecStatus GetActorRotation(const TArray<FString>& Args);
	/** vget /actor/location */
	FExecStatus GetActorLocation(const TArray<FString>& Args);
	/** vget /actor/transform */
	FExecStatus GetActorTransform(const TArray<FString>& Args);

	/** vget /camera/view */
	FExecStatus GetScreenshot(const TArray<FString>& Args);

	/** Get camera image with a given mode, Get ViewMode data using SceneCaptureComponent, support multi-camera */
	FExecStatus GetCameraViewMode(const TArray<FString>& Args);

	/** Explicitly set ViewMode, then GetCameraViewMode */
	FExecStatus GetLitViewMode(const TArray<FString>& Args);

	/** Get camera project matrix */
	FExecStatus GetCameraProjMatrix(const TArray<FString>& Args);
	/** Set camera project matrix */
	FExecStatus SetCameraProjMatrix(const TArray<FString>& Args);

	/** Get HDR buffer visualization */
	FExecStatus GetBuffer(const TArray<FString>& Args);

	/** Get HDR from the scene */
	// FExecStatus GetCameraHDR(const TArray<FString>& Args);
	// FExecStatus GetCameraDepth(const TArray<FString>& Args);
	// FExecStatus GetCameraLit(const TArray<FString>& Args);

	/** Get ViewMode data by switching to this viewmode then switch back, can not support multi-camera */
	FExecStatus GetObjectInstanceMask(const TArray<FString>& Args);

	/** vset /actor/move x y z pitch yaw roll */
	FExecStatus	MoveActor(const TArray<FString>& Args);
};
