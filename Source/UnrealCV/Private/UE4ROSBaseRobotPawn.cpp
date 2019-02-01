// Fill out your copyright notice in the Description page of Project Settings.

//#include "UnrealCV.h"
#include "UnrealCVPrivate.h"
#include "UE4ROSBaseRobotPawn.h"


// Sets default values
AUE4ROSBaseRobotPawn::AUE4ROSBaseRobotPawn()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUE4ROSBaseRobotPawn::BeginPlay()
{
	Super::BeginPlay();

}

void AUE4ROSBaseRobotPawn::ResetPose_Implementation()
{

}

// Called every frame
void AUE4ROSBaseRobotPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUE4ROSBaseRobotPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

