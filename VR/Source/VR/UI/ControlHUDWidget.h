#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlHUDWidget.generated.h"

class UTextBlock;

UCLASS()
class VR_API UControlHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetStatus(const FString& StatusText);
    void SetCue(const FString& CueText);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY()
    UTextBlock* StatusLabel;

    UPROPERTY()
    UTextBlock* CueLabel;

    void BuildUI();
};
