#pragma once
#include "CoreMinimal.h"
#include "ROSBridgeHandler.h"
#include "ROSBridgePublisher.h"
#include "UE4ROSBridgeManager.generated.h"

//#define ROS_MASTER_ADDR "192.168.1.3"
#define ROS_MASTER_ADDR "127.0.0.1"
//#define ROS_MASTER_ADDR "localhost"
#define ROS_MASTER_PORT 9090
#define ROS_FAST_MASTER_PORT 9091
UCLASS()
class UNREALCV_API AUE4ROSBridgeManager : public AActor
	// NetworkManager needs to be an UObject, so that we can bind ip and port to UI.
{
	GENERATED_BODY()

public:
	TSharedPtr<FROSBridgeHandler> ROSHandler;
	TSharedPtr<FROSBridgePublisher> Publisher;
	UFUNCTION(BlueprintCallable, Category="Connnect")
  void PublishReward();
	UFUNCTION(BlueprintCallable, Category="Connnect")
	void AttachCaptureComponentToCamera(APawn* Pawn);
private:
	TArray<UGTCaptureComponent*> CaptureComponentList;
	TArray<URemoteMovementComponent*> ActorList;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
