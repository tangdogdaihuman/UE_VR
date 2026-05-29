#include "VRAssetGenerator.h"

#if WITH_EDITOR
#include "Core/InteractableInterface.h"
#include "Physics/CoriolisForceComponent.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/BlueprintFactory.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/GameModeBase.h"
#include "EditorAssetLibrary.h"

void UVRAssetGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (!GIsEditor || bHasRun) return;
    bHasRun = true;
    GenerateAllAssets();
}

void UVRAssetGenerator::Deinitialize()
{
    Super::Deinitialize();
}

void UVRAssetGenerator::EnsureDirectory(const FString& Path)
{
    UEditorAssetLibrary::MakeDirectory(Path);
}

void UVRAssetGenerator::SaveAsset(UObject* Asset)
{
    if (Asset)
    {
        UEditorAssetLibrary::SaveLoadedAsset(Asset, true);
    }
}

UBlueprint* UVRAssetGenerator::CreateBlueprint(const FString& AssetPath, UClass* ParentClass)
{
    if (!ParentClass) return nullptr;

    // Parse path: /Game/Blueprints/UI/BP_FloatingUI_Base -> package=/Game/Blueprints/UI, name=BP_FloatingUI_Base
    FString PackagePath, AssetName;
    int32 LastSlash;
    if (AssetPath.FindLastChar('/', LastSlash))
    {
        PackagePath = AssetPath.Left(LastSlash);
        AssetName = AssetPath.RightChop(LastSlash + 1);
    }
    else
    {
        PackagePath = TEXT("/Game");
        AssetName = AssetPath;
    }

    // Check if already exists
    if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
    {
        UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Already exists: %s"), *AssetPath);
        return Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    }

    EnsureDirectory(PackagePath);

    UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
    Factory->ParentClass = ParentClass;

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UObject* NewAsset = AssetTools.CreateAsset(AssetName, PackagePath, UBlueprint::StaticClass(), Factory);

    if (!NewAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("[VRAssetGen] FAILED: %s"), *AssetPath);
        return nullptr;
    }

    SaveAsset(NewAsset);
    UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Created: %s"), *AssetPath);
    return Cast<UBlueprint>(NewAsset);
}

UBlueprint* UVRAssetGenerator::GetOrCreateBlueprint(const FString& AssetPath, UClass* ParentClass)
{
    UBlueprint* BP = CreateBlueprint(AssetPath, ParentClass);
    if (!BP)
    {
        BP = Cast<UBlueprint>(UEditorAssetLibrary::LoadAsset(AssetPath));
    }
    return BP;
}

void UVRAssetGenerator::AddComponentToBlueprint(UBlueprint* BP, UClass* ComponentClass, const FName& ComponentName)
{
    if (!BP || !ComponentClass) return;
    if (BP->BlueprintType != BPTYPE_Normal) return;

    USimpleConstructionScript* SCS = BP->SimpleConstructionScript;
    if (!SCS)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VRAssetGen] No SCS on %s — cannot add %s"),
            *BP->GetName(), *ComponentName.ToString());
        return;
    }

    // Check if component already exists
    const TArray<USCS_Node*>& AllNodes = SCS->GetAllNodes();
    for (USCS_Node* Node : AllNodes)
    {
        if (Node && Node->GetVariableName() == ComponentName)
        {
            UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Component already exists: %s"), *ComponentName.ToString());
            return;
        }
    }

    USCS_Node* NewNode = SCS->CreateNode(ComponentClass, ComponentName);
    if (NewNode)
    {
        SCS->AddNode(NewNode);
        UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Added component '%s' to %s"),
            *ComponentName.ToString(), *BP->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[VRAssetGen] Failed to create node '%s' in %s"),
            *ComponentName.ToString(), *BP->GetName());
    }
}

void UVRAssetGenerator::ImplementInterface(UBlueprint* BP, UClass* InterfaceClass)
{
    if (!BP || !InterfaceClass) return;

    // Check if already implemented
    for (const FBPInterfaceDescription& Iface : BP->ImplementedInterfaces)
    {
        if (Iface.Interface == InterfaceClass)
        {
            UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Interface already implemented: %s on %s"),
                *InterfaceClass->GetName(), *BP->GetName());
            return;
        }
    }

    FBPInterfaceDescription NewIface;
    NewIface.Interface = InterfaceClass;
    BP->ImplementedInterfaces.Add(NewIface);
    UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Implemented interface '%s' on %s"),
        *InterfaceClass->GetName(), *BP->GetName());
}

