// Fill out your copyright notice in the Description page of Project Settings.

//#include "UnrealCV.h"
#include "UnrealCVPrivate.h"
#include "UE4ROSBaseRobot.h"


// Sets default values
AUE4ROSBaseRobot::AUE4ROSBaseRobot()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUE4ROSBaseRobot::BeginPlay()
{
	Super::BeginPlay();
	
}

void AUE4ROSBaseRobot::ResetPose_Implementation()
{

}

// Called every frame
void AUE4ROSBaseRobot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUE4ROSBaseRobot::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

