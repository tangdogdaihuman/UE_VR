#include "Scene3NatureManager.h"
#include "UI/QuizComponent.h"
#include "Level/LevelTransitionSubsystem.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "TimerManager.h"

AScene3NatureManager::AScene3NatureManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AScene3NatureManager::BeginPlay()
{
    Super::BeginPlay();

    // Create QuizComponent
    QuizComp = NewObject<UQuizComponent>(this);
    QuizComp->InitializeHardcodedQuizzes();
    QuizComp->OnQuizCompleted.AddDynamic(this, &AScene3NatureManager::OnQuizCompleted);

    // Generate 2 scene-choice markers (typhoon, river) near the earth model
    if (MarkerClass)
    {
        struct FSceneMarker { FName Name; FVector Offset; };
        TArray<FSceneMarker> Defs = {
            { FName("Typhoon"), FVector(200, 150, 50) },
            { FName("River"),   FVector(250, -100, 20) }
        };

        for (const FSceneMarker& Def : Defs)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            FVector WorldLoc = GetActorLocation() + Def.Offset;
            AActor* Marker = GetWorld()->SpawnActor<AActor>(MarkerClass, WorldLoc, FRotator::ZeroRotator, SpawnParams);
            if (Marker)
            {
                Markers.Add(Marker);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Scene3NatureManager] Initialized. Markers=%d"), Markers.Num());
}

void AScene3NatureManager::PlayScene(FName SceneName)
{
    if (bIsPlaying) return;
    bIsPlaying = true;

    UE_LOG(LogTemp, Log, TEXT("[Scene3NatureManager] Playing: %s"), *SceneName.ToString());

    if (SceneName == FName("Typhoon") && TyphoonAnimRef)
    {
        // Trigger typhoon animation (counter-clockwise rotation)
        // Animation control delegated to blueprint via function call
        CompletedScenes.AddUnique(FName("Typhoon"));
    }
    else if (SceneName == FName("River") && RiverAnimRef)
    {
        // Trigger river erosion animation (right-bank erosion)
        CompletedScenes.AddUnique(FName("River"));
    }

    bIsPlaying = false;

    if (CompletedScenes.Num() >= 2)
    {
        ShowQuiz();
    }
}

void AScene3NatureManager::ShowQuiz()
{
    if (QuizComp)
    {
        QuizComp->ShowQuiz(FName("Scene3"));
    }
}

void AScene3NatureManager::OnQuizCompleted(bool bIsCorrect, FName RowName)
{
    if (bIsCorrect)
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (ULevelTransitionSubsystem* Trans = GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>())
            {
                Trans->TransitionToLevel(FName("MainMenu"), 2.0f);
            }
        }, 3.0f, false);
    }
}
