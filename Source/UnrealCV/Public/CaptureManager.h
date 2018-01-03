#pragma once
#include "GTCaptureComponent.h"
#include "RemoteMovementComponent.h"

class FCaptureManager
{
private:
	FCaptureManager() {}
	TArray<UGTCaptureComponent*> CaptureComponentList;
	TArray<URemoteMovementComponent*> ActorList;

public:
	void AttachGTCaptureComponentToCamera(APawn* Pawn);
	static FCaptureManager& Get()
	{
		static FCaptureManager Singleton;
		return Singleton;
	};
	UGTCaptureComponent* GetCamera(int32 CameraId);
	URemoteMovementComponent* GetActor(int32 ActorId);
};
