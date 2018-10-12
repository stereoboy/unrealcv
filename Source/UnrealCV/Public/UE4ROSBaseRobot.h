// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UE4ROSBaseRobot.generated.h"

UCLASS()
class UNREALCV_API AUE4ROSBaseRobot : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AUE4ROSBaseRobot();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	
	
};
