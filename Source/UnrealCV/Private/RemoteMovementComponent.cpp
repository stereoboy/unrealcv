#include "UnrealCVPrivate.h"
#include "CaptureManager.h"
#include "RemoteMovementComponent.h"
#include "UE4ROSBridgeManager.h"
#include "rosgraph_msgs/Clock.h"
#include "geometry_msgs/TransformStamped.h"
#include "tf2_msgs/TFMessage.h"
#include "sensor_msgs/JointState.h"
#include "nav_msgs/Odometry.h"
#include "std_msgs/Header.h"
#include "sensor_msgs/Image.h"
#include "visualization_msgs/MarkerArray.h"
#include "ROSHelper.h"
#include "Kismet/KismetMathLibrary.h"

#define BASE_LINK_HEIGHT 0.08322
/*
	Hard Coded Color Table seg_color_pallet.png
*/
TArray<uint8> LabelColorTable =
{
	 57, 181, 55, 255,
	 6, 108, 153, 255,
	 191, 105, 112, 255,
	 72, 121, 89, 255,
	 64, 225, 190, 255,
	 59, 190, 206, 255,
	 36, 13, 81, 255,
	 195, 176, 115, 255,
	 27, 171, 161, 255,
	 180, 169, 135, 255,
	 199, 26, 29, 255,
	 239, 16, 102, 255,
	 146, 107, 242, 255,
	 23, 198, 156, 255,
	 160, 89, 49, 255,
	 116, 218, 68, 255,
	 9, 236, 11, 255,
	 8, 30, 196, 255,
	 28, 67, 121, 255,
	 65, 53, 0, 255,
	 70, 52, 146, 255,
	 143, 149, 226, 255,
	 171, 126, 151, 255,
	 7, 39, 194, 255,
	 161, 120, 205, 255,
	 60, 51, 212, 255,
	 208, 80, 211, 255,
	 188, 135, 189, 255,
	 205, 72, 54, 255,
	 157, 252, 103, 255,
	 123, 21, 124, 255,
	 69, 132, 19, 255,
	 132, 237, 195, 255,
	 175, 253, 94, 255,
	 87, 251, 182, 255,
	 242, 162, 90, 255,
	 1, 29, 199, 255,
	 229, 12, 254, 255,
	 244, 196, 35, 255,
	 49, 163, 220, 255,
	 214, 254, 86, 255,
	 129, 3, 152, 255,
	 106, 31, 92, 255,
	 90, 229, 207, 255,
	 48, 75, 125, 255,
	 74, 55, 98, 255,
	 238, 129, 126, 255,
	 109, 153, 222, 255,
	 34, 152, 85, 255,
	 31, 69, 173, 255,
	 125, 128, 37, 255,
	 33, 19, 58, 255,
	 119, 57, 134, 255,
	 115, 124, 218, 255,
	 200, 0, 120, 255,
	 92, 131, 225, 255,
	 16, 90, 246, 255,
	 241, 155, 51, 255,
	 155, 97, 202, 255,
	 182, 145, 184, 255,
	 44, 232, 96, 255,
	 133, 244, 133, 255,
	 29, 191, 180, 255,
	 192, 222, 1, 255,
	 104, 242, 99, 255,
	 219, 168, 91, 255,
	 217, 54, 65, 255,
	 130, 66, 148, 255,
	 204, 102, 203, 255,
	 75, 78, 216, 255,
	 250, 20, 234, 255,
	 24, 206, 109, 255,
	 17, 194, 164, 255,
	 236, 23, 157, 255,
	 88, 114, 158, 255,
	 110, 22, 245, 255,
	 35, 17, 67, 255,
	 93, 213, 181, 255,
	 42, 179, 170, 255,
	 148, 187, 52, 255,
	 111, 200, 247, 255,
	 174, 62, 25, 255,
	 240, 25, 100, 255,
	 144, 195, 191, 255,
	 67, 36, 252, 255,
	 149, 77, 241, 255,
	 141, 33, 237, 255,
	 85, 230, 119, 255,
	 108, 34, 28, 255,
	 254, 98, 78, 255,
	 30, 161, 114, 255,
	 243, 50, 75, 255,
	 253, 226, 66, 255,
	 76, 104, 46, 255,
	 216, 234, 8, 255,
	 102, 241, 15, 255,
	 71, 14, 93, 255,
	 193, 255, 192, 255,
	 164, 41, 253, 255,
	 120, 175, 24, 255,
	 231, 243, 185, 255,
	 97, 233, 169, 255,
	 145, 215, 243, 255,
	 21, 137, 72, 255,
	 101, 113, 160, 255,
	 13, 92, 214, 255,
	 147, 140, 167, 255,
	 181, 109, 101, 255,
	 126, 118, 53, 255,
	 32, 177, 3, 255,
	 99, 63, 40, 255,
	 153, 139, 186, 255,
	 100, 207, 88, 255,
	 227, 146, 71, 255,
	 187, 38, 236, 255,
	 215, 4, 215, 255,
	 66, 211, 18, 255,
	 134, 49, 113, 255,
	 63, 42, 47, 255,
	 127, 103, 219, 255,
	 137, 240, 57, 255,
	 211, 133, 227, 255,
	 201, 71, 145, 255,
	 183, 173, 217, 255,
	 113, 40, 250, 255,
	 68, 125, 208, 255,
	 249, 186, 224, 255,
	 46, 148, 69, 255,
	 20, 85, 239, 255,
	 224, 116, 108, 255,
	 26, 214, 56, 255,
	 43, 147, 179, 255,
	 172, 188, 48, 255,
	 47, 83, 221, 255,
	 218, 166, 155, 255,
	 189, 217, 62, 255,
	 122, 180, 198, 255,
	 169, 144, 201, 255,
	 14, 2, 132, 255,
	 114, 189, 128, 255,
	 112, 227, 163, 255,
	 177, 157, 45, 255,
	 142, 86, 64, 255,
	 163, 193, 118, 255,
	 79, 32, 14, 255,
	 170, 45, 200, 255,
	 2, 81, 74, 255,
	 212, 37, 59, 255,
	 225, 35, 73, 255,
	 39, 224, 95, 255,
	 220, 170, 84, 255,
	 173, 58, 159, 255,
	 237, 91, 17, 255,
	 84, 95, 31, 255,
	 248, 201, 34, 255,
	 209, 73, 63, 255,
	 107, 235, 129, 255,
	 40, 115, 231, 255,
	 95, 74, 36, 255,
	 154, 228, 238, 255,
	 54, 212, 61, 255,
	 165, 94, 13, 255,
	 0, 174, 141, 255,
	 255, 167, 140, 255,
	 91, 93, 117, 255,
	 186, 10, 183, 255,
	 61, 28, 165, 255,
	 194, 238, 144, 255,
	 41, 158, 12, 255,
	 234, 110, 76, 255,
	 121, 9, 150, 255,
	 246, 1, 142, 255,
	 198, 136, 230, 255,
	 233, 60, 5, 255,
	 80, 250, 232, 255,
	 56, 112, 143, 255,
	 156, 70, 187, 255,
	 62, 185, 2, 255,
	 226, 223, 138, 255,
	 222, 183, 122, 255,
	 3, 245, 166, 255,
	 140, 6, 175, 255,
	 210, 59, 240, 255,
	 10, 44, 248, 255,
	 52, 82, 83, 255,
	 167, 248, 223, 255,
	 150, 15, 87, 255,
	 117, 178, 111, 255,
	 22, 84, 197, 255,
	 124, 208, 235, 255,
	 45, 76, 9, 255,
	 50, 24, 176, 255,
	 251, 159, 154, 255,
	 207, 111, 149, 255,
	 15, 231, 168, 255,
	 202, 247, 209, 255,
	 152, 205, 80, 255,
	 213, 221, 178, 255,
	 38, 8, 27, 255,
	 51, 117, 244, 255,
	 190, 68, 107, 255,
	 139, 199, 23, 255,
	 168, 88, 171, 255,
	 58, 202, 136, 255,
	 86, 46, 6, 255,
	 176, 127, 105, 255,
	 197, 249, 174, 255,
	 138, 172, 172, 255,
	 81, 142, 228, 255,
	 185, 204, 7, 255,
	 247, 61, 22, 255,
	 78, 100, 233, 255,
	 105, 65, 127, 255,
	 158, 87, 33, 255,
	 252, 156, 139, 255,
	 136, 7, 42, 255,
	 179, 99, 20, 255,
	 223, 150, 79, 255,
	 184, 182, 131, 255,
	 37, 123, 110, 255,
	 96, 138, 60, 255,
	 94, 96, 210, 255,
	 18, 48, 123, 255,
	 162, 197, 137, 255,
	 5, 18, 188, 255,
	 151, 219, 39, 255,
	 135, 143, 204, 255,
	 73, 79, 249, 255,
	 178, 64, 77, 255,
	 77, 246, 41, 255,
	 4, 154, 16, 255,
	 19, 134, 116, 255,
	 235, 122, 4, 255,
	 230, 106, 177, 255,
	 12, 119, 21, 255,
	 98, 5, 104, 255,
	 53, 130, 50, 255,
	 25, 192, 30, 255,
	 166, 165, 26, 255,
	 82, 160, 10, 255,
	 131, 43, 106, 255,
	 103, 216, 44, 255,
	 221, 101, 255, 255,
	 196, 151, 32, 255,
	 89, 220, 213, 255,
	 228, 209, 70, 255,
	 83, 184, 97, 255,
	 232, 239, 82, 255,
	 128, 164, 251, 255,
	 245, 11, 193, 255,
	 159, 27, 38, 255,
	 203, 141, 229, 255,
	 55, 56, 130, 255,
	 11, 210, 147, 255,
	 118, 203, 162, 255,
	 206, 47, 43, 255,
};

