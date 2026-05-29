#include "Scene1DiskManager.h"
#include "Interaction/InteractionManager.h"
#include "Physics/CoriolisForceComponent.h"
#include "UI/QuizComponent.h"
#include "Level/LevelTransitionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

AScene1DiskManager::AScene1DiskManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AScene1DiskManager::BeginPlay()
{
    Super::BeginPlay();

    // Create QuizComponent and bind completion callback
    QuizComp = NewObject<UQuizComponent>(this);
    QuizComp->InitializeHardcodedQuizzes();
    QuizComp->OnQuizCompleted.AddDynamic(this, &AScene1DiskManager::OnQuizCompleted);

    // Spawn control panel in front-right of player
    if (ControlPanelClass)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            FVector CameraLoc;
            FRotator CameraRot;
            PC->GetPlayerViewPoint(CameraLoc, CameraRot);

            FVector PanelLoc = CameraLoc + CameraRot.Vector() * 150.0f + CameraRot.RotateVector(FVector(0, 80.0f, -20.0f));
            ControlPanel = GetWorld()->SpawnActor<AActor>(ControlPanelClass, PanelLoc, CameraRot, SpawnParams);
        }
    }

    // Bind action keys (keyboard 1/2/3) for scene interaction
    if (UInteractionManager* IM = GetGameInstance()->GetSubsystem<UInteractionManager>())
    {
        IM->OnActionKey.AddDynamic(this, &AScene1DiskManager::HandleActionKey);
    }

    // Create a simple rotating disk if DiskReference not set
    if (!DiskReference)
    {
        DiskReference = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),
            GetActorLocation() + FVector(300, 0, 0), FRotator::ZeroRotator);
        if (DiskReference)
        {
            UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(DiskReference, TEXT("DiskMesh"));
            Mesh->RegisterComponent();
            DiskReference->AddInstanceComponent(Mesh);
            Mesh->AttachToComponent(DiskReference->GetRootComponent(),
                FAttachmentTransformRules::KeepRelativeTransform);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Scene1DiskManager] Initialized. RequiredBalls=%d. Keys: 1=Fire 2=Speed+ 3=Speed-"), RequiredBallCount);
}

void AScene1DiskManager::SpawnBall()
{
    if (bIsInQuiz)
    {
        UE_LOG(LogTemp, Warning, TEXT("[Scene1DiskManager] Cannot spawn ball: quiz active"));
        return;
    }

    if (!BallClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[Scene1DiskManager] BallClass not set"));
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // Spawn in front of disk; if no disk reference, spawn in front of camera
    FVector SpawnLoc;
    FRotator SpawnRot = FRotator::ZeroRotator;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();

    if (DiskReference)
    {
        SpawnLoc = DiskReference->GetActorLocation() + DiskReference->GetActorForwardVector() * 150.0f;
        SpawnLoc.Z += 100.0f;
    }
    else if (PC)
    {
        FVector CameraLoc;
        FRotator CameraRot;
        PC->GetPlayerViewPoint(CameraLoc, CameraRot);
        SpawnLoc = CameraLoc + CameraRot.Vector() * 200.0f;
        SpawnLoc.Z -= 30.0f;
    }
    else
    {
        SpawnLoc = GetActorLocation() + GetActorForwardVector() * 200.0f;
    }

    AActor* Ball = GetWorld()->SpawnActor<AActor>(BallClass, SpawnLoc, SpawnRot, SpawnParams);
    if (!Ball) return;

    UCoriolisForceComponent* CoriolisComp = Ball->FindComponentByClass<UCoriolisForceComponent>();
    if (CoriolisComp)
    {
        CoriolisComp->CurrentLatitude = 90.0f;
        CoriolisComp->BaseForceCoefficient = GetDiskRotationSpeed() * 50.0f + 100.0f;
        CoriolisComp->OnBallLanded.AddDynamic(this, &AScene1DiskManager::OnBallDestroyed);

        FVector LaunchDir = FVector::ForwardVector;
        if (PC)
        {
            LaunchDir = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraRotation().Vector() : FVector::ForwardVector;
        }
        CoriolisComp->Launch(LaunchDir * 500.0f);
    }

    SpawnedBallCount++;
    UE_LOG(LogTemp, Log, TEXT("[Scene1DiskManager] Ball spawned (%d/%d)"), SpawnedBallCount, RequiredBallCount);
}

void AScene1DiskManager::OnBallDestroyed(AActor* Ball)
{
    if (!Ball) return;

    DestroyedBallCount++;
    UE_LOG(LogTemp, Log, TEXT("[Scene1DiskManager] Ball destroyed (%d/%d)"), DestroyedBallCount, RequiredBallCount);

    if (DestroyedBallCount >= RequiredBallCount && !bIsInQuiz)
    {
        ShowQuiz();
    }
}

void AScene1DiskManager::ShowQuiz()
{
    bIsInQuiz = true;
    if (QuizComp)
    {
        QuizComp->ShowQuiz(FName("Scene1"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[Scene1DiskManager] QuizComp or DataTable not ready"));
    }
}

void AScene1DiskManager::OnQuizCompleted(bool bIsCorrect, FName RowName)
{
    if (bIsCorrect)
    {
        // Transition to Scene2 after a short delay
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
        {
            if (ULevelTransitionSubsystem* Trans = GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>())
            {
                Trans->TransitionToLevel(FName("Scene2_EarthLevel"), 1.0f);
            }
        }, 3.0f, false);
    }
    else
    {
        // Allow retry
        bIsInQuiz = false;
    }
}

void AScene1DiskManager::HandleActionKey(int32 Index)
{
    if (bIsInQuiz)
    {
        // Quiz mode: 1=A, 2=B, 3=C
        if (QuizComp && QuizComp->IsQuizActive())
        {
            QuizComp->SubmitAnswer(Index - 1); // zero-based
        }
        return;
    }

    // Normal mode: 1=SpawnBall, 2=SpeedUp, 3=SpeedDown
    switch (Index)
    {
    case 1: SpawnBall(); break;
    case 2:
        if (DiskReference)
        {
            RotationSpeed += 50.0f;
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green,
                FString::Printf(TEXT("Disk Speed: %.0f"), RotationSpeed));
        }
        break;
    case 3:
        if (DiskReference)
        {
            RotationSpeed = FMath::Max(0.0f, RotationSpeed - 50.0f);
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green,
                FString::Printf(TEXT("Disk Speed: %.0f"), RotationSpeed));
        }
        break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("[Scene1DiskManager] Unknown action key: %d"), Index);
        break;
    }
}

void AScene1DiskManager::SwitchView()
{
    // Toggle between global and disk close-up views
    // Blueprint handles camera interpolation
    UE_LOG(LogTemp, Log, TEXT("[Scene1DiskManager] SwitchView called"));
}

float AScene1DiskManager::GetDiskRotationSpeed() const
{
    // Attempt to read rotation speed from the disk reference via reflection.
    // Blueprint subclasses can call a BP-implemented function instead.
    if (!DiskReference) return 0.0f;

    UClass* DiskClass = DiskReference->GetClass();
    FProperty* Prop = DiskClass->FindPropertyByName(FName("RotationSpeed"));
    if (!Prop) return 0.0f;

    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
    {
        return FloatProp->GetPropertyValue_InContainer(DiskReference);
    }
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
    {
        return static_cast<float>(DoubleProp->GetPropertyValue_InContainer(DiskReference));
    }
    return 0.0f;
}
