#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoriolisForceComponent.generated.h"

UCLASS(ClassGroup=(Physics), meta=(BlueprintSpawnableComponent))
class VR_API UCoriolisForceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoriolisForceComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float CurrentLatitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float BaseForceCoefficient = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics",
        meta = (ClampMin = "0.001", ClampMax = "10.0"))
    float EarthRotationRate = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics",
        meta = (ClampMin = "0.01"))
    float Mass = 1.0f;

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void Launch(FVector InitialVelocity);

    UFUNCTION(BlueprintCallable, Category = "Physics")
    void Stop();

    /** Returns true while the ball is in flight. */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    bool IsSimulating() const { return bIsActive; }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBallLanded, AActor*, Ball);
    UPROPERTY(BlueprintAssignable, Category = "Physics")
    FOnBallLanded OnBallLanded;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bIsActive = false;
    UPROPERTY()
    UPrimitiveComponent* TargetPrimitive = nullptr;
    void ApplyCoriolisForce(float DeltaTime);
};