URemoteMovementComponent::FROSPoseStampedSubScriber::FROSPoseStampedSubScriber(const FString& InTopic,  URemoteMovementComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("geometry_msgs/PoseStamped"))
{
	this->Component = Component;
}

URemoteMovementComponent::FROSPoseStampedSubScriber::~FROSPoseStampedSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> URemoteMovementComponent::FROSPoseStampedSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<geometry_msgs::PoseStamped> PoseStampedMessage = MakeShareable<geometry_msgs::PoseStamped>(new geometry_msgs::PoseStamped());
	PoseStampedMessage->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(PoseStampedMessage);
}

void URemoteMovementComponent::FROSPoseStampedSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<geometry_msgs::PoseStamped> PoseStampedMessage = StaticCastSharedPtr<geometry_msgs::PoseStamped>(InMsg);
	UE_LOG(LogUnrealCV, Log, TEXT("Message received! Content: %s"), *PoseStampedMessage->ToString());

	// Convert Relative Translation to Global Translation
	FVector  Position = FROSHelper::ConvertPointROSToUE4(PoseStampedMessage->GetPose().GetPosition());
	FQuat Orientation = FROSHelper::ConvertQuatROSToUE4(PoseStampedMessage->GetPose().GetOrientation());

	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	if (Pawn)
		Pawn->SetActorLocationAndRotation(Position, Orientation.Rotator());
	return;
}

