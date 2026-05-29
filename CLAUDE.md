# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

UE 5.6 project (`VR.uproject` in `VR/`) — An educational first-person interactive experience demonstrating the **Coriolis force** (科里奥利力/地转偏向力) across three progressive scenes. Built as a C++ core + Blueprint presentation hybrid. Also contains two additional game-mode variants (Horror, Shooter) derived from the base First Person template.

## Build & Development Commands

UE 5.6 is installed at `D:\software\Epic Games\UE_5.6`. The `.uproject` and `.sln` are in `VR/`.

```powershell
# Build the VR module (Development Editor, Win64)
& "D:\software\Epic Games\UE_5.6\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" VREditor Win64 Development -Project="C:\Users\admin\Desktop\UE_VR\VR\VR.uproject" -WaitMutex

# Run editor headless (validates VRAssetGenerator + no crash on startup)
& "D:\software\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\admin\Desktop\UE_VR\VR\VR.uproject" -unattended -nopause -nullrhi -log

# Open project in UE editor GUI
& "D:\software\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" "C:\Users\admin\Desktop\UE_VR\VR\VR.uproject"
```

**Note**: All C++ classes use the `VR_API` macro. Module name is `VR`. The `UE_VR` directory is just the repo folder; the actual UE module is `VR`. Kill any running `UnrealEditor*` processes before rebuilding to avoid DLL lock errors.

## High-Level Architecture

### C++ Source Layout (`VR/Source/VR/`)

The project follows a layered architecture with three distinct game variants sharing common infrastructure:

```
Core/           — IInteractableInterface (the binding contract for all interactive objects)
Interaction/    — UInteractionManager (GameInstanceSubsystem; handles ray-trace, click dispatch, popup mutual exclusion, batched UI orientation updates)
Physics/        — UCoriolisForceComponent (applies F_c = -2m(ω×v) per tick)
UI/             — UQuizComponent (self-contained quiz: hardcoded TMap, optional DataTable), FQuizRow
Scene/          — AScene1DiskManager / AScene2EarthManager / AScene3NatureManager (per-scene flow controllers)
Level/          — ULevelTransitionSubsystem (fadeOut → OpenLevel → deferred fadeIn with progress)
Editor/         — UVRAssetGenerator (EngineSubsystem: auto-creates Blueprints, components, interfaces on editor start)

VRCharacter.h/.cpp       — Base first-person Character (Enhanced Input, camera, movement)
VRGameMode.h/.cpp         — Base GameMode (mostly stub)
VRPlayerController.h/.cpp — Base PlayerController (sets camera manager class, manages IMCs)
VRCameraManager.h/.cpp    — Camera manager with pitch limits (-70° to +80°)

Variant_Horror/   — AHorrorCharacter (stamina-based sprint + flashlight), AHorrorGameMode, AHorrorPlayerController, UHorrorUI
Variant_Shooter/  — AShooterCharacter (implementing IShooterWeaponHolder, weapons inventory, HP, death/respawn),
                    AShooterGameMode (team scores), AShooterPlayerController, ShooterWeapon system,
                    AI subsystem (ShooterNPC, ShooterAIController, StateTree, EnvQuery)
```

### Key Architectural Patterns

**Interface-driven interaction**: `IInteractableInterface` provides `OnRayHover()`, `OnRayClick()`, `ResetState()`. Anything the player can aim at and click implements this interface. The `UInteractionManager` never casts to concrete types — it just calls the interface methods. Adding a new interactive item means implementing the interface, no changes to the manager.

**Subsystem registration (in `DefaultEngine.ini`)**:
```
+GameInstanceSubsystemClasses=/Script/VR.InteractionManager
+GameInstanceSubsystemClasses=/Script/VR.LevelTransitionSubsystem
+EngineSubsystemClasses=/Script/VR.VRAssetGenerator
```
Auto-created by the engine. GameInstanceSubsystems persist across level loads. VRAssetGenerator runs once on editor start, generates missing Blueprints/components/interfaces.

**Collision channel**: `ECC_GameTraceChannel2` = `UI_Trace`. Interactive objects use collision preset `Custom_UI` (blocks `UI_Trace` only). The `InteractionManager::PerformLineTrace` traces against this channel exclusively.

**Batch UI orientation**: Instead of each floating UI ticking independently, `UInteractionManager` runs a timer (every 0.05s) that iterates all registered UIs and sets their rotation to face the camera — N UIs cost 1 timer, not N ticks.

