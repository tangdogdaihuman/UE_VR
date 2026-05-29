#include "CoriolisForceComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UCoriolisForceComponent::UCoriolisForceComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCoriolisForceComponent::BeginPlay()
{
    Super::BeginPlay();
    AActor* Owner = GetOwner();
    if (Owner)
        TargetPrimitive = Owner->FindComponentByClass<UPrimitiveComponent>();
}

void UCoriolisForceComponent::Launch(FVector InitialVelocity)
{
    if (!TargetPrimitive) return;
    TargetPrimitive->SetSimulatePhysics(true);
    TargetPrimitive->SetPhysicsLinearVelocity(InitialVelocity);
    bIsActive = true;
    UE_LOG(LogTemp, Log, TEXT("[CoriolisForce] Launch: Lat=%.1f Coeff=%.1f"), CurrentLatitude, BaseForceCoefficient);
}

void UCoriolisForceComponent::Stop()
{
    if (!bIsActive) return;  // prevent double-broadcast
    bIsActive = false;
    if (TargetPrimitive)
        TargetPrimitive->SetSimulatePhysics(false);

    OnBallLanded.Broadcast(GetOwner());
    UE_LOG(LogTemp, Log, TEXT("[CoriolisForce] Ball landed. Lat=%.1f"), CurrentLatitude);
}

void UCoriolisForceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (bIsActive) ApplyCoriolisForce(DeltaTime);
}

void UCoriolisForceComponent::ApplyCoriolisForce(float DeltaTime)
{
    if (!TargetPrimitive || !bIsActive) return;
    FVector Velocity = TargetPrimitive->GetPhysicsLinearVelocity();

    const float LatitudeRad = FMath::DegreesToRadians(CurrentLatitude);
    const float CoriolisCoefficient = FMath::Sin(LatitudeRad);
    if (FMath::IsNearlyZero(CoriolisCoefficient)) return;

    // F_c = -2m (omega x v)
    const FVector AngularVelocity(0.0f, 0.0f, EarthRotationRate);
    const FVector CoriolisForce = -2.0f * Mass *
        FVector::CrossProduct(AngularVelocity, Velocity) *
        CoriolisCoefficient * BaseForceCoefficient;

    TargetPrimitive->AddForce(CoriolisForce);
}
