#pragma once
#include "Object.h"
#include "UE4CVServer.h"
#include "ROSBRidgeHandler.h"
#include "ROSBridgePublisher.h"
#include "GTCaptureComponent.generated.h"

struct FGTCaptureTask
{
	FString Mode;
	FString Filename;
	uint64 CurrentFrame;
	FAsyncRecord* AsyncRecord;
	FGTCaptureTask() {}
	FGTCaptureTask(FString InMode, FString InFilename, uint64 InCurrentFrame, FAsyncRecord* InAsyncRecord) :
		Mode(InMode), Filename(InFilename), CurrentFrame(InCurrentFrame), AsyncRecord(InAsyncRecord) {}
};

/**
 * Use USceneCaptureComponent2D to export information from the scene.
 * This class needs to be tickable to update the rotation of the USceneCaptureComponent2D
 */

//FIXME
#define PARAM_MOVE_ANIM_COUNT       20   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_LINEAR_SPEED  20   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_ANGULAR_SPEED 20   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_ZOOM_SPEED    20	 //	tuning parameter considering unrealcv, robot, ROS

#define PARAM_MIN_CAM_ZOOM_FOV		10
#define PARAM_MAX_CAM_ZOOM_FOV		120

UCLASS()
class UNREALCV_API UGTCaptureComponent : public USceneComponent // , public FTickableGameObject
{
	GENERATED_BODY()
protected:
	UGTCaptureComponent();
	APawn* Pawn;
	FTransform Velocity;
	int32 MoveAnimCountFromRemote = 0;

	FTransform TargetPose;
	float TargetFOVAngle;
	bool IsMoveToTarget = false;
	bool IsZoomToTarget = false;

public:
	static UGTCaptureComponent* Create(FName Name, APawn* Pawn, TArray<FString> Modes);

	static UMaterial* GetMaterial(FString ModeName);

	// virtual void Tick(float DeltaTime) override; // TODO
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // TODO

	/** Save image to a file */
	FAsyncRecord* Capture(FString Mode, FString Filename);

	/** Read binary data in png format */
	// TArray<uint8> CapturePng(FString Mode);

	/** Read binary data in uncompressed numpy array */
	// TArray<uint8> CaptureNpy(FString Mode);

	void CaptureImage(const FString& Mode, TArray<FColor>& OutImageData, int32& OutWidth, int32& OutHeight);
	void CaptureFloat16Image(const FString& Mode, TArray<FFloat16Color>& OutImageData, int32& OutWidth, int32& OutHeight);
	void GetFieldOfView(const FString& Mode, float& FOV);
	void GetSize(const FString& Mode, int32& Width, int32& Height);
	void SetVelocity(const FTransform& Velocity);
	void SetTargetPose(const FTransform& TargetPose);
	void SetTargetFieldOfView(const float TargetFOV);
protected:
	const bool bIsTicking = true;

	TQueue<FGTCaptureTask, EQueueMode::Spsc> PendingTasks;
	TMap<FString, USceneCaptureComponent2D*> CaptureComponents;
};

/**
* Use USceneCaptureComponent2D to export information from the scene.
* This class needs to be tickable to update the rotation of the USceneCaptureComponent2D
*/
UCLASS()
class UNREALCV_API UGTCameraCaptureComponent : public UGTCaptureComponent // , public FTickableGameObject
{
	GENERATED_BODY()
protected:
	AActor* CameraActor;
	UCameraComponent* CameraComponent;

	// variables for UROSBridge
	FString ROSTopic;
	FString ROSFrameId;
	TSharedPtr<FROSBridgeHandler> ROSHandler;
	TSharedPtr<FROSBridgePublisher> TfPublisher;

	TSharedPtr<FROSBridgePublisher> ImageCameraInfoPublisher;
	TSharedPtr<FROSBridgePublisher> ImagePublisher;

	TSharedPtr<FROSBridgePublisher> DepthCameraInfoPublisher;
	TSharedPtr<FROSBridgePublisher> DepthPublisher;

public:
	static UGTCameraCaptureComponent* Create(FName Name, APawn* InPawn, AActor* InCameraActor, UCameraComponent* InCameraComp, TArray<FString> Modes);

	UCameraComponent* GetCameraComponent() { return CameraComponent; }
	// virtual void Tick(float DeltaTime) override; // TODO
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // TODO

	// functions for UROSBridge
	void SetUROSBridge(FString InROSTopic, FString InROSFrameId);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

protected:
	void ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	void PublishImage(void);
	void PublishDepth(void);
};
