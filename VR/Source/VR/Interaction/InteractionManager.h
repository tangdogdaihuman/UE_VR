#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InteractionManager.generated.h"

UCLASS(Config=Game)
class VR_API UInteractionManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float TraceDistance = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float InteractionCooldown = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float UIUpdateInterval = 0.05f;

    /** Collision channel used for UI trace (default: GameTraceChannel2). Must match DefaultEngine.ini */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    TEnumAsByte<ECollisionChannel> UITraceChannel = ECC_GameTraceChannel2;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RegisterFloatingUI(AActor* FloatingUI);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UnregisterFloatingUI(AActor* FloatingUI);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RefreshTrace();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HandleClick();

    /** Handle action key (keyboard 1/2/3/F). Broadcasts to listeners. */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HandleActionKey(int32 Index);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionKey, int32, Index);
    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnActionKey OnActionKey;

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetActivePopup(AActor* Popup);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    AActor* GetActivePopup() const { return ActivePopup.Get(); }

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

private:
    TWeakObjectPtr<AActor> LastHitActor;
    float LastClickTime = -1.0f;
    TWeakObjectPtr<AActor> ActivePopup;
    TArray<TWeakObjectPtr<AActor>> RegisteredUIs;
    FTimerHandle UIUpdateTimerHandle;
    FTimerHandle IdlePollTimerHandle;

    void PerformLineTrace(FHitResult& OutHit);
    void ProcessHitResult(const FHitResult& Hit);
    void ProcessNoHit();
    void BatchUpdateUIOrientations();
};