URemoteMovementComponent::FROSTwistSubScriber::FROSTwistSubScriber(const FString& InTopic,  URemoteMovementComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("geometry_msgs/Twist"))
{
	this->Component = Component;
}

URemoteMovementComponent::FROSTwistSubScriber::~FROSTwistSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> URemoteMovementComponent::FROSTwistSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<geometry_msgs::Twist> TwistMessage = MakeShareable<geometry_msgs::Twist>(new geometry_msgs::Twist());
	TwistMessage->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(TwistMessage);
}

void URemoteMovementComponent::FROSTwistSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<geometry_msgs::Twist> TwistMessage = StaticCastSharedPtr<geometry_msgs::Twist>(InMsg);
	UE_LOG(LogUnrealCV, Log, TEXT("Message received! Content: %s"), *TwistMessage->ToString());

	// do something with the message

	// Convert Relative Translation to Global Translation
	FVector  Linear = FROSHelper::ConvertVectorROSToUE4(TwistMessage->GetLinear());
	FRotator Angular = FROSHelper::ConvertEulerAngleROSToUE4(TwistMessage->GetAngular());

	APawn* Pawn = FUE4CVServer::Get().GetPawn();
	FRotator CtrlRot = Pawn->GetControlRotation();
	FVector foward = UKismetMathLibrary::GetForwardVector(CtrlRot);
	FVector right = UKismetMathLibrary::GetRightVector(CtrlRot);
	FVector up = UKismetMathLibrary::GetUpVector(CtrlRot);

	FVector GlobalLinear = foward * Linear.X + right * Linear.Y + up * Linear.Z;
	FVector Direction;
	float Norm;
	float Factor = 1.0;
	GlobalLinear.ToDirectionAndLength(Direction, Norm);

	URemoteMovementComponent* Remote = FCaptureManager::Get().GetActor(0);
	FTransform Vel = FTransform(Angular, Direction*Norm*Factor);
	//FTransform Vel = FTransform(Angular, Linear);
	this->Component->SetVelocityCmd(Vel);

	UE_LOG(LogUnrealCV, Log, TEXT("Transform: (%f %f %f), (%f %f %f)"), Linear.X, Linear.Y, Linear.Z, Angular.Roll, Angular.Pitch, Angular.Yaw);
	return;
}

URemoteMovementComponent::FROSJointStateSubScriber::FROSJointStateSubScriber(const FString& InTopic, URemoteMovementComponent* Component) :
	FROSBridgeSubscriber(InTopic, TEXT("sensor_msgs/JointState"))
{
	this->Component = Component;
}

URemoteMovementComponent::FROSJointStateSubScriber::~FROSJointStateSubScriber()
{
};

TSharedPtr<FROSBridgeMsg> URemoteMovementComponent::FROSJointStateSubScriber::ParseMessage
(TSharedPtr<FJsonObject> JsonObject) const
{
	TSharedPtr<sensor_msgs::JointState> Message = MakeShareable<sensor_msgs::JointState>(new sensor_msgs::JointState());
	Message->FromJson(JsonObject);
	return StaticCastSharedPtr<FROSBridgeMsg>(Message);
}

