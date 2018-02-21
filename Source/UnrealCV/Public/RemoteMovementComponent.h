#pragma once
#include "Object.h"
#include "ROSBRidgeHandler.h"
#include "ROSBridgePublisher.h"
#include "ROSBridgeSubscriber.h"
#include "geometry_msgs/Twist.h"
#include "RemoteMovementComponent.generated.h"


/*
reference: https://docs.unrealengine.com/latest/INT/Programming/Tutorials/Components/4/index.html
*/

UCLASS()
class UNREALCV_API URemoteMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

	class FROSTwistSubScriber : public FROSBridgeSubscriber
	{
		URemoteMovementComponent* Component;
	public:
		FROSTwistSubScriber(const FString& InTopic, URemoteMovementComponent* Component);
		~FROSTwistSubScriber() override;
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

	TSharedPtr<FROSBridgeHandler> Handler;
	TSharedPtr<FROSBridgePublisher> Publisher;
	TSharedPtr<FROSBridgePublisher> OdomTfPublisher;
	TSharedPtr<FROSBridgePublisher> OdomPublisher;
	TSharedPtr<FROSTwistSubScriber> TwistSubScriber;

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

protected:
	void ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
};
