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

	class FROSResetSubScriber : public FROSBridgeSubscriber
	{
		AUE4ROSBridgeManager* Component;
	public:
		FROSResetSubScriber(const FString& InTopic, AUE4ROSBridgeManager* InComponent);
		~FROSResetSubScriber() override;
		TSharedPtr<FROSBridgeMsg> ParseMessage(TSharedPtr<FJsonObject> JsonObject) const override;
		void Callback(TSharedPtr<FROSBridgeMsg> InMsg) override;
	};

public:
	AUE4ROSBridgeManager();

	TSharedPtr<FROSBridgeHandler> ROSHandler;

	UFUNCTION(BlueprintCallable, Category="Connnect")
	void HandleHit();
	UFUNCTION(BlueprintCallable, Category="Connnect")
	void AttachCaptureComponentToCamera(APawn* Pawn);
	UFUNCTION(BlueprintCallable, Category="Connnect")
	void ResetCharacterPoses();

	UPROPERTY(EditAnywhere, Category="Connnect")
	float Reward = 0.0f;

	UPROPERTY(EditAnywhere, Category="Connnect")
	float EpisodeDone = 0.0f;
	void SetEpisodeDone(float InEpisodeDone) { EpisodeDone = InEpisodeDone; }

	bool IsHit = false;

	FROSTime GetROSSimTime()
	{
		float GameTime = GetWorld()->GetTimeSeconds();
		uint32 Secs = (uint32)GameTime;
		uint32 NSecs = (uint32)((GameTime - Secs)*1000000000);
		return FROSTime(Secs, NSecs);
	}

private:
	TArray<UGTCameraCaptureComponent*> CaptureComponentList;
	TArray<URemoteMovementComponent*> ActorList;
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