void URemoteMovementComponent::FROSJointStateSubScriber::Callback(TSharedPtr<FROSBridgeMsg> InMsg)
{
	// downcast to subclass using StaticCastSharedPtr function
	TSharedPtr<sensor_msgs::JointState> Message = StaticCastSharedPtr<sensor_msgs::JointState>(InMsg);

	TArray<FString> Names = Message->GetName();
	TArray<double> Positions = Message->GetPosition();

	// do something with the message
	for (UGTCameraCaptureComponent* Elem : this->Component->CaptureComponentList)
	{
		bool bSet = false;
		FString ROSName = Elem->GetROSName();
		FTransform InitialTransform = Elem->GetInitialTransform();
		//float Z = InitialTransform.GetLocation().Z , Yaw = InitialTransform.Rotator().Yaw, Pitch = InitialTransform.Rotator().Pitch;
		float Z = InitialTransform.GetLocation().Z, Yaw = 0.0, Pitch = 0.0;
		for (int i = 0; i < Names.Num(); i++)
		{
			FString Name = Names[i];
			double Position = Positions[i];
			if (Name.Compare(ROSName + TEXT("_base_joint")) == 0) {
				Z += FROSHelper::ConvertVectorROSToUE4(geometry_msgs::Vector3(0.0, 0.0, Position)).Z;
				bSet = true;
			}
			else if (Name.Compare(ROSName + TEXT("_pan_joint")) == 0) {
				Yaw += FROSHelper::ConvertEulerAngleROSToUE4(geometry_msgs::Vector3(0.0, 0.0, Position)).Yaw;
				bSet = true;
			}
			else if (Name.Compare(ROSName + TEXT("_tilt_joint")) == 0) {
				Pitch += FROSHelper::ConvertEulerAngleROSToUE4(geometry_msgs::Vector3(0.0, Position, 0.0)).Pitch;
				bSet = true;
			}
		}
		if (bSet) {
			Elem->SetTargetPose(FTransform(FRotator(Pitch, Yaw, 0.0), FVector(0.0, 0.0, Z)));
			UE_LOG(LogUnrealCV, Log, TEXT("Pitch: %f, Yaw: %f, Z: %f"), Pitch, Yaw, Z);
		}
	}
	for (auto& Elem : this->Component->JointComponentMap)
	{
		for (int i = 0; i < Names.Num(); i++)
		{
			FString Name = Names[i];
			double Position = Positions[i];
			if (Name.Compare(Elem.Key) == 0) {
				Elem.Value->SetRelativeRotation(FROSHelper::ConvertEulerAngleROSToUE4(geometry_msgs::Vector3(0.0, 0.0, Position)));
			}
		}
	}
}

URemoteMovementComponent::URemoteMovementComponent()
	:bIsTicking(false)
{
	this->PrevLinear = FVector(0.0, 0.0, 0.0);
	this->PrevAngular = FRotator(0.0, 0.0, 0.0);

  LabelColorTablePubCount = 0;
  bSkeletalActorMapInitialized = false;
}

URemoteMovementComponent* URemoteMovementComponent::Create(FName Name, APawn* Pawn)
{
	//URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)GetTransientPackage(), Name);
	//URemoteMovementComponent* remoteActor = Pawn->CreateDefaultSubobject<URemoteMovementComponent>(Name);
	URemoteMovementComponent* remoteActor = NewObject<URemoteMovementComponent>((UObject *)Pawn, Name);	
	remoteActor->AddToRoot();
	remoteActor->RegisterComponent();

	remoteActor->Init();

	return remoteActor;
}

void URemoteMovementComponent::Init(void)
{
	this->bIsTicking = true;

	// Build Joint Info List
	const FRegexPattern RegexPattern(TEXT("Robot_Arm_Joint_[0-9]+"));

	APawn* OwningPawn = Cast<APawn>(this->GetOwner());

	UE_LOG(LogUnrealCV, Log, TEXT("Sub Joint Component List"));
	TArray<UActorComponent*> joints = (OwningPawn->GetComponentsByClass(UStaticMeshComponent::StaticClass()));
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));
	for (int32 idx = 0; idx < joints.Num(); ++idx)
	{
		UStaticMeshComponent* joint = Cast<UStaticMeshComponent>(joints[idx]);
		FRegexMatcher Matcher(RegexPattern, joint->GetName());
		UE_LOG(LogUnrealCV, Log, TEXT("joint[%d]->GetFullName():						%s"), idx, *joint->GetFullName());
		UE_LOG(LogUnrealCV, Log, TEXT("joint[%d]->GetName():								%s"), idx, *joint->GetName());
		UE_LOG(LogUnrealCV, Log, TEXT("joint[%d]->GetFullGroupName(false):	%s"), idx, *joint->GetFullGroupName(false));
		if (Matcher.FindNext())
		{
			FString name = joint->GetName().ToLower();
			JointComponentMap.Add(name, joint);
			UE_LOG(LogUnrealCV, Log, TEXT("-> joint(%s) added"), *name);
		}
	}
	UE_LOG(LogUnrealCV, Log, TEXT("====================================================================="));
}

void URemoteMovementComponent::SetVelocityCmd(const FTransform& InVelocityCmd)
{
	this->VelocityCmd = InVelocityCmd;
	this->MoveAnimCountFromRemote = PARAM_MOVE_ANIM_COUNT;
}

