#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scene1DiskManager.generated.h"

class UQuizComponent;
class UControlHUDWidget;

UCLASS()
class VR_API AScene1DiskManager : public AActor
{
    GENERATED_BODY()

public:
    AScene1DiskManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene1")
    AActor* DiskReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene1")
    int32 RequiredBallCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene1")
    TSubclassOf<AActor> BallClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene1")
    TSubclassOf<AActor> ControlPanelClass;

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void SpawnBall();

    UFUNCTION()
    void OnBallDestroyed(AActor* Ball);

    UFUNCTION()
    void ShowQuiz();

    UFUNCTION()
    void OnQuizCompleted(bool bIsCorrect, FName RowName);

    UFUNCTION()
    void HandleActionKey(int32 Index);

    UFUNCTION()
    void SwitchView();

private:
    int32 SpawnedBallCount = 0;
    int32 DestroyedBallCount = 0;
    bool bIsInQuiz = false;

    UPROPERTY()
    UQuizComponent* QuizComp;

    UPROPERTY()
    AActor* ControlPanel;

    UPROPERTY()
    UControlHUDWidget* HUDWidget;

    float RotationSpeed = 50.0f;

    float GetDiskRotationSpeed() const;
};
