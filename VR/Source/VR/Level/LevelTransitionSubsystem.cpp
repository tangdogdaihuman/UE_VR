#include "LevelTransitionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

void ULevelTransitionSubsystem::TransitionToLevel(FName LevelName, float FadeDuration)
{
    if (bIsLoading) return;
    bIsLoading = true;
    TargetLevelName = LevelName;
    FadeTime = FadeDuration;
    LoadProgress = 0.0f;

    UE_LOG(LogTemp, Log, TEXT("[Transition] -> %s (fade %.1fs)"), *LevelName.ToString(), FadeDuration);
    ExecuteFadeOut();
}

void ULevelTransitionSubsystem::ExecuteFadeOut()
{
    // Fade-out phase: broadcast progress for UI animation
    OnLoadProgress.Broadcast(0.0f);

    if (UWorld* World = GetWorld())
    {
        FTimerHandle Timer;
        World->GetTimerManager().SetTimer(Timer, this,
            &ULevelTransitionSubsystem::StartAsyncLoad, FadeTime, false);
    }
}

void ULevelTransitionSubsystem::StartAsyncLoad()
{
    LoadProgress = 0.1f;
    OnLoadProgress.Broadcast(LoadProgress);

    // Begin polling-based progress simulation while the async load runs
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(ProgressPollTimer, this,
            &ULevelTransitionSubsystem::PollLoadProgress, 0.1f, true);
    }

    // Use OpenLevel with a post-load fade-in. The level load itself is
    // internally async in UE5; we simulate progress via the polling timer
    // and then execute the actual transition.
    FTimerHandle LoadTimer;
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(LoadTimer, this,
            &ULevelTransitionSubsystem::ExecuteOpenLevel, 0.2f, false);
    }
}

void ULevelTransitionSubsystem::PollLoadProgress()
{
    // Simulate incremental progress while level loads
    LoadProgress = FMath::Min(LoadProgress + 0.15f, 0.9f);
    OnLoadProgress.Broadcast(LoadProgress);

    if (LoadProgress >= 0.9f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(ProgressPollTimer);
        }
    }
}

void ULevelTransitionSubsystem::ExecuteOpenLevel()
{
    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::OpenLevel(World, TargetLevelName);
    }

    // OpenLevel is synchronous: when it returns we are in the new level.
    // The GameInstanceSubsystem persists across level loads, so we can
    // safely call ExecuteFadeIn here.
    if (UWorld* NewWorld = GetWorld())
    {
        // Defer FadeIn by one frame so the new level's BeginPlay runs first
        FTimerHandle FadeInTimer;
        NewWorld->GetTimerManager().SetTimer(FadeInTimer, this,
            &ULevelTransitionSubsystem::ExecuteFadeIn, 0.01f, false);
    }
    else
    {
        // Fallback: if no world (shouldn't happen), just finish
        ExecuteFadeIn();
    }
}

void ULevelTransitionSubsystem::ExecuteFadeIn()
{
    LoadProgress = 1.0f;
    OnLoadProgress.Broadcast(LoadProgress);
    bIsLoading = false;

    UE_LOG(LogTemp, Log, TEXT("[Transition] Complete -> %s"), *TargetLevelName.ToString());
}