void URemoteMovementComponent::ROSPublishOdom(float DeltaTime)
{
	APawn* OwningPawn = Cast<APawn>(this->GetOwner());
	std_msgs::Header Header(0, FROSTime(), "odom");

	FTransform Transform = OwningPawn->GetActorTransform();

	// publish tf
	geometry_msgs::Transform transform = FROSHelper::ConvertTransformUE4ToROS(Transform);
	transform.SetTranslation(
		geometry_msgs::Vector3(	transform.GetTranslation().GetX(),
								transform.GetTranslation().GetY(),
								transform.GetTranslation().GetZ() - BASE_LINK_HEIGHT)
	);

	TArray<geometry_msgs::TransformStamped> transforms = { geometry_msgs::TransformStamped(Header, "base_footprint", transform) };
	TSharedPtr<tf2_msgs::TFMessage> odomtrans = MakeShareable(new tf2_msgs::TFMessage(transforms));
	Handler->PublishMsg("/tf", odomtrans);

	// publish odometry
	geometry_msgs::Vector3 vec = transform.GetTranslation();
	geometry_msgs::Quaternion quat = transform.GetRotation();
	geometry_msgs::Pose pose(geometry_msgs::Point(vec.GetX(), vec.GetY(), vec.GetZ()), quat);
	const TArray<double> posecov = {
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	};
	geometry_msgs::PoseWithCovariance	posewithcov(pose, posecov);

	geometry_msgs::Vector3 linear = FROSHelper::ConvertVectorUE4ToROS((Transform.GetLocation() - PrevLinear) / DeltaTime);
	FRotator AngularVel = FRotator((Transform.Rotator().Pitch - PrevAngular.Pitch) / DeltaTime,
		(Transform.Rotator().Yaw - PrevAngular.Yaw) / DeltaTime,
		(Transform.Rotator().Roll - PrevAngular.Roll) / DeltaTime);
	geometry_msgs::Vector3 angular = FROSHelper::ConvertEulerAngleUE4ToROS(AngularVel);
	const TArray<double> twistcov = {
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
	};
	geometry_msgs::TwistWithCovariance	twistwithcov(geometry_msgs::Twist(linear, angular), twistcov);

	TSharedPtr<nav_msgs::Odometry> odometry = MakeShareable(new nav_msgs::Odometry(Header, TEXT("base_footprint"), posewithcov, twistwithcov));
	Handler->PublishMsg("/ue4/odom", odometry);

	Handler->Process();

	PrevLinear = Transform.GetLocation();
	PrevAngular = Transform.Rotator();
}

void URemoteMovementComponent::ROSPublishJointState(float DeltaTime)
{
	std_msgs::Header Header(0, FROSTime(), "");
	TArray<FString> Names = { TEXT("main_cam_base_joint"), TEXT("main_cam_pan_joint"), TEXT("main_cam_tilt_joint") };
	TArray<double> Positions;
	TArray<double> Velocities;
	TArray<double> Efforts;

	Positions.Empty();
	Velocities.Empty();
	Efforts.Empty();

	for (UGTCameraCaptureComponent* Elem : this->CaptureComponentList)
	{
		FString ROSName = Elem->GetROSName();
		FTransform InitialTransform = Elem->GetInitialTransform();
		//float Z = InitialTransform.GetLocation().Z , Yaw = InitialTransform.Rotator().Yaw, Pitch = InitialTransform.Rotator().Pitch;
		float Z = InitialTransform.GetLocation().Z, Yaw = 0.0, Pitch = 0.0;
		for (int i = 0; i < Names.Num(); i++)
		{
			FString Name = Names[i];
			UCameraComponent* CameraComponent = Elem->GetCameraComponent();

			if (Name.Compare(ROSName + TEXT("_base_joint")) == 0) {
				Z = CameraComponent->GetRelativeTransform().GetLocation().Z - Z;
				Positions.Add(FROSHelper::ConvertVectorUE4ToROS(FVector(0.0, 0.0, Z)).GetZ());
			}
			else if (Name.Compare(ROSName + TEXT("_pan_joint")) == 0) {
				Yaw = CameraComponent->GetRelativeTransform().Rotator().Yaw - Yaw;
				Positions.Add(FROSHelper::ConvertEulerAngleUE4ToROS(FRotator(0.0, Yaw, 0.0)).GetZ());
			}
			else if (Name.Compare(ROSName + TEXT("_tilt_joint")) == 0) {
				Pitch = CameraComponent->GetRelativeTransform().Rotator().Pitch - Pitch;
				Positions.Add(FROSHelper::ConvertEulerAngleUE4ToROS(FRotator(Pitch, 0.0, 0.0)).GetY());
			}
		}
	}

	for (auto& Elem : JointComponentMap)
	{
		float Yaw = 0.0;
		Names.Add(Elem.Key);
		Yaw = Elem.Value->GetRelativeTransform().Rotator().Yaw;
		Positions.Add(FROSHelper::ConvertEulerAngleUE4ToROS(FRotator(0.0, Yaw, 0.0)).GetZ());
	}

	TSharedPtr<sensor_msgs::JointState> jointstates = MakeShareable(new sensor_msgs::JointState(Header, Names, Positions, Velocities, Efforts));
	Handler->PublishMsg("/ue4/robot/joint_states", jointstates);
}

