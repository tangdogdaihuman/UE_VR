#pragma once
#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "VRAssetGenerator.generated.h"

/**
 * Engine subsystem that auto-generates all missing Blueprint assets,
 * DataTable rows, and component configurations when the editor starts.
 * Skips already-existing assets (idempotent).
 */
UCLASS()
class VR_API UVRAssetGenerator : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    bool bHasRun = false;

    void GenerateAllAssets();

    UBlueprint* CreateBlueprint(const FString& AssetPath, UClass* ParentClass);
    UBlueprint* GetOrCreateBlueprint(const FString& AssetPath, UClass* ParentClass);

    void AddComponentToBlueprint(UBlueprint* BP, UClass* ComponentClass, const FName& ComponentName);
    void ImplementInterface(UBlueprint* BP, UClass* InterfaceClass);

    void EnsureDirectory(const FString& Path);
    void SaveAsset(UObject* Asset);
};
