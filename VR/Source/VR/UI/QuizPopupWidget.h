#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuizPopupWidget.generated.h"

class UTextBlock;
class UQuizComponent;

/**
 * Runtime-created quiz popup. No .uasset needed.
 * Displays question + options text. Answer via keyboard 1/2/3.
 */
UCLASS()
class VR_API UQuizPopupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void Setup(UQuizComponent* InQuizComp, const FText& Question, const TArray<FText>& Options);
    void ShowResult(const FText& Message, bool bIsSuccess);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY()
    UQuizComponent* QuizComp;

    UPROPERTY()
    UTextBlock* QuestionText;

    UPROPERTY()
    UTextBlock* OptionsText;

    UPROPERTY()
    UTextBlock* ResultText;

    void BuildUI();
};
