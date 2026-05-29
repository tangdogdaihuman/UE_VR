#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scene2EarthManager.generated.h"

class UQuizComponent;

UCLASS()
class VR_API AScene2EarthManager : public AActor
{
    GENERATED_BODY()

public:
    AScene2EarthManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene2")
    AActor* EarthReference;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene2")
    TSubclassOf<AActor> BallClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene2")
    TSubclassOf<AActor> MarkerClass;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void SelectLocation(FName LocationName, float Latitude);

    UFUNCTION(BlueprintCallable)
    void SpawnBall();

    UFUNCTION()
    void OnBallComplete(AActor* Ball);

    UFUNCTION()
    void ShowQuiz();

    UFUNCTION()
    void OnQuizCompleted(bool bIsCorrect, FName RowName);

private:
    float CurrentLatitude = 0.0f;
    FName CurrentLocationName;
    TArray<FName> CompletedLocations;
    bool bIsZoomedIn = false;

    UPROPERTY()
    UQuizComponent* QuizComp;

    UPROPERTY()
    TArray<AActor*> Markers;

    // Tracks which ball (by Actor) was spawned from which location
    TMap<AActor*, FName> BallLocationMap;
};
