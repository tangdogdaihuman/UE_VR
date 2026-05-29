#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuizData.h"
#include "QuizComponent.generated.h"

class UDataTable;
class UQuizPopupWidget;

UCLASS(ClassGroup=(UI), meta=(BlueprintSpawnableComponent))
class VR_API UQuizComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UQuizComponent();

    /** Optional DataTable (editor workflow). If null, uses HardcodedQuizzes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    UDataTable* QuizDataTable;

    /** Hardcoded quizzes — used when no DataTable is set. Pre-populated in InitializeHardcodedQuizzes(). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    TMap<FName, FQuizRow> HardcodedQuizzes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    float DefaultAutoCloseDelay = 3.0f;

    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void ShowQuiz(FName RowName);

    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void SubmitAnswer(int32 SelectedIndex);

    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void ForceClose();

    UFUNCTION(BlueprintCallable, Category = "Quiz")
    bool IsQuizActive() const { return CurrentPopup.IsValid(); }

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuizCompleted, bool, bIsCorrect, FName, QuizRowName);
    UPROPERTY(BlueprintAssignable, Category = "Quiz")
    FOnQuizCompleted OnQuizCompleted;

    /** Fill hardcoded quiz data for all 3 scenes. Call once in BeginPlay. */
    void InitializeHardcodedQuizzes();

private:
    FName CurrentRowName;
    int32 CorrectAnswerIndex = 0;
    TWeakObjectPtr<AActor> CurrentPopup;
    TWeakObjectPtr<AActor> ResultPopup;

    UPROPERTY()
    UQuizPopupWidget* QuizWidget;

    const FQuizRow* FindQuizRow(FName RowName) const;
    void DisplayResultPopup(const FText& Message, bool bIsSuccess);
    void CleanupPopups();
};