void URemoteMovementComponent::ProcessUROSBridge(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// publish clock
	/*
	float GameTime = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	uint64 GameSeconds = (int)GameTime;
	uint64 GameUseconds = (GameTime - GameSeconds) * 1000000000;
	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable
	(new rosgraph_msgs::Clock(FROSTime(GameSeconds, GameUseconds)));
	*/

	float GameTime = GetWorld()->GetTimeSeconds();
	uint32 Secs = (uint32)GameTime;
	uint32 NSecs = (uint32)((GameTime - Secs)*1000000000);
	TSharedPtr<rosgraph_msgs::Clock> Clock = MakeShareable(new rosgraph_msgs::Clock(FROSTime(Secs, NSecs)));
	Handler->PublishMsg("/clock", Clock);

	ROSPublishOdom(DeltaTime);

	ROSPublishJointState(DeltaTime);

	if (LabelColorTablePubCount < 10)
	{
		FString Topic = TEXT("/ue4/segmentation_color_table");
		FROSTime ROSTime = FROSTime();
		std_msgs::Header Header(LabelColorTablePubCount, ROSTime, TEXT(""));
		TSharedPtr<sensor_msgs::Image> ColorTableMsg =
			MakeShareable(new sensor_msgs::Image(
				Header, 1, LabelColorTable.Num(), TEXT("rgba8")/*4Channel*/, true,
				LabelColorTable.Num() * 4, LabelColorTable)
			);

		Handler->PublishMsg(Topic, ColorTableMsg);
		LabelColorTablePubCount++;
	}

	ROSPublishSkeletalState(DeltaTime);
}
void URemoteMovementComponent::ROSBuildSkeletalState(USkeletalMeshComponent* SkeletalMeshComponent, TArray<geometry_msgs::Point> &Points, TArray<std_msgs::ColorRGBA> &Colors)
{
	FTransform Transform = SkeletalMeshComponent->GetComponentTransform();
	FVector Scale = Transform.GetScale3D();
	//UE_LOG(LogUnrealCV, Log, TEXT("Scale %f %f %f"), Scale.X, Scale.Y, Scale.Z);
	TArray<FName> Labels = {TEXT("LABEL_HEAD"), TEXT("LABEL_UARM_L"), TEXT("LABEL_UARM_R"), TEXT("LABEL_LARM_L"), TEXT("LABEL_LARM_R"), TEXT("LABEL_HAND_L"), TEXT("LABEL_HAND_R"), TEXT("LABEL_ULEG_L"), TEXT("LABEL_ULEG_R"), TEXT("LABEL_LLEG_L"), TEXT("LABEL_LLEG_R"), TEXT("LABEL_FOOT_L"), TEXT("LABEL_FOOT_R")};
	for (auto& Label: Labels)
	{
		const USkeletalMeshSocket* Socket = SkeletalMeshComponent->GetSocketByName(Label);
		if (Socket)
		{
			//vec = FROSHelper::ConvertVectorUE4ToROS(Socket->GetSocketLocation(SkeletalMeshComponent));
			geometry_msgs::Vector3 vec = FROSHelper::ConvertVectorUE4ToROS(SkeletalMeshComponent->GetBoneLocation(Socket->BoneName, EBoneSpaces::ComponentSpace));
			Points.Add(geometry_msgs::Point(Scale.X*vec.GetX(), Scale.Y*vec.GetY(), Scale.Z*vec.GetZ()));
			//Colors.Add(std_msgs::ColorRGBA(1.0, 1.0, 0.0, 1.0));
		}
	}
/*
	TArray<FName> Labels_TypeB = {TEXT("head"), TEXT("upperarm_l"), TEXT("upperarm_r"), TEXT("lowerarm_l"), TEXT("lowerarm_r"), TEXT("hand_l"), TEXT("hand_r"), TEXT("upperleg_l"), TEXT("upperleg_r"), TEXT("lowerleg_l"), TEXT("lowerleg_r")};
	geometry_msgs::Vector3 vec;
	for (auto& Label: Labels_TypeB)
	{
		vec = FROSHelper::ConvertVectorUE4ToROS(SkeletalMeshComponent->GetBoneLocation(Label, EBoneSpaces::ComponentSpace));
		if (vec.Size() > 0)
		{
			Points.Add(geometry_msgs::Point(Scale.X*vec.GetX(), Scale.Y*vec.GetY(), Scale.Z*vec.GetZ()));
			//Colors.Add(std_msgs::ColorRGBA(1.0, 1.0, 0.0, 1.0));
		}
	}
*/

	TArray<FName> Labels_TypeC = {TEXT("Head"), TEXT("LeftArm"), TEXT("RightArm"), TEXT("LeftForeArm"), TEXT("RightForeArm"), TEXT("LeftHand"), TEXT("RightHand"), TEXT("LeftUpLeg"), TEXT("RightUpLeg"), TEXT("LeftLeg"), TEXT("RightLeg"), TEXT("LeftFoot"), TEXT("RightFoot")};
	for (auto& Label: Labels_TypeC)
	{
		FVector Vec = SkeletalMeshComponent->GetBoneLocation(Label, EBoneSpaces::ComponentSpace);
		if (Vec.Size() > 0)
		{
			geometry_msgs::Vector3 vec = FROSHelper::ConvertVectorUE4ToROS(Vec);
			Points.Add(geometry_msgs::Point(Scale.X*vec.GetX(), Scale.Y*vec.GetY(), Scale.Z*vec.GetZ()));
			//Colors.Add(std_msgs::ColorRGBA(1.0, 1.0, 0.0, 1.0));
		}
	}

//	TSharedPtr<visualization_msgs::Marker> MarkerMsg = MakeShareable(new visualization_msgs::Marker(
//				Header, Name,
//				visualization_msgs::Marker::ARROW, visualization_msgs::Marker::ADD,
//				pose, scale,
//				color, 0,
//				true,
//        Points, Colors, TEXT("TEST"), TEXT(""), false
//				));
//	Handler->PublishMsg("/ue4/person", MarkerMsg);
}