### The Coriolis Force Demo (Educational Core)

Three scenes progressively teach Coriolis force concepts:
1. **Rotating Disk** (`Scene1DiskManager`): Player adjusts disk rotation speed, launches balls, observes deflection proportional to spin rate
2. **Virtual Earth** (`Scene2EarthManager`): 5 latitude markers (90°N to 90°S), launching balls shows sin(latitude) deflection pattern
3. **Real-world Examples** (`Scene3NatureManager`): Typhoon (counter-clockwise rotation) and river (right-bank erosion) animations

Physical formula implemented in `UCoriolisForceComponent::ApplyCoriolisForce()`:
```
F_c = -2m (ω × v) × sin(latitude) ×BaseForceCoefficient
```
where ω = (0, 0, EarthRotationRate).

### Quiz System

`UQuizComponent` has two data sources: a `HardcodedQuizzes` TMap (built in `InitializeHardcodedQuizzes()` — no .uasset needed) and an optional `UDataTable*`. `FindQuizRow()` checks DataTable first, falls back to hardcoded. SceneManagers call `InitializeHardcodedQuizzes()` in BeginPlay. SubmitAnswer broadcasts `OnQuizCompleted(bIsCorrect, RowName)`.

### Variant Systems (Independent of Coriolis Demo)

**Horror** and **Shooter** are self-contained game modes sharing the base `AVRCharacter`:
- **Horror**: Stamina-gated sprint + flashlight
- **Shooter**: Weapon system (`IShooterWeaponHolder`), HP/death, AI (ShooterNPC, StateTree/EQS), team scoring

These variants share no logic with the Coriolis scenes. The base `AVRCharacter` now creates runtime InputActions (Interact + Action1/2/3) in its constructor — these are available to all three variants but only the Coriolis SceneManagers bind to `OnActionKey`.

### Input System

Uses **Enhanced Input**. InputActions are created at runtime in `AVRCharacter` constructor (`InteractAction`, `Action1Action`, `Action2Action`, `Action3Action`) — no .uasset files needed. IMCs are configured per PlayerController subclass. `LookInput()` triggers `InteractionManager::RefreshTrace()` on every camera rotation (event-driven, not per-tick).

### Keyboard Controls (Scene + Quiz)

| Key | Normal Mode | Quiz Mode |
|-----|-----------|-----------|
| 1 | Spawn ball | Answer A |
| 2 | Speed up disk | Answer B |
| 3 | Slow down disk | Answer C |
| Mouse click | Ray-trace interact | — |

`InteractionManager::HandleActionKey(Index)` broadcasts `OnActionKey` to all listeners. SceneManagers bind in BeginPlay and route to `SpawnBall()` / `SubmitAnswer()` depending on quiz state. Scene2 and Scene3 use the same pattern.

### Build Dependencies

`VR.Build.cs` public dependencies: `Core, CoreUObject, Engine, InputCore, EnhancedInput, AIModule, StateTreeModule, GameplayStateTreeModule, UMG, Slate, PhysicsCore`.

## Key Design Decisions

- **No .uasset dependency for core logic**: InputActions created at runtime, quiz data hardcoded, collision channel configurable via UPROPERTY(Config). The project compiles and runs without editor-created .uasset assets for interaction/quiz/input.
- **OnBallLanded passes AActor* Ball**: Scene2 uses `TMap<AActor*, FName> BallLocationMap` to correctly identify which latitude a landed ball came from — safe for concurrent multi-ball flight.
- **CoriolisForceComponent::IsSimulating()** (not `IsActive()`): Renamed to avoid shadowing `UActorComponent::IsActive()`.
- **Module is Runtime with UnrealEd dependency**: `VR.Build.cs` includes `UnrealEd` for `VRAssetGenerator`. This means the module only builds for Editor targets (not standalone game). Acceptable for this educational VR project.

## Reference Documentation

- `docs/requirements_v2.md` — Complete v2.0 architecture with class API pseudocode and scene flow diagrams
- `docs/需求文档_优化版.md` — Shorter Chinese architecture summary
- `docs/UE第一人称纯蓝图开发需求文档(1).pdf` — Original v1.0 blueprint-only requirements (superseded)
- `ONBOARDING.md` — New teammate onboarding guide for Claude Code usage
