#pragma once
#include "Object.h"
#include "UE4CVServer.h"
#include "Camera/CameraComponent.h"
#include "GTCaptureComponent.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"
#include "ROSTime.h"
#include "GTCameraCaptureComponent.generated.h"

/**
 * Use USceneCaptureComponent2D to export information from the scene.
 * This class needs to be tickable to update the rotation of the USceneCaptureComponent2D
 */

//FIXME
#define PARAM_MOVE_ANIM_COUNT       10   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_LINEAR_SPEED  30   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_ANGULAR_SPEED 30   // tuning parameter considering unrealcv, robot, ROS
#define PARAM_MAX_CAM_ZOOM_SPEED    20	 //	tuning parameter considering unrealcv, robot, ROS

#define PARAM_MIN_CAM_ZOOM_FOV		10
#define PARAM_MAX_CAM_ZOOM_FOV		120

/**
* Use USceneCaptureComponent2D to export information from the scene.
* This class needs to be tickable to update the rotation of the USceneCaptureComponent2D
*/
UCLASS()
class UNREALCV_API UGTCameraCaptureComponent : public UGTCaptureComponent // , public FTickableGameObject
{
	GENERATED_BODY()


	class FROSFloat32SubScriber : public FROSBridgeSubscriber
	{
		UGTCameraCaptureComponent* Component;
	public:
		FROSFloat32SubScriber(const FString& InTopic, UGTCameraCaptureComponent* InComponent);
		~FROSFloat32SubScriber() override;
		TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override;
		void Callback(TSharedPtr<FROSBridgeMsg> InMsg) override;
	};

protected:
	AActor* CameraActor;
	UCameraComponent* CameraComponent;

	FTransform Velocity;
	int32 MoveAnimCountFromRemote = 0;

	FTransform TargetPose;
	float TargetFOVAngle;
	bool IsMoveToTarget = false;
	bool IsZoomToTarget = false;

	// variables for UROSBridge
	FString ROSNamespace;
	FString ROSName;
	FString ROSTopic;
	FString ROSFrameId;
	TSharedPtr<FROSBridgeHandler> ROSHandler;

	TMap<FString, TArray<uint8>> ROSFastMsgHeaderCache;
	TSharedPtr<FROSBridgeHandler> ROSFastHandler;

	FTransform InitialTransform;

public:
	static UGTCameraCaptureComponent* Create(FName Name, APawn* InPawn, AActor* InCameraActor, UCameraComponent* InCameraComp, TArray<FString> Modes);

	void GetFieldOfView(const FString& Mode, float& FOV);
	void GetSize(const FString& Mode, int32& Width, int32& Height);
	void SetVelocity(const FTransform& Velocity);
	void SetTargetPose(const FTransform& TargetPose);
	void SetTargetFieldOfView(const float TargetFOV);

	UCameraComponent* GetCameraComponent() { return CameraComponent; }
	// virtual void Tick(float DeltaTime) override; // TODO
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // TODO

	// functions for UROSBridge
	void SetUROSBridge(FString InROSNamespace, FString InROSName, FString InROSFrameId);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	FString GetROSName(void) { return ROSName; }
	FTransform GetInitialTransform(void) { return InitialTransform; }

	//TODO
	//void SetROSHandler(TSharedPtr<FROSBridgeHandler> ROSHandler){ this->ROSHandler = ROSHandler; }

protected:
	FROSTime GetROSSimTime()
	{
		float GameTime = GetWorld()->GetTimeSeconds();
		uint32 Secs = (uint32)GameTime;
		uint32 NSecs = (uint32)((GameTime - Secs)*1000000000);
		return FROSTime(Secs, NSecs);
	}

	void PackFastMsgHeader(TArray<uint8> &ByteData, FString Type, FString Topic, FString MsgType, FROSTime Stamp, FString Name, FString FrameId, uint32 Height, uint32 Width);
	void ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
	void PublishImage(void);
	void PublishDepth(void);
	void PublishLabel(void);
};
