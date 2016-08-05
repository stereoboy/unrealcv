// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "CommandDispatcher.h"
/**
 * Define different ViewModes of the scene
 */
class UNREALCV_API FCameraViewMode
{
public:
	static FCameraViewMode& Get();
	~FCameraViewMode();
	void Depth();
	void DepthWorldUnits();
	void Normal();
	void Lit();
	void Unlit();
	void Object();
	void BaseColor();
	/** The ShowFlags of this mode will be set within debugger */
	void DebugMode();
	FExecStatus SetMode(const TArray<FString>& Args); // Check input arguments
	FExecStatus GetMode(const TArray<FString>& Args);
	void SetWorld(UWorld* InWorld) { World = InWorld; }
private:
	void SetCurrentBufferVisualizationMode(FString ViewMode);
	FCameraViewMode();
	UWorld* World;
	FString CurrentViewMode;
};