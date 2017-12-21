#include "UnrealCVPrivate.h"

DEFINE_LOG_CATEGORY(LogUnrealCV);

class FUnrealCVPlugin : public IModuleInterface
{
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FUnrealCVPlugin, UnrealCV)

void FUnrealCVPlugin::StartupModule()
{
	FString DeviceProfileName = UDeviceProfileManager::Get().GetActiveProfileName();
	UDeviceProfile *DeviceProfile = UDeviceProfileManager::Get().GetActiveProfile();
	//FString DeviceProfileName = DeviceProfile->BaseProfileName;
	UE_LOG(LogUnrealCV, Warning, TEXT("====================================================================="));
	UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfileName = %s"), *DeviceProfileName);
	UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfile->BaseProfileName = %s"), *DeviceProfile->BaseProfileName);
	UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfile->DeviceType = %s"), *DeviceProfile->DeviceType);
	UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfile->ConfigPlatform = %s"), *DeviceProfile->ConfigPlatform);
	for (auto elem : DeviceProfile->CVars)
	{
		UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfile->ConfigPlatform = %s %s"), *elem);
	}
	UE_LOG(LogUnrealCV, Warning, TEXT("====================================================================="));

	FUE4CVServer &Server = FUE4CVServer::Get();
	Server.RegisterCommandHandlers();
	if (DeviceProfileName.Equals(TEXT("WindowsNoEditor")) || DeviceProfileName.Equals(TEXT("LinuxNoEditor")))
		Server.Config.Port += 1;

	Server.NetworkManager->Start(Server.Config.Port);
}

void FUnrealCVPlugin::ShutdownModule()
{
}
