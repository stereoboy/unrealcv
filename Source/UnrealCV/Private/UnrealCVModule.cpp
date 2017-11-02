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
	//FString DeviceProfileName = UDeviceProfileManager::Get().GetActiveProfileName();
	UDeviceProfile *DeviceProfile = UDeviceProfileManager::Get().GetActiveProfile();
	FString DeviceProfileName = DeviceProfile->BaseProfileName;
	UE_LOG(LogUnrealCV, Warning, TEXT("DeviceProfileName = %s"), *DeviceProfileName);
	FUE4CVServer &Server = FUE4CVServer::Get();
	Server.RegisterCommandHandlers();
	Server.NetworkManager->Start(Server.Config.Port);
}

void FUnrealCVPlugin::ShutdownModule()
{
}
