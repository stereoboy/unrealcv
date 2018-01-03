#pragma once
#include "Object.h"
#include "RemoteMovementComponent.generated.h"

/*
reference: https://docs.unrealengine.com/latest/INT/Programming/Tutorials/Components/4/index.html
*/

UCLASS()
class UNREALCV_API URemoteMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

protected:
	URemoteMovementComponent();
	FTransform VelocityCmd;
	int32 MoveAnimCountFromRemote = 0;

	FTransform TargetPose;
	bool IsMoveToTarget = false;
	bool bIsTicking ;

public:
	static URemoteMovementComponent* Create(FName Name, APawn* Pawn);

	void Init(void);
	void SetVelocityCmd(const FTransform& InVelocityCmd);
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	virtual void BeginPlay() override;

};
