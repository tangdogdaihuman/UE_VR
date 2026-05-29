#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "QuizData.generated.h"

USTRUCT(BlueprintType)
struct VR_API FQuizRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText Question;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    TArray<FText> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    int32 CorrectIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText SuccessMessage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText FailureMessage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    float AutoAdvanceDelay = 3.0f;
};
