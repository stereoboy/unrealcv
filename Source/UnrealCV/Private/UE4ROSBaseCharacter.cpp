// Fill out your copyright notice in the Description page of Project Settings.

//#include "UnrealCV.h"
#include "UnrealCVPrivate.h"
#include "UE4ROSBaseCharacter.h"


// Sets default values
AUE4ROSBaseCharacter::AUE4ROSBaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AUE4ROSBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AUE4ROSBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AUE4ROSBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

