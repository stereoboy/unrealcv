// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UE4ROSBaseRobotPawn.generated.h"

UCLASS()
class UNREALCV_API AUE4ROSBaseRobotPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUE4ROSBaseRobotPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="UE4ROS")
	void ResetPose();

	UPROPERTY(EditDefaultsOnly, Category="UE4ROS")
	TMap<FString, FString> JointDescs;

	// Movement 
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Movement")
	float MoveForwardBaseSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	FVector MoveForwardVector;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "Movement")
	float TurnRightBaseSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Movement")
	FRotator TurnRightRotator;
};
