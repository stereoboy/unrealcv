#include "UnrealCVPrivate.h"
#include "UE4ROSBridgeManager.h"
#include "rosgraph_msgs/Clock.h"

void AUE4ROSBridgeManager::BeginPlay()
{
	Super::BeginPlay();
	// Set websocket server address to ws://127.0.0.1:9001
	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT("192.168.18.128"), 9090));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****
	Publisher = Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("rosgraph_msgs/Clock"), TEXT("clock")));
	Handler->AddPublisher(Publisher);

	// Connect to ROSBridge Websocket server.
	Handler->Connect();
}

void AUE4ROSBridgeManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Do something

	float GameTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	uint64 GameSeconds = (int)GameTime;
	uint64 GameUseconds = (GameTime - GameSeconds) * 1000000000;
	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable
	(new rosgraph_msgs::Clock(FROSTime(GameSeconds, GameUseconds)));
	Handler->PublishMsg("clock", Clock);

	Handler->Process();
}

void AUE4ROSBridgeManager::EndPlay(const EEndPlayReason::Type Reason)
{
	Handler->Disconnect();
	// Disconnect the handler before parent ends

	Super::EndPlay(Reason);
}