void URemoteMovementComponent::ROSPublishSkeletalState(float DeltaTime)
{
	if (!bSkeletalActorMapInitialized)
	{
		UE_LOG(LogUnrealCV, Log, TEXT("Initialize SkeletalActorMap"));
		for (AActor* Actor : FUE4CVServer::Get().GetPawn()->GetLevel()->Actors)
		{
			if (Actor && Actor->GetHumanReadableName().Compare(TEXT("Player")))
			{
				TArray<UMeshComponent*> PaintableComponents;
				Actor->GetComponents<UMeshComponent>(PaintableComponents);
				if (PaintableComponents.Num() == 0)
				{
					continue;
				}
				for (auto MeshComponent : PaintableComponents)
				{
					if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
					{
						// TODO
						SkeletalActorMap.Add(Actor->GetHumanReadableName(), Actor);
					}
				}
			}
		}
		bSkeletalActorMapInitialized = true;
	}

	TArray<visualization_msgs::Marker> MarkerArray;
	for (auto Elem : SkeletalActorMap)
	{
		// TODO
		FString Name = Elem.Key;
		AActor *Actor = Elem.Value;
		//UE_LOG(LogUnrealCV, Log, TEXT("Publish Marker for %s"), *Name);
		FROSTime ROSTime = FROSTime();
		std_msgs::Header Header(0, ROSTime, TEXT("odom"));
		FTransform Transform = Actor->GetActorTransform();
    FVector Scale = Transform.GetScale3D();
		geometry_msgs::Transform transform = FROSHelper::ConvertTransformUE4ToROS(Transform);
		geometry_msgs::Vector3 vec = transform.GetTranslation();
		geometry_msgs::Quaternion quat = transform.GetRotation();
		geometry_msgs::Pose pose(geometry_msgs::Point(vec.GetX(), vec.GetY(), vec.GetZ()), quat);
		geometry_msgs::Vector3 scale(0.3, 0.1, 0.1);
		TArray<geometry_msgs::Point> Points;
		TArray<std_msgs::ColorRGBA> Colors;

		TArray<UMeshComponent*> PaintableComponents;
		Actor->GetComponents<UMeshComponent>(PaintableComponents);

		for (auto MeshComponent : PaintableComponents)
		{
			if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(MeshComponent))
			{
				int ColorID = SkeletalMeshComponent->CustomDepthStencilValue;
				float B = LabelColorTable[4*ColorID], G = LabelColorTable[4*ColorID + 1], R = LabelColorTable[4*ColorID + 2], A = LabelColorTable[4*ColorID + 3];
				//UE_LOG(LogUnrealCV, Log, TEXT("ID = %d, Color = %f %f %f %f"), ColorID, R, G, B, A);
				std_msgs::ColorRGBA color(R/255.0, G/255.0, B/255.0, A/255.0);
				//				Marker.SetColor(std_msgs::ColorRGBA(R, G, B, A));
				visualization_msgs::Marker Marker(
						Header, Name+TEXT("_arrow"),
						visualization_msgs::Marker::ARROW, visualization_msgs::Marker::ADD,
						pose, scale,
						color, 0,
						true,
						Points, Colors, TEXT("TEST"), TEXT(""), false
						);
				MarkerArray.Add(Marker);

				ROSBuildSkeletalState(SkeletalMeshComponent, Points, Colors);

				geometry_msgs::Vector3 scale2(0.1, 0.1, 0.1);
				visualization_msgs::Marker Marker2(
							Header, Name,
							visualization_msgs::Marker::POINTS, visualization_msgs::Marker::ADD,
							pose, scale2,
							color, 0,
							true,
							Points, Colors, TEXT("TEST"), TEXT(""), false
						);
				MarkerArray.Add(Marker2);
				break;
			}
		}
	}
	TSharedPtr<visualization_msgs::MarkerArray> MarkerArrayMsg = MakeShareable(new visualization_msgs::MarkerArray(MarkerArray));
	//UE_LOG(LogUnrealCV, Log, TEXT("%s"), *MarkerArrayMsg->ToString());
	Handler->PublishMsg("/ue4/persons", MarkerArrayMsg);
}

void URemoteMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!this->bIsTicking)
	{
		return;
	}
	//UE_LOG(LogUnrealCV, Log, TEXT("URemoteMovementComponent::Tick()"));
	APawn* OwningPawn = Cast<APawn>(this->GetOwner());
	const AController* OwningController = OwningPawn ? OwningPawn->GetController() : nullptr;
	if (OwningController && OwningController->IsLocalPlayerController())
	{
		if (this->MoveAnimCountFromRemote > 0)
		{
			FVector DeltaXYZ = this->VelocityCmd.GetLocation()*DeltaTime*UE4ROS_LINEAR_MOVEMENT_SCALE_FACTOR;
			FRotator Rot = this->VelocityCmd.Rotator()*UE4ROS_ANGULAR_MOVEMENT_SCALE_FACTOR;

			OwningPawn->AddMovementInput(DeltaXYZ);
			OwningPawn->AddControllerPitchInput(Rot.Pitch*DeltaTime);
			OwningPawn->AddControllerYawInput(Rot.Yaw*DeltaTime);
			OwningPawn->AddControllerRollInput(Rot.Roll*DeltaTime);

			this->MoveAnimCountFromRemote--;
		}

		ProcessUROSBridge(DeltaTime, TickType, ThisTickFunction);
	}
}


void URemoteMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	/*
	// https://answers.unrealengine.com/questions/704875/cannot-set-max-fps-in-417.html
	// This code write the setting on GameUserSettings.ini
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	Settings->SetFrameRateLimit(60.0f);
	Settings->ApplySettings(true);
	*/
	/*
	// https://answers.unrealengine.com/questions/221428/console-exec-command-from-c-fails-to-run.html
	APlayerController* Controller = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (Controller)
	{
		FString result = Controller->ConsoleCommand(TEXT("t.MaxFPS 60"));
	}
	*/
	/*
	UWorld* World = FUE4CVServer::Get().GetGameWorld();
	bool result = World->Exec(World, TEXT("t.MaxFPS 60"));
	*/
	/*
	 * MaxFPS Setup is very important on LINUX. If it setup too high value for fast rendering, data order can be mixed up.
	 *
	 */
	UE_LOG(LogUnrealCV, Warning, TEXT("URemoteMovementComponent::BeginPlay()"));
	// setup for CustomDepthStencil buffer for label images
	GEngine->Exec(GetWorld(), TEXT("r.CustomDepth 3"));
	// FPS limitation for publishing stability
	GEngine->Exec(GetWorld(), TEXT("stat FPS"));
	GEngine->Exec(GetWorld(), TEXT("t.MaxFPS 10"));

	// Set websocket server address to ws://127.0.0.1:9001
	Handler = MakeShareable<FROSBridgeHandler>(new FROSBridgeHandler(TEXT(ROS_MASTER_ADDR), ROS_MASTER_PORT));

	// Add topic subscribers and publishers
	// Add service clients and servers
	// **** Create publishers here ****
	TSharedPtr<FROSBridgePublisher> Publisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/clock"), TEXT("rosgraph_msgs/Clock")));
	Handler->AddPublisher(Publisher);

	TSharedPtr<FROSBridgePublisher> OdomTfPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/tf"), TEXT("tf2_msgs/TFMessage")));
	Handler->AddPublisher(OdomTfPublisher);
	TSharedPtr<FROSBridgePublisher> OdomPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/odom"), TEXT("nav_msgs/Odometry")));
	Handler->AddPublisher(OdomPublisher);

	TSharedPtr<FROSBridgePublisher> JointStatePublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/robot/joint_states"), TEXT("sensor_msgs/JointState")));
	Handler->AddPublisher(JointStatePublisher);

	// Add topic subscribers
	TSharedPtr<FROSPoseStampedSubScriber> PoseStampedSubscriber = MakeShareable<FROSPoseStampedSubScriber>(new FROSPoseStampedSubScriber(TEXT("/ue4/robot/ctrl/pose"), this));
	Handler->AddSubscriber(PoseStampedSubscriber);
	TSharedPtr<FROSTwistSubScriber> TwistSubscriber = MakeShareable<FROSTwistSubScriber>(new FROSTwistSubScriber(TEXT("/ue4/robot/ctrl/move"), this));
	Handler->AddSubscriber(TwistSubscriber);
	TSharedPtr<FROSJointStateSubScriber> JointStateSubscriber = MakeShareable<FROSJointStateSubScriber>(new FROSJointStateSubScriber(TEXT("/ue4/robot/ctrl/joint_states"), this));
	Handler->AddSubscriber(JointStateSubscriber);

	FString Topic = TEXT("/ue4/segmentation_color_table");
	TSharedPtr<FROSBridgePublisher> LabelColorTablePublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(Topic, TEXT("sensor_msgs/Image")));
	Handler->AddPublisher(LabelColorTablePublisher);

	TSharedPtr<FROSBridgePublisher> MarkerArrayPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/persons"), TEXT("visualization_msgs/MarkerArray")));
	Handler->AddPublisher(MarkerArrayPublisher);

	TSharedPtr<FROSBridgePublisher> MarkerPublisher = MakeShareable<FROSBridgePublisher>(new FROSBridgePublisher(TEXT("/ue4/person"), TEXT("visualization_msgs/Marker")));
	Handler->AddPublisher(MarkerPublisher);

	// Connect to ROSBridge Websocket server.
	Handler->Connect();
}

void URemoteMovementComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	// Disconnect the handler before parent ends
	Handler->Disconnect();

	Super::EndPlay(Reason);
}