void UVRAssetGenerator::GenerateAllAssets()
{
    UE_LOG(LogTemp, Display, TEXT("==================================================="));
    UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Auto-generating Coriolis project assets..."));
    UE_LOG(LogTemp, Display, TEXT("==================================================="));

    UClass* ActorClass = AActor::StaticClass();
    UClass* GameModeClass = AGameModeBase::StaticClass();

    // ====== UI Blueprints ======
    UBlueprint* BP_FloatingUI_Base = GetOrCreateBlueprint(TEXT("/Game/Blueprints/UI/BP_FloatingUI_Base"), ActorClass);
    if (BP_FloatingUI_Base)
    {
        AddComponentToBlueprint(BP_FloatingUI_Base, UWidgetComponent::StaticClass(), FName("FloatingWidget"));
        // Implement IInteractableInterface — need to get the class from module
        if (UClass* Iface = UInteractableInterface::StaticClass())
        {
            ImplementInterface(BP_FloatingUI_Base, Iface);
        }
        SaveAsset(BP_FloatingUI_Base);
    }

    GetOrCreateBlueprint(TEXT("/Game/Blueprints/UI/BP_UI_Button"), ActorClass);
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/UI/BP_UI_Slider"), ActorClass);
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/UI/BP_UI_Popup"), ActorClass);
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/UI/BP_UI_Marker"), ActorClass);

    // ====== Scene 1 ======
    UBlueprint* BP_Disk = GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene1/BP_Rotate_Disk"), ActorClass);
    if (BP_Disk)
    {
        AddComponentToBlueprint(BP_Disk, UStaticMeshComponent::StaticClass(), FName("DiskMesh"));
        SaveAsset(BP_Disk);
    }

    UBlueprint* BP_S1Ball = GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene1/BP_Scene1_Ball"), ActorClass);
    if (BP_S1Ball)
    {
        AddComponentToBlueprint(BP_S1Ball, UStaticMeshComponent::StaticClass(), FName("BallMesh"));
        AddComponentToBlueprint(BP_S1Ball, UCoriolisForceComponent::StaticClass(), FName("CoriolisForce"));
        SaveAsset(BP_S1Ball);
    }

    GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene1/BP_UI_ControlPanel"), ActorClass);

    // ====== Scene 2 ======
    UBlueprint* BP_Earth = GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene2/BP_Virtual_Earth"), ActorClass);
    if (BP_Earth)
    {
        AddComponentToBlueprint(BP_Earth, UStaticMeshComponent::StaticClass(), FName("EarthMesh"));
        SaveAsset(BP_Earth);
    }

    UBlueprint* BP_S2Ball = GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene2/BP_Scene2_Ball"), ActorClass);
    if (BP_S2Ball)
    {
        AddComponentToBlueprint(BP_S2Ball, UStaticMeshComponent::StaticClass(), FName("BallMesh"));
        AddComponentToBlueprint(BP_S2Ball, UCoriolisForceComponent::StaticClass(), FName("CoriolisForce"));
        SaveAsset(BP_S2Ball);
    }

    // ====== Scene 3 ======
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene3/BP_Typhoon_Anim"), ActorClass);
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/Scene3/BP_River_Anim"), ActorClass);

    // ====== Game Framework ======
    GetOrCreateBlueprint(TEXT("/Game/Blueprints/BP_FirstPersonGameMode"), GameModeClass);

    // Quiz data is hardcoded in UQuizComponent::InitializeHardcodedQuizzes() — no DataTable needed

    UE_LOG(LogTemp, Display, TEXT("==================================================="));
    UE_LOG(LogTemp, Display, TEXT("[VRAssetGen] Asset generation complete!"));
    UE_LOG(LogTemp, Display, TEXT("==================================================="));
}

#endif // WITH_EDITOR
