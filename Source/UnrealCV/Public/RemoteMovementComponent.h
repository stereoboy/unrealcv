#pragma once
#include "Object.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"
#include "ROSBridgeSubscriber.h"
#include "geometry_msgs/PoseStamped.h"
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/Point.h"
#include "std_msgs/ColorRGBA.h"
#include "RemoteMovementComponent.generated.h"


/*
reference: https://docs.unrealengine.com/latest/INT/Programming/Tutorials/Components/4/index.html
*/

#define UE4ROS_LINEAR_MOVEMENT_SCALE_FACTOR 0.01
#define UE4ROS_ANGULAR_MOVEMENT_SCALE_FACTOR 1.0

UCLASS()
class UNREALCV_API URemoteMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

	class FROSPoseStampedSubScriber : public FROSBridgeSubscriber
	{
		URemoteMovementComponent* Component;
	public:
		FROSPoseStampedSubScriber(const FString& InTopic, URemoteMovementComponent* Component);
		~FROSPoseStampedSubScriber() override;
		TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override;
		void Callback(TSharedPtr<FROSBridgeMsg> InMsg) override;
	};

	class FROSTwistSubScriber : public FROSBridgeSubscriber
	{
		URemoteMovementComponent* Component;
	public:
		FROSTwistSubScriber(const FString& InTopic, URemoteMovementComponent* Component);
		~FROSTwistSubScriber() override;
		TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override;
		void Callback(TSharedPtr<FROSBridgeMsg> InMsg) override;
	};

	class FROSJointStateSubScriber : public FROSBridgeSubscriber
	{
		friend class URemoteMovementComponent;
		URemoteMovementComponent* Component;
	public:
		FROSJointStateSubScriber(const FString& InTopic, URemoteMovementComponent* Component);
		~FROSJointStateSubScriber() override;
		TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override;
		void Callback(TSharedPtr<FROSBridgeMsg> InMsg) override;
	};

protected:
	URemoteMovementComponent();

	FTransform VelocityCmd;
	int32 MoveAnimCountFromRemote = 0;

	FTransform TargetPose;
	bool IsMoveToTarget = false;
	bool bIsTicking ;

	FVector	PrevLinear;
	FRotator PrevAngular;

	int LabelColorTablePubCount;
	bool bSkeletalActorMapInitialized;
	TMap<FString, AActor*> SkeletalActorMap;

	TSharedPtr<FROSBridgeHandler> ROSHandler;

	TArray<UGTCameraCaptureComponent*> CaptureComponentList;
	TMap<FString, UStaticMeshComponent*> JointComponentMap;

	UStaticMeshComponent* FootprintComponent;

public:
	static URemoteMovementComponent* Create(FName Name, APawn* Pawn);

	void Init(void);
	void SetVelocityCmd(const FTransform& InVelocityCmd);
	/*
	* reference : https://docs.unrealengine.com/latest/INT/Programming/UnrealArchitecture/Actors/Ticking/#componentticking
	*/
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	void AddCaptureComponent(UGTCameraCaptureComponent* Component) 
	{
		CaptureComponentList.Add(Component);
	}

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

	void ROSPublishOdom(float DeltaTime);
	void ROSPublishJointState(float DeltaTime);
	void ROSPublishSkeletalState(float DeltaTime);
	void ROSBuildSkeletalState(USkeletalMeshComponent* SkeletalMeshComponent, TArray<geometry_msgs::Point> &Points, TArray<std_msgs::ColorRGBA> &Colors);
	void ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
};
