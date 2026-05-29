#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(Blueprintable, MinimalAPI)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class VR_API IInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void OnRayHover();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void OnRayClick();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void ResetState();

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    FString GetInteractableName() const;
};
