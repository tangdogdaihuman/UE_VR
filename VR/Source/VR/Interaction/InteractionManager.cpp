#include "InteractionManager.h"
#include "Core/InteractableInterface.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

void UInteractionManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UWorld* World = GetWorld();
    if (!World) return;

    World->GetTimerManager().SetTimer(
        UIUpdateTimerHandle, this,
        &UInteractionManager::BatchUpdateUIOrientations,
        UIUpdateInterval, true);

    // Idle polling: ensures hover state doesn't stale when player isn't moving (per spec §2.2)
    World->GetTimerManager().SetTimer(
        IdlePollTimerHandle, this,
        &UInteractionManager::RefreshTrace,
        0.1f, true);

    UE_LOG(LogTemp, Log, TEXT("[InteractionManager] Initialized. TraceDist=%.0f UIUpdate=%.3fs"),
        TraceDistance, UIUpdateInterval);
}

void UInteractionManager::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UIUpdateTimerHandle);
        World->GetTimerManager().ClearTimer(IdlePollTimerHandle);
    }
    RegisteredUIs.Empty();
    LastHitActor.Reset();
    ActivePopup.Reset();
    Super::Deinitialize();
}

void UInteractionManager::RegisterFloatingUI(AActor* FloatingUI)
{
    if (!FloatingUI) return;
    RegisteredUIs.AddUnique(FloatingUI);
}

void UInteractionManager::UnregisterFloatingUI(AActor* FloatingUI)
{
    if (!FloatingUI) return;
    RegisteredUIs.Remove(FloatingUI);
    if (LastHitActor.Get() == FloatingUI) LastHitActor.Reset();
}

void UInteractionManager::RefreshTrace()
{
    FHitResult Hit;
    PerformLineTrace(Hit);
    if (Hit.bBlockingHit && Hit.GetActor())
        ProcessHitResult(Hit);
    else
        ProcessNoHit();
}

void UInteractionManager::HandleClick()
{
    UWorld* World = GetWorld();
    if (!World) return;

    float CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastClickTime < InteractionCooldown) return;
    LastClickTime = CurrentTime;

    if (LastHitActor.IsValid() && LastHitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        IInteractableInterface::Execute_OnRayClick(LastHitActor.Get());
    }
}

void UInteractionManager::HandleActionKey(int32 Index)
{
    OnActionKey.Broadcast(Index);
    UE_LOG(LogTemp, Log, TEXT("[InteractionManager] ActionKey: %d"), Index);
}

void UInteractionManager::SetActivePopup(AActor* Popup)
{
    if (ActivePopup.IsValid() && ActivePopup.Get() != Popup)
    {
        ActivePopup->Destroy();
    }
    ActivePopup = Popup;
}

void UInteractionManager::PerformLineTrace(FHitResult& OutHit)
{
    UWorld* World = GetWorld();
    if (!World) return;
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    FVector Start = CameraLocation;
    FVector End = Start + CameraRotation.Vector() * TraceDistance;

    FCollisionQueryParams QueryParams;
    if (AActor* Pawn = PC->GetPawn())
    {
        QueryParams.AddIgnoredActor(Pawn);
    }
    QueryParams.AddIgnoredActor(PC);
    QueryParams.bTraceComplex = false;

    World->LineTraceSingleByChannel(OutHit, Start, End, UITraceChannel.GetValue(), QueryParams);
}

void UInteractionManager::ProcessHitResult(const FHitResult& Hit)
{
    AActor* HitActor = Hit.GetActor();
    if (!HitActor) return;

    if (LastHitActor.IsValid() && LastHitActor.Get() != HitActor)
    {
        if (LastHitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            IInteractableInterface::Execute_ResetState(LastHitActor.Get());
    }

    if (HitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
        IInteractableInterface::Execute_OnRayHover(HitActor);

    LastHitActor = HitActor;
}

void UInteractionManager::ProcessNoHit()
{
    if (LastHitActor.IsValid())
    {
        if (LastHitActor->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
            IInteractableInterface::Execute_ResetState(LastHitActor.Get());
        LastHitActor.Reset();
    }
}

void UInteractionManager::BatchUpdateUIOrientations()
{
    UWorld* World = GetWorld();
    if (!World) return;
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;

    FVector CameraLocation;
    FRotator CameraRotation;
    PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

    RegisteredUIs.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

    for (const TWeakObjectPtr<AActor>& UIPtr : RegisteredUIs)
    {
        AActor* UI = UIPtr.Get();
        if (!UI) continue;
        FVector DirToCamera = CameraLocation - UI->GetActorLocation();
        DirToCamera.Z = 0.0f;
        if (!DirToCamera.IsNearlyZero())
            UI->SetActorRotation(DirToCamera.Rotation());
    }
}
