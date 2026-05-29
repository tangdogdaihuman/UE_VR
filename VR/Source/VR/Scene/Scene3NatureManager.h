#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Scene3NatureManager.generated.h"

class UQuizComponent;

UCLASS()
class VR_API AScene3NatureManager : public AActor
{
    GENERATED_BODY()

public:
    AScene3NatureManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene3")
    TSubclassOf<AActor> MarkerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene3")
    AActor* TyphoonAnimRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene3")
    AActor* RiverAnimRef;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void PlayScene(FName SceneName);

    UFUNCTION()
    void ShowQuiz();

    UFUNCTION()
    void OnQuizCompleted(bool bIsCorrect, FName RowName);

private:
    TArray<FName> CompletedScenes;
    bool bIsPlaying = false;

    UPROPERTY()
    UQuizComponent* QuizComp;

    UPROPERTY()
    TArray<AActor*> Markers;
};
