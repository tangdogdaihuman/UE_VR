#include "Scene2EarthManager.h"
#include "Physics/CoriolisForceComponent.h"
#include "UI/QuizComponent.h"
#include "Level/LevelTransitionSubsystem.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

AScene2EarthManager::AScene2EarthManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AScene2EarthManager::BeginPlay()
{
    Super::BeginPlay();

    // Create QuizComponent
    QuizComp = NewObject<UQuizComponent>(this);
    QuizComp->InitializeHardcodedQuizzes();
    QuizComp->OnQuizCompleted.AddDynamic(this, &AScene2EarthManager::OnQuizCompleted);

    // Generate 5 latitude markers around the earth model
    if (MarkerClass && EarthReference)
    {
        struct FLatMarker { FName Name; float Lat; FVector LocalOffset; };
        TArray<FLatMarker> MarkerDefs = {
            { FName("NorthPole"),  90.0f,  FVector(0, 0, 200) },
            { FName("NorthHem"),   45.0f,  FVector(150, 0, 150) },
            { FName("Equator"),    0.0f,   FVector(200, 0, 0) },
            { FName("SouthHem"),  -45.0f,  FVector(150, 0, -150) },
            { FName("SouthPole"), -90.0f,  FVector(0, 0, -200) }
        };

        for (const FLatMarker& Def : MarkerDefs)
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            FVector WorldLoc = EarthReference->GetActorLocation() + EarthReference->GetActorRotation().RotateVector(Def.LocalOffset);
            AActor* Marker = GetWorld()->SpawnActor<AActor>(MarkerClass, WorldLoc, FRotator::ZeroRotator, SpawnParams);
            if (Marker)
            {
                Markers.Add(Marker);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Scene2EarthManager] Initialized. Markers=%d"), Markers.Num());
}

void AScene2EarthManager::SelectLocation(FName LocationName, float Latitude)
{
    if (bIsZoomedIn) return;

    CurrentLocationName = LocationName;
    CurrentLatitude = Latitude;
    bIsZoomedIn = true;

    UE_LOG(LogTemp, Log, TEXT("[Scene2EarthManager] Selected: %s (Lat=%.1f)"), *LocationName.ToString(), Latitude);
}

void AScene2EarthManager::SpawnBall()
{
    if (!bIsZoomedIn || !BallClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Scene2EarthManager] Cannot spawn: zoomed=%d class=%s"),
            bIsZoomedIn, BallClass ? TEXT("valid") : TEXT("null"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    FVector SpawnLoc = EarthReference ? EarthReference->GetActorLocation() : GetActorLocation();
    FVector Forward = FVector::ForwardVector;
    if (PC && PC->PlayerCameraManager)
    {
        Forward = PC->PlayerCameraManager->GetCameraRotation().Vector();
    }
    SpawnLoc += Forward * 200.0f;

    AActor* Ball = GetWorld()->SpawnActor<AActor>(BallClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
    if (!Ball) return;

    UCoriolisForceComponent* CoriolisComp = Ball->FindComponentByClass<UCoriolisForceComponent>();
    if (CoriolisComp)
    {
        CoriolisComp->CurrentLatitude = CurrentLatitude;
        CoriolisComp->BaseForceCoefficient = 200.0f;

        // Track which location this ball came from (solves multi-ball race condition)
        BallLocationMap.Add(Ball, CurrentLocationName);

        // Bind landing callback BEFORE Launch (avoids race if ball collides instantly)
        CoriolisComp->OnBallLanded.AddDynamic(this, &AScene2EarthManager::OnBallComplete);

        CoriolisComp->Launch(Forward * 500.0f);
    }

    bIsZoomedIn = false;
    UE_LOG(LogTemp, Log, TEXT("[Scene2EarthManager] Ball spawned at %s"), *CurrentLocationName.ToString());
}

void AScene2EarthManager::OnBallComplete(AActor* Ball)
{
    if (!Ball) return;

    // Look up which location this ball was launched from
    FName* LocationPtr = BallLocationMap.Find(Ball);
    if (!LocationPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Scene2EarthManager] Unknown ball landed: %s"), *Ball->GetName());
        return;
    }

    FName LocationName = *LocationPtr;
    BallLocationMap.Remove(Ball);  // cleanup

    CompletedLocations.AddUnique(LocationName);
    UE_LOG(LogTemp, Log, TEXT("[Scene2EarthManager] Complete: %s (%d/5)"),
        *LocationName.ToString(), CompletedLocations.Num());

    if (CompletedLocations.Num() >= 5)
    {
        ShowQuiz();
    }
}

void AScene2EarthManager::ShowQuiz()
{
    if (QuizComp)
    {
        QuizComp->ShowQuiz(FName("Scene2"));
    }
}

void AScene2EarthManager::OnQuizCompleted(bool bIsCorrect, FName RowName)
{
    if (bIsCorrect)
    {
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (ULevelTransitionSubsystem* Trans = GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>())
            {
                Trans->TransitionToLevel(FName("Scene3_NatureLevel"), 1.0f);
            }
        }, 3.0f, false);
    }
}
