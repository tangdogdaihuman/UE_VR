#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelTransitionSubsystem.generated.h"

UCLASS()
class VR_API ULevelTransitionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "LevelTransition")
    void TransitionToLevel(FName LevelName, float FadeDuration = 1.0f);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadProgress, float, Progress);
    UPROPERTY(BlueprintAssignable, Category = "LevelTransition")
    FOnLoadProgress OnLoadProgress;

    UFUNCTION(BlueprintCallable, Category = "LevelTransition")
    bool IsTransitioning() const { return bIsLoading; }

    /** Returns the current load progress (0.0 ~ 1.0) */
    UFUNCTION(BlueprintCallable, Category = "LevelTransition")
    float GetLoadProgress() const { return LoadProgress; }

    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

private:
    bool bIsLoading = false;
    FName TargetLevelName;
    float FadeTime = 1.0f;
    float LoadProgress = 0.0f;
    FTimerHandle ProgressPollTimer;

    void ExecuteFadeOut();
    void StartAsyncLoad();
    void PollLoadProgress();
    void ExecuteFadeIn();
    void ExecuteOpenLevel();
};
