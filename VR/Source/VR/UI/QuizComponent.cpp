#include "QuizComponent.h"
#include "QuizData.h"
#include "Engine/DataTable.h"

UQuizComponent::UQuizComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UQuizComponent::InitializeHardcodedQuizzes()
{
    // Populate quizzes inline — no .uasset DataTable required

    FQuizRow Scene1;
    Scene1.Question = FText::FromString(TEXT("地转偏向力何时最大？"));
    Scene1.Options = { FText::FromString(TEXT("低速慢转时")), FText::FromString(TEXT("高速快转时")) };
    Scene1.CorrectIndex = 1;
    Scene1.SuccessMessage = FText::FromString(TEXT("正确！旋转越快、物体运动越快，地转偏向力越大。"));
    Scene1.FailureMessage = FText::FromString(TEXT("再想想——转速和力的大小有关。"));
    Scene1.AutoAdvanceDelay = 3.0f;
    HardcodedQuizzes.Add(FName("Scene1"), Scene1);

    FQuizRow Scene2;
    Scene2.Question = FText::FromString(TEXT("地转偏向力最大的位置是？"));
    Scene2.Options = {
        FText::FromString(TEXT("赤道")),
        FText::FromString(TEXT("中纬度")),
        FText::FromString(TEXT("两极"))
    };
    Scene2.CorrectIndex = 2;
    Scene2.SuccessMessage = FText::FromString(TEXT("正确！北右南左赤道无，纬度越高力越强。"));
    Scene2.FailureMessage = FText::FromString(TEXT("再想想——纬度高低如何影响偏转？"));
    Scene2.AutoAdvanceDelay = 3.0f;
    HardcodedQuizzes.Add(FName("Scene2"), Scene2);

    FQuizRow Scene3;
    Scene3.Question = FText::FromString(TEXT("北半球台风逆时针、河流右岸侵蚀的核心原因是？"));
    Scene3.Options = {
        FText::FromString(TEXT("地球公转")),
        FText::FromString(TEXT("地转偏向力向右作用"))
    };
    Scene3.CorrectIndex = 1;
    Scene3.SuccessMessage = FText::FromString(TEXT("正确！地转偏向力是地球自转引发的惯性力。"));
    Scene3.FailureMessage = FText::FromString(TEXT("再想想——是地球自转还是公转的影响？"));
    Scene3.AutoAdvanceDelay = 3.0f;
    HardcodedQuizzes.Add(FName("Scene3"), Scene3);

    UE_LOG(LogTemp, Log, TEXT("[Quiz] Hardcoded quizzes initialized (%d rows)"), HardcodedQuizzes.Num());
}

const FQuizRow* UQuizComponent::FindQuizRow(FName RowName) const
{
    // Try DataTable first, then hardcoded
    if (QuizDataTable)
    {
        return QuizDataTable->FindRow<FQuizRow>(RowName, TEXT("FindQuizRow"));
    }

    if (const FQuizRow* Row = HardcodedQuizzes.Find(RowName))
    {
        return Row;
    }

    return nullptr;
}

void UQuizComponent::ShowQuiz(FName RowName)
{
    CurrentRowName = RowName;
    const FQuizRow* Row = FindQuizRow(RowName);
    if (!Row)
    {
        UE_LOG(LogTemp, Error, TEXT("[Quiz] Row not found: %s"), *RowName.ToString());
        return;
    }

    CorrectAnswerIndex = Row->CorrectIndex;
    UE_LOG(LogTemp, Log, TEXT("[Quiz] Show: %s (answer=%d, source=%s)"),
        *RowName.ToString(), CorrectAnswerIndex,
        QuizDataTable ? TEXT("DataTable") : TEXT("Hardcoded"));
}

void UQuizComponent::SubmitAnswer(int32 SelectedIndex)
{
    UE_LOG(LogTemp, Log, TEXT("[Quiz] Submit: %d (correct=%d)"), SelectedIndex, CorrectAnswerIndex);

    const FQuizRow* Row = FindQuizRow(CurrentRowName);

    if (SelectedIndex == CorrectAnswerIndex)
    {
        CleanupPopups();
        if (Row) DisplayResultPopup(Row->SuccessMessage, true);
        OnQuizCompleted.Broadcast(true, CurrentRowName);
    }
    else
    {
        if (Row) DisplayResultPopup(Row->FailureMessage, false);
    }
}

void UQuizComponent::ForceClose() { CleanupPopups(); }

void UQuizComponent::DisplayResultPopup(const FText& Message, bool bIsSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("[Quiz] Result: %s (%s)"),
        *Message.ToString(), bIsSuccess ? TEXT("SUCCESS") : TEXT("FAILURE"));
}

void UQuizComponent::CleanupPopups()
{
    if (CurrentPopup.IsValid()) { CurrentPopup->Destroy(); CurrentPopup.Reset(); }
    if (ResultPopup.IsValid()) { ResultPopup->Destroy(); ResultPopup.Reset(); }
}
