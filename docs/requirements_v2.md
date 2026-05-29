# UE 第一人称开发需求文档（优化版）

**版本：2.0**
**面向：C++ 核心 + 蓝图表现层混合开发**
**核心规则：第一人称视角，屏幕中心固定射线检测，全部交互使用中心射线 + 世界空间漂浮 UI**
**备注：基于 v1.0 纯蓝图版优化架构，性能优先，可扩展优先**

---

## 项目背景

本项目的核心教育目标是**演示地转偏向力（科里奥利力）**——地球自转引发的一种惯性力，它使北半球的运动物体向右偏转，南半球向左偏转，赤道处为零。项目通过三个递进式场景让玩家直观感受这一物理现象。

v1.0 原版需求文档采用"纯蓝图"实现方式。经过分析，该项目存在以下结构性问题：
- 物理计算（正弦、叉积、每帧力施加）用蓝图实现效率极低
- 全部交互逻辑塞在 BP_FirstPersonCharacter 内，形成"上帝类"
- 三个场景的答题逻辑完全重复（复制粘贴），新增场景需大量重复工作
- 每帧 Tick 中执行射线检测和 UI 朝向更新，大量不必要的 CPU 开销
- 同步 Open Level 加载，关卡复杂时可能卡死

v2.0 版本将计算密集型逻辑下沉到 C++，蓝图层仅保留 UI 布局、动画、材质等表现层工作。同时架构设计预留了 VR 扩展能力——当前使用屏幕中心射线交互，未来可无缝切换为 VR 手柄射线。

---

## 一、架构总览

### 1.1 分层架构

```
┌──────────────────────────────────────────────┐
│              蓝图层 (Blueprints)               │
│  UI 布局  │  材质/特效  │  动画驱动  │  关卡美术  │
│                                            │
│  → 零复杂逻辑，仅做表现层配置                    │
└──────────────────┬───────────────────────────┘
                   │ 继承/实现接口
┌──────────────────▼───────────────────────────┐
│             C++ 逻辑层 (Runtime)              │
│  UInteractionManager  │  UCoriolisForceComponent │
│  UQuizComponent       │  各场景Manager (Actor)   │
│                                            │
│  → 全部计算、状态机、物理模拟在此层               │
└──────────────────┬───────────────────────────┘
                   │ 依赖
┌──────────────────▼───────────────────────────┐
│            C++ 基础层 (Foundation)             │
│  IInteractableInterface  │  FQuizRow (DataTable) │
│  ULevelTransitionSubsystem  │  碰撞通道配置       │
│                                            │
│  → 接口定义、数据结构、跨模块基础设施              │
└──────────────────────────────────────────────┘
```

### 1.2 核心设计原则

| 原则 | 说明 |
|------|------|
| **蓝图层只做表现** | 蓝图不写复杂逻辑，只负责 UI 布局、动画、材质参数调整 |
| **C++ 承担所有计算** | 物理计算、交互状态机、答题判断全部下沉到 C++ |
| **接口解耦** | 全部可交互对象实现 `IInteractableInterface`，调用方不关心具体类型 |
| **DataTable 驱动** | 答题内容、场景配置全部表格化，修改无需重新编译 |
| **事件驱动替代 Tick** | 射线检测在视角变化时触发（非每帧），UI 朝向由统一定时器批量更新 |
| **VR 可扩展** | 当前第一人称中心射线，架构预留手柄射线/手势交互接入点 |

### 1.3 全局交互规则

1. 射线命中 `IInteractableInterface` 实现者 → 调用 `OnRayHover()`
2. 点击输入 → 调用当前命中对象的 `OnRayClick()`
3. 射线离开 → 调用上一命中对象的 `ResetState()`
4. 弹窗互斥：全局仅允许一个弹窗存在，新弹窗创建时自动销毁旧弹窗
5. 交互冷却：点击后 200ms 内忽略所有重复点击

## 二、C++ 核心层设计

### 2.1 交互接口 — IInteractableInterface

这是整个交互系统的基石。任何可以被射线"看到并点击"的对象（漂浮 UI、场景中的可交互 Actor）都实现此接口。调用方（InteractionManager）无需知道命中对象的类型，只需调用接口方法。

**v1.0 的问题：** 原文档在 BP_FirstPersonCharacter 的 Tick 中用 `Cast<BP_FloatingUI_Base>` 判断命中对象类型，每新增一种可交互类型就要改角色蓝图。

**v2.0 方案：** 定义统一接口，新增可交互类型只需实现接口，InteractionManager 零修改。

```cpp
// InteractableInterface.h
#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

UINTERFACE(Blueprintable, MinimalAPI)
class UInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class UE_VR_API IInteractableInterface
{
    GENERATED_BODY()

public:
    /** 射线悬浮时触发 — UI 切换高亮/悬浮样式 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void OnRayHover();

    /** 鼠标左键按下时触发 — 执行按钮/滑块的核心功能 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void OnRayClick();

    /** 射线离开时触发 — UI 恢复默认样式 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    void ResetState();

    /** 可选：返回调试名称，用于日志输出 */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Interaction")
    FString GetInteractableName() const;
};
```

**接口方法调用流程：**

```
每帧/每次视角变化:
  RayTrace → HitActor 实现了 IInteractableInterface?
    ├── YES → HitActor->OnRayHover()           // 命中: 高亮
    │         + 如果鼠标按下 → HitActor->OnRayClick()  // 点击: 执行功能
    └── NO  → LastHitActor->ResetState()       // 未命中: 恢复
```

### 2.2 交互管理器 — UInteractionManager

**职责：** 接管原文档中散落在 BP_FirstPersonCharacter 内的全部交互逻辑——射线检测、命中状态追踪、UI 批量更新、弹窗互斥。

**为什么是 GameInstanceSubsystem：**
- 跨关卡持久存在（玩家切换场景时交互系统不重启）
- 全局唯一实例，无需手动管理生命周期
- 任意位置的代码都可以通过 `GetGameInstance()->GetSubsystem<T>()` 访问

```cpp
// InteractionManager.h
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "InteractionManager.generated.h"

UCLASS()
class UE_VR_API UInteractionManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ==================== 配置（可在编辑器/DefaultEngine.ini 中覆盖）====================

    /** 射线检测距离（厘米） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float TraceDistance = 800.0f;

    /** 交互冷却时间（秒），防止连点 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float InteractionCooldown = 0.2f;

    /** UI 批量朝向更新间隔（秒）。0.05s = 20fps，肉眼无法感知，节省大量 Tick */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Config, Category = "Interaction")
    float UIUpdateInterval = 0.05f;

    // ==================== 核心方法 ====================

    /** 注册漂浮 UI 到批量更新列表。UI 的 BeginPlay 中调用 */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RegisterFloatingUI(AActor* FloatingUI);

    /** 注销漂浮 UI。UI 的 EndPlay 中调用 */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void UnregisterFloatingUI(AActor* FloatingUI);

    /** 
     * 执行一次射线检测并处理结果。
     * 调用时机：玩家视角旋转时（InputAction 回调），而非每帧 Tick。
     * 静置时也以低频（0.1s 间隔）刷新，确保 UI Hover 状态不丢失。
     */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void RefreshTrace();

    /** 处理鼠标左键点击 */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void HandleClick();

    /** 设置当前活跃弹窗（互斥逻辑：设置前自动销毁旧弹窗） */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void SetActivePopup(AActor* Popup);

    /** 获取当前活跃弹窗 */
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    AActor* GetActivePopup() const { return ActivePopup.Get(); }

    // ==================== 子系统生命周期 ====================
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // ---- 命中状态追踪 ----
    TWeakObjectPtr<AActor> LastHitActor;   // 上一帧命中的 Actor
    float LastClickTime = -1.0f;            // 上次点击时间戳（用于冷却判断）
    TWeakObjectPtr<AActor> ActivePopup;     // 当前活跃弹窗（互斥用）

    // ---- UI 批量更新 ----
    TArray<TWeakObjectPtr<AActor>> RegisteredUIs;  // 已注册的漂浮 UI 列表
    FTimerHandle UIUpdateTimerHandle;               // 批量更新定时器

    // ---- 内部方法 ----
    void PerformLineTrace(FHitResult& OutHit);
    void ProcessHitResult(const FHitResult& Hit);
    void ProcessNoHit();
    void BatchUpdateUIOrientations();  // 定时器回调：批量更新 UI 朝向
};
```

**关键优化说明：**

| v1.0 做法 | v2.0 做法 | 收益 |
|-----------|-----------|------|
| BP_FirstPersonCharacter Tick 中每帧 LineTrace | 仅在视角旋转时 RefreshTrace()，静置时低频轮询 | 减少 70-90% 的射线检测调用 |
| 每个漂浮 UI 在自身 Tick 中 `FindLookAtRotation` | InteractionManager 定时器批量遍历 RegisteredUIs，统一计算 | N 个 UI 从 N 次 Tick 降为 1 个定时器 |
| `Cast<具体类型>` 判断命中对象 | 直接调用 `IInteractableInterface` 接口方法 | 新增可交互类型无需改动管理器 |
| 弹窗互斥逻辑写在角色蓝图 | SetActivePopup() 统一管理 | 逻辑集中，便于调试 |

### 2.3 科里奥利力物理组件 — UCoriolisForceComponent

这是整个项目**物理正确性的核心**。原文档使用 `sin(纬度) * 系数` 后硬编码"右侧/左侧"方向——这在小球旋转后方向会出错，且物理上不严谨。

**修正为标准的科里奥利力叉积公式：**

```
F_c = -2m (ω × v)

其中：
  ω = 地球自转角速度向量（沿世界 Z 轴向上）
  v = 小球当前速度向量
  m = 小球质量
  × = 向量叉积（Cross Product）

叉积自动决定偏转方向：
  北半球（纬度>0）：sin(纬度)>0 → 力向右
  南半球（纬度<0）：sin(纬度)<0 → 力向左  
  赤道（纬度=0） ：sin(0)=0    → 力为零
```

```cpp
// CoriolisForceComponent.h
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CoriolisForceComponent.generated.h"

UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent))
class UE_VR_API UCoriolisForceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCoriolisForceComponent();

    // ==================== 公开参数 ====================

    /** 当前纬度（度）。北极=90，赤道=0，南极=-90 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float CurrentLatitude = 0.0f;

    /** 基础侧向力系数（可调整以适配不同场景的视觉效果） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float BaseForceCoefficient = 100.0f;

    /** 地球自转参考角速度（rad/s）。真实值 ≈ 7.2921e-5，建议调大以增强视觉效果 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float EarthRotationRate = 0.5f;

    /** 小球质量（kg） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
    float Mass = 1.0f;

    // ==================== 核心方法 ====================

    /** 赋予初始速度，激活物理模拟 */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void Launch(FVector InitialVelocity);

    /** 停止物理模拟（小球落地时调用） */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    void Stop();

    /** 小球是否正在飞行中 */
    UFUNCTION(BlueprintCallable, Category = "Physics")
    bool IsActive() const { return bIsActive; }

    // ==================== 委托 ====================

    /** 小球落地时广播（碰撞回调中触发） */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBallLanded);
    UPROPERTY(BlueprintAssignable, Category = "Physics")
    FOnBallLanded OnBallLanded;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
                               FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bIsActive = false;
    
    /** 缓存的物理组件引用（BeginPlay 中获取） */
    UPROPERTY()
    UPrimitiveComponent* TargetPrimitive = nullptr;

    /** 每帧施加科里奥利力 */
    void ApplyCoriolisForce(float DeltaTime);
};
```

**cpp 实现（核心计算）：**

```cpp
void UCoriolisForceComponent::ApplyCoriolisForce(float DeltaTime)
{
    if (!TargetPrimitive || !bIsActive)
        return;

    FVector Velocity = TargetPrimitive->GetPhysicsLinearVelocity();

    // 纬度 → 弧度 → sin 系数
    const float LatitudeRad = FMath::DegreesToRadians(CurrentLatitude);
    const float CoriolisCoefficient = FMath::Sin(LatitudeRad);

    // 赤道处 sin(0) = 0，跳过计算
    if (FMath::IsNearlyZero(CoriolisCoefficient))
        return;

    // 科里奥利力公式：F_c = -2m (omega x v)
    // omega 沿世界 Z 轴（地球自转轴）向上
    const FVector AngularVelocity(0.0f, 0.0f, EarthRotationRate);
    const FVector CoriolisForce = -2.0f * Mass * 
        FVector::CrossProduct(AngularVelocity, Velocity) * 
        CoriolisCoefficient * BaseForceCoefficient;

    TargetPrimitive->AddForce(CoriolisForce);
}
```

**与 v1.0 的物理差异：**

| 方面 | v1.0（原版） | v2.0（优化版） |
|------|-------------|---------------|
| 偏转方向 | 硬编码 if(北半球)→右侧; if(南半球)→左侧 | 叉积自动决定方向 |
| 力的大小 | sin(纬度) * 固定系数 * 圆盘转速 | -2m(omega x v) * sin(纬度) * 系数 |
| 物理正确性 | 近似，但在小球速度方向变化后可能出错 | 严格遵循科里奥利力定义 |
| 可配置性 | 固定参数 | 全部 UPROPERTY，编辑器中可调 |

### 2.4 答题数据结构 — FQuizRow (DataTable)

**v1.0 的问题：** 三个场景的 ShowQuiz/OnQuizAnswered/ShowResult 逻辑几乎完全相同，仅题目文本和选项不同。每个场景都需要复制粘贴蓝图节点，新增题目意味着新增蓝图逻辑。

**v2.0 方案：** 将所有题目数据放入 DataTable，答题 UI（BP_UI_Popup）通过 UQuizComponent 读取 DataTable 行，一行代码完成题目加载。

```cpp
// QuizData.h
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "QuizData.generated.h"

USTRUCT(BlueprintType)
struct UE_VR_API FQuizRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 题目文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText Question;

    /** 选项列表（A/B/C/D，支持2-6个选项） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    TArray<FText> Options;

    /** 正确答案索引（0-based，对应 Options 数组） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    int32 CorrectIndex = 0;

    /** 回答正确后的科普文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText SuccessMessage;

    /** 回答错误后的提示文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    FText FailureMessage;

    /** 正确后延迟多少秒自动跳转（0 = 手动跳转） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    float AutoAdvanceDelay = 3.0f;
};
```

**DataTable 配置示例（在 UE 编辑器中编辑，无需编译）：**

| RowName | Question | Options[0] | Options[1] | Options[2] | CorrectIndex | SuccessMessage | AutoAdvanceDelay |
|---------|----------|-----------|-----------|-----------|--------------|----------------|------------------|
| Scene1 | 地转偏向力何时最大？ | 低速慢转 | 高速快转 | — | 1 | 旋转越快、物体运动越快，地转偏向力越大 | 3.0 |
| Scene2 | 地转偏向力最大的位置是？ | 赤道 | 中纬度 | 两极 | 2 | 北右南左赤道无，纬度越高，地转偏向力越强 | 3.0 |
| Scene3 | 北半球台风逆时针、河流右岸侵蚀的核心原因是？ | 地球公转 | 地转偏向力向右作用 | — | 1 | 地转偏向力是地球自转引发的惯性力，塑造了诸多自然现象 | 3.0 |

---

## 三、通用漂浮 UI 体系

### 3.1 基类 — BP_FloatingUI_Base

继承 `AActor`，实现 `IInteractableInterface`。所有漂浮 UI（按钮、滑块、弹窗、标记）的公共父类。

```
组件结构：
  DefaultSceneRoot（场景根组件）
  └── WidgetComponent（世界空间渲染，Screen 模式）
       ├── 碰撞预设：Custom_UI（仅对 UI_Trace 通道 Block）
       ├── 命中测试：开启
       └── Draw Size：由子类配置
```

**与 v1.0 的核心差异：**

| 功能 | v1.0 | v2.0 |
|------|------|------|
| 面向玩家旋转 | 每个 UI 在**自身 Tick** 中 `FindLookAtRotation` | InteractionManager 定时器**批量处理**，UI 自身零 Tick |
| 三态样式切换 | 每个子类**重复实现** Hover/Normal/Click 切换 | **基类统一实现**，子类仅提供三套材质/颜色参数 |
| 交互触发 | 依赖 WidgetComponent 的 OnMouse 事件 | 实现 `IInteractableInterface`，由 InteractionManager 统一分发 |
| 弹窗互斥 | 角色蓝图中检查 | InteractionManager::SetActivePopup() 自动销毁旧弹窗 |

### 3.2 基类统一三态逻辑

```
IInteractableInterface::OnRayHover()：
  1. 切换为悬浮样式（半透明高亮边框 / 105% 缩放微动效）
  2. 播放悬浮音效（可选）

IInteractableInterface::OnRayClick()：
  1. 切换为点击样式（全亮边框 / 95% 缩放按压感）
  2. 0.1s 后调用子类重写的 ExecuteAction()  // 子类在此实现具体功能
  3. 播放点击音效（可选）

IInteractableInterface::ResetState()：
  1. 恢复默认样式
  2. 停止所有过渡动效
```

### 3.3 子类一览

| 子类蓝图 | 用途 | 核心参数 | 与 v1.0 差异 |
|----------|------|---------|-------------|
| **BP_UI_Button** | 通用按钮（发射、确认、切换视角等） | NormalColor, HoverColor, ClickColor, ButtonText, OnClicked委托 | 三态逻辑由基类处理，子类仅配置颜色和文本 |
| **BP_UI_Slider** | 通用滑块（转速调节等） | MinValue, MaxValue, CurrentValue, OnValueChanged委托 | 拖拽计算移至 C++ 组件 USliderDragComponent，蓝图仅显示 |
| **BP_UI_Popup** | 通用弹窗（答题、科普提示） | Title, BodyText, OptionButtons数组, AutoCloseDelay | 自动关闭时间从 DataTable 读取；UQuizComponent 管理选项按钮 |
| **BP_UI_Marker** | 点位标记（地球点位选择） | LocationName, Latitude, NormalIcon, CompletedIcon | 新增 Completed 态（绿勾），由场景管理器控制切换 |

**子类与基类的职责分配：**

```
BP_FloatingUI_Base（基类 — C++ 逻辑，蓝图表现）:
  ├── 实现 IInteractableInterface::OnRayHover/OnRayClick/ResetState
  ├── 管理三态样式切换（Normal/Hover/Click）
  ├── 提供虚拟函数 ExecuteAction() 供子类重写
  └── 注册/注销到 InteractionManager
      │
      ├── BP_UI_Button:  重写 ExecuteAction() → 调用 OnClicked 委托
      ├── BP_UI_Slider:  重写 ExecuteAction() → 启动拖拽模式
      ├── BP_UI_Popup:   重写 ExecuteAction() → 不响应点击（由内部按钮处理）
      └── BP_UI_Marker:  重写 ExecuteAction() → 通知场景管理器选中此点位
```

## 四、场景 1：旋转圆盘射球

**目标：** 让玩家直观感受"旋转速度越快，科里奥利力越大"。玩家调节圆盘转速后发射小球，观察小球在不同转速下的偏转轨迹。

### 4.1 场景管理器 — AScene1Manager

继承 `AActor`，直接放置在场景中。负责场景1的全部流程控制。

```
放置方式：拖入关卡，设置圆盘引用即可

事件流程：
  BeginPlay
    │
    ├─1─ 在玩家右前方 150cm 处生成控制面板 BP_UI_ControlPanel
    │    （包含：转速滑块、发射按钮、视角切换按钮）
    │
    ├─2─ 绑定滑块 OnValueChanged → 圆盘->SetRotationSpeed(NewValue)
    ├─3─ 绑定发射按钮 OnRayClick → SpawnBall()
    ├─4─ 绑定视角切换按钮 OnRayClick → SwitchView()
    │
    └─5─ 控制面板注册到 InteractionManager
```

**核心变量：**

| 变量 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| DiskReference | ABP_Rotate_Disk* | — | 场景中的圆盘引用 |
| SpawnedBallCount | int32 | 0 | 已发射小球数 |
| RequiredBallCount | int32 | 3 | 通关所需小球数 |
| DestroyedBallCount | int32 | 0 | 已落地小球数 |
| bIsInQuiz | bool | false | 答题中禁止发射新小球 |
| ControlPanel | ABP_FloatingUI_Base* | — | 控制面板引用 |

**核心方法：**

```
SpawnBall()：
  1. 在圆盘中心正前方 N 厘米处 Spawn BP_Scene1_Ball
  2. 将圆盘当前转速传递给小球（用于调整力的强度）
  3. SpawnedBallCount++

OnBallDestroyed()：
  1. DestroyedBallCount++
  2. 若 DestroyedBallCount >= RequiredBallCount → ShowQuiz("Scene1")

ShowQuiz(RowName)：
  1. bIsInQuiz = true（锁住发射按钮）
  2. 调用 UQuizComponent::ShowQuiz(RowName)
  3. 显示答题弹窗

OnQuizCompleted(bool IsCorrect)：
  正确 → 显示科普弹窗（SuccessMessage） → AutoAdvanceDelay后切换场景2
  错误 → 显示错误提示（FailureMessage） → 保留答题弹窗

SwitchView()：
  在"全局视角"和"圆盘近景视角"之间平滑过渡相机位置
```

### 4.2 旋转圆盘 — BP_Rotate_Disk

最简单的场景组件，仅负责绕 Z 轴自转。

```
组件：
  StaticMeshComponent（圆盘模型，碰撞预设：BlockAll）

变量：
  RotationSpeed（float，默认 0） — 度/秒

Tick：
  AddActorLocalRotation(FRotator(0, RotationSpeed * DeltaTime, 0))

方法：
  SetRotationSpeed(float Speed) { RotationSpeed = Speed; }
  GetRotationSpeed() { return RotationSpeed; }
```

### 4.3 发射小球 — BP_Scene1_Ball

```
组件：
  StaticMeshComponent（球体，Simulate Physics = true）
  UCoriolisForceComponent

BeginPlay：
  1. 获取场景管理器传递的圆盘转速
  2. CoriolisForceComponent->CurrentLatitude = 90.0f  // 模拟极端纬度（演示用）
  3. CoriolisForceComponent->BaseForceCoefficient = 圆盘转速 * 调节系数
  4. CoriolisForceComponent->Launch(摄像机前向 * 初速度大小)

OnComponentHit（碰撞事件）：
  碰撞到地面/墙壁/圆盘边缘 →
    1. CoriolisForceComponent->Stop()
    2. 延迟 1 秒（让玩家观察偏转后的落点位置）
    3. Destroy()
    4. 通知 AScene1Manager::OnBallDestroyed()
```

**场景 1 物理逻辑闭环：**

```
滑块拖动 → 圆盘转速变化 → 发射小球 → 科里奥利力偏转 → 落地观察
                                                              │
玩家观察：转速=0 时小球直线 │ 转速=50 时小球微右偏 │ 转速=100 时小球大幅右偏
                                                              │
发射 3 个小球后 → 答题："地转偏向力何时最大？" → 正确 → 科普弹窗 → 场景2
```

---

## 五、场景 2：虚拟地球射球

**目标：** 让玩家理解"纬度越高，科里奥利力越强"。玩家选择地球上的 5 个纬度点位（北极/北半球/赤道/南半球/南极），分别发射小球，观察不同纬度的偏转差异。

### 5.1 场景管理器 — AScene2Manager

```
BeginPlay：
  1. 在地球模型对应位置上生成 5 个 BP_UI_Marker：
     - 北极 Marker（纬度 = 90°N，位置：地球模型顶部）
     - 北半球 Marker（纬度 = 45°N，位置：地球模型北纬 45°处）
     - 赤道 Marker（纬度 = 0°，位置：地球模型赤道处）
     - 南半球 Marker（纬度 = 45°S，位置：地球模型南纬 45°处）
     - 南极 Marker（纬度 = 90°S，位置：地球模型底部）
  2. 绑定每个 Marker 的 OnRayClick → SelectLocation(LocationName, Latitude)
  3. 发射按钮（场景中固定位置）OnRayClick → SpawnBall()
```

**核心变量：**

| 变量 | 类型 | 说明 |
|------|------|------|
| EarthReference | BP_Virtual_Earth* | 地球模型引用 |
| CurrentLatitude | float | 当前选中的纬度值 |
| CurrentLocationName | FName | 当前选中的点位名称 |
| CompletedLocations | TArray\<FName\> | 已完成（已发射小球）的点位名称列表 |
| bIsZoomedIn | bool | 地球是否处于放大聚焦状态 |

**核心方法：**

```
SelectLocation(Name, Latitude)：
  1. 记录 CurrentLatitude 和 CurrentLocationName
  2. 触发地球模型平滑放大动画（2 秒），相机聚焦选中点位
  3. 在选中点位旁显示点位名称漂浮文本（如"北极 90°N"）
  4. bIsZoomedIn = true
  5. 启亮发射按钮

SpawnBall()：
  1. 在当前聚焦点位正前方生成 BP_Scene2_Ball
  2. 将 CurrentLatitude 传递给小球
  3. bIsZoomedIn = false

OnBallComplete(LocationName)：
  1. 将 LocationName 加入 CompletedLocations
  2. 对应 Marker 切换为 CompletedIcon（绿色勾）
  3. 地球模型缩小动画，返回全局视角
  4. 检查 CompletedLocations.Num()：
     - < 5：提示"请选择下一个点位"
     - == 5：ShowQuiz("Scene2")

ShowQuiz("Scene2")：
  从 DataTable 加载题目："地转偏向力最大的位置是？" → 选项：赤道/中纬度/两极
```

### 5.2 虚拟地球 — BP_Virtual_Earth

```
组件：
  StaticMeshComponent（球体模型）
  AnimationBlueprint（控制缩放/位移动画）

动画蓝图暴露参数（供管理器设置）：
  ZoomTargetLocation：Vector — 放大目标位置（对应选中的纬度点位）
  ZoomAmount：float（0=全局视角, 1=完全聚焦）

Manager 通过 Timeline 在 2 秒内将 ZoomAmount 从 0 平滑过渡到 1，实现放大效果。
```

### 5.3 纬度偏转小球 — BP_Scene2_Ball

与场景1小球结构相同（StaticMesh + UCoriolisForceComponent），区别在于：

```
BeginPlay：
  1. CoriolisForceComponent->CurrentLatitude = 管理器传入的纬度值
     // 场景1是固定 90°，场景2是真实纬度值
  2. CoriolisForceComponent->BaseForceCoefficient = 默认值
     // 场景2不依赖转速，依赖纬度
  3. CoriolisForceComponent->Launch(小球正前方 * 初速度)

碰撞 → 自毁 → 通知 AScene2Manager::OnBallComplete(点位名称)
```

**预期物理效果（玩家在不同点位发射小球后观察到的轨迹）：**

| 点位 | 纬度 | sin(纬度) | 物理效果 |
|------|------|-----------|---------|
| 北极 | 90°N | +1.00 | 最大幅右偏，轨迹大幅向右弯曲 |
| 北半球 | 45°N | +0.71 | 中等右偏，轨迹轻微向右弯曲 |
| 赤道 | 0° | 0.00 | 无偏转，直线运动 |
| 南半球 | 45°S | -0.71 | 中等左偏，轨迹轻微向左弯曲 |
| 南极 | 90°S | -1.00 | 最大幅左偏，轨迹大幅向左弯曲 |

玩家完成全部 5 个点位的发射后，触发答题。

---

## 六、场景 3：真实地球应用

**目标：** 将地转偏向力概念与真实自然现象关联。展示北半球台风逆时针旋转和河流右岸侵蚀两个经典案例。

### 6.1 场景管理器 — AScene3Manager

```
BeginPlay：
  1. 在真实地球模型上生成 2 个场景选择 Marker：
     - "台风" Marker（放在北半球太平洋区域）
     - "河流" Marker（放在北半球某河流区域）
  2. 绑定每个 Marker 的 OnRayClick → PlayScene(SceneName)
  3. 初始状态：两个 Marker 均为可选态（闪烁提示）
```

**核心变量：**

| 变量 | 类型 | 说明 |
|------|------|------|
| CompletedScenes | TArray\<FName\> | 已观看的场景名称列表 |
| TyphoonAnimRef | BP_Typhoon_Anim* | 台风动画控制器引用 |
| RiverAnimRef | BP_River_Anim* | 河流动画控制器引用 |

**核心方法：**

```
PlayScene(SceneName)：
  ├── SceneName == "台风"：
  │    1. 隐藏其他 Marker
  │    2. 播放台风逆时针旋转动画（3-5 秒循环）
  │    3. 动画结束后生成科普弹窗："北半球台风逆时针旋转——科里奥利力使气流向右偏转"
  │    4. 弹窗关闭后 → 恢复 Marker → CompletedScenes.Add("台风")
  │
  ├── SceneName == "河流"：
  │    1. 隐藏其他 Marker
  │    2. 播放河流右岸侵蚀动画（水流冲向右侧河岸）
  │    3. 动画结束后生成科普弹窗："北半球河流右岸侵蚀——科里奥利力使水流向右偏移"
  │    4. 弹窗关闭后 → 恢复 Marker → CompletedScenes.Add("河流")
  │
  └── CompletedScenes.Num() == 2 → ShowQuiz("Scene3")

ShowQuiz("Scene3")：
  从 DataTable 加载题目：
  "北半球台风逆时针、河流右岸侵蚀的核心原因是？"
  选项：A.地球公转  B.地转偏向力向右作用
```

### 6.2 动画控制器 — BP_Typhoon_Anim / BP_River_Anim

```
继承：AActor

台风动画（BP_Typhoon_Anim）：
  - 内含多个 StaticMesh（云层粒子效果）
  - 预制 Sequencer/Matinee 动画：云层绕中心逆时针旋转
  - 暴露方法：PlayAnimation(), StopAnimation(), IsPlaying()

河流动画（BP_River_Anim）：
  - 内含 StaticMesh（河道模型）+ 粒子特效（水流）
  - 预制动画：水流向右岸偏移，右岸发生侵蚀变色
  - 暴露方法：PlayAnimation(), StopAnimation(), IsPlaying()
```

**场景3流程闭环：**

```
选择"台风" → 观看逆时针旋转 → 科普弹窗 → 返回选择
选择"河流" → 观看右岸侵蚀   → 科普弹窗 → 返回选择
两者皆完成 → 最终答题 → 正确 → 总结弹窗 → 结束界面
```

## 七、答题系统（DataTable 驱动）

### 7.1 设计思路

**v1.0 的问题：** 三个场景的答题流程完全相同（显示题目 → 判断对错 → 显示结果 → 跳转），但每个场景都重新实现了一遍蓝图节点。新增一个题目 = 复制粘贴大量蓝图，极易出错。

**v2.0 方案：** 将"题目数据"和"答题UI"完全解耦：
- 题目数据存储在 DataTable 中（策划可直接编辑，无需程序员介入）
- 答题UI由 `UQuizComponent` 统一管理
- 场景管理器只需调用 `ShowQuiz("RowName")` 并绑定完成回调

**数据流：**

```
DataTable (DT_QuizData)
    │
    │  GetRow<FQuizRow>(RowName)
    ▼
UQuizComponent::ShowQuiz()
    │
    ├── 1. 读取题目、选项、正确答案索引
    ├── 2. 生成 BP_UI_Popup（弹窗UI）
    ├── 3. 根据 Options 数组动态创建选项按钮
    ├── 4. 绑定每个按钮的 OnClick → SubmitAnswer(Index)
    └── 5. 设置自动关闭定时器（AutoAdvanceDelay）
         │
         ▼
SubmitAnswer(Index)
    ├── Index == CorrectIndex → 正确流程
    │     ├── 关闭答题弹窗
    │     ├── 显示科普弹窗（SuccessMessage）
    │     └── 广播 OnQuizCompleted(true, RowName)
    │
    └── Index != CorrectIndex → 错误流程
          ├── 显示 FailureMessage 提示
          └── 保留答题弹窗，等待重新选择
```

### 7.2 UQuizComponent 接口

```cpp
// QuizComponent.h
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuizComponent.generated.h"

class UDataTable;

UCLASS(ClassGroup = (UI), meta = (BlueprintSpawnableComponent))
class UE_VR_API UQuizComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // ==================== 配置 ====================

    /** 指向答题 DataTable（DT_QuizData） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    UDataTable* QuizDataTable;

    /** 科普弹窗的自动关闭时间（秒），可在 DataTable 中按题目覆盖 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quiz")
    float DefaultAutoCloseDelay = 3.0f;

    // ==================== 核心方法 ====================

    /** 
     * 显示指定 RowName 的题目。
     * @param RowName DataTable 中的行名（如 "Scene1", "Scene2"）
     * @param TargetPlayer 目标玩家控制器
     */
    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void ShowQuiz(FName RowName, APlayerController* TargetPlayer);

    /** 提交答案（由选项按钮调用） */
    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void SubmitAnswer(int32 SelectedIndex);

    /** 强制关闭当前答题（关卡切换时调用） */
    UFUNCTION(BlueprintCallable, Category = "Quiz")
    void ForceClose();

    /** 当前是否有答题进行中 */
    UFUNCTION(BlueprintCallable, Category = "Quiz")
    bool IsQuizActive() const { return CurrentPopup != nullptr; }

    // ==================== 委托 ====================

    /** 答题完成（正确或超时强制关闭） */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuizCompleted, bool, bIsCorrect, FName, QuizRowName);
    UPROPERTY(BlueprintAssignable, Category = "Quiz")
    FOnQuizCompleted OnQuizCompleted;

private:
    FName CurrentRowName;
    int32 CorrectAnswerIndex = 0;
    TWeakObjectPtr<AActor> CurrentPopup;
    TWeakObjectPtr<APlayerController> TargetPC;

    void DisplayResultPopup(const FText& Message, bool bIsSuccess);
};
```

### 7.3 场景管理器中的使用示例

```cpp
// 任意场景管理器的 BeginPlay 中：
void AScene1Manager::BeginPlay()
{
    Super::BeginPlay();

    // 创建 QuizComponent
    QuizComp = NewObject<UQuizComponent>(this);
    QuizComp->QuizDataTable = LoadObject<UDataTable>(nullptr, 
        TEXT("/Game/Data/DT_QuizData"));
    
    // 绑定答题完成回调
    QuizComp->OnQuizCompleted.AddDynamic(this, &AScene1Manager::OnQuizCompleted);
}

// 所有小球落地后：
void AScene1Manager::CheckComplete()
{
    if (DestroyedBallCount >= RequiredBallCount)
    {
        QuizComp->ShowQuiz("Scene1", GetWorld()->GetFirstPlayerController());
    }
}

// 答题结果处理：
void AScene1Manager::OnQuizCompleted(bool bIsCorrect, FName RowName)
{
    if (bIsCorrect)
    {
        // 正确：科普弹窗已由 QuizComponent 显示
        // 延迟后切换到下一场景
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            // 调用关卡切换
            ULevelTransitionSubsystem* Trans = 
                GetGameInstance()->GetSubsystem<ULevelTransitionSubsystem>();
            Trans->TransitionToLevel("Scene2_EarthLevel");
        }, 3.0f, false);
    }
    // 错误时 QuizComponent 自动保留答题弹窗，无需额外处理
}
```

---

## 八、关卡切换与过渡

### 8.1 设计要点

**v1.0 的问题：** 使用同步 `Open Level` + 2 秒固定等待时间。关卡较大时 2 秒不够（卡在加载中），关卡较小时 2 秒浪费。没有任何加载进度反馈。

**v2.0 方案：** 使用 UE 的异步关卡加载 API（`LoadPackageAsync` 或 `UGameplayStatics::LoadStreamLevel`），配合淡入淡出动画。

### 8.2 ULevelTransitionSubsystem

```cpp
// LevelTransitionSubsystem.h
#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelTransitionSubsystem.generated.h"

UCLASS()
class UE_VR_API ULevelTransitionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 带过渡动画的异步关卡切换。
     * 流程：淡出 → 异步加载 → 淡入
     * @param LevelName 目标关卡名称（如 "Scene2_EarthLevel"）
     * @param FadeDuration 淡入/淡出动画时长（秒）
     */
    UFUNCTION(BlueprintCallable, Category = "LevelTransition")
    void TransitionToLevel(FName LevelName, float FadeDuration = 1.0f);

    /** 加载进度（0.0 ~ 1.0），可用于更新 Loading 界面 */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadProgress, float, Progress);
    UPROPERTY(BlueprintAssignable, Category = "LevelTransition")
    FOnLoadProgress OnLoadProgress;

    /** 是否正在加载中 */
    UFUNCTION(BlueprintCallable, Category = "LevelTransition")
    bool IsTransitioning() const { return bIsLoading; }

private:
    bool bIsLoading = false;
    FName TargetLevelName;
    float FadeTime = 1.0f;

    // 内部步骤
    void ExecuteFadeOut();
    void ExecuteAsyncLoad();
    void OnLevelLoaded();
    void ExecuteFadeIn();
    
    /** 检查加载进度（轮询） */
    void PollLoadProgress();
};
```

**过渡流程时间线：**

```
时间 →
│── FadeOut ──│────── Async Load ──────│── FadeIn ──│
│ (动画)      │ (加载+进度反馈)          │ (动画)     │
│ 1.0s        │ 不定时长                  │ 1.0s       │
│             │                          │            │
│ 屏幕全黑    │ Loading界面+进度条        │ 新场景显现 │
└─────────────┴─────────────────────────┴────────────┘
```

---

## 九、项目文件结构

```
UE_VR/
├── UE_VR.uproject                          // 项目描述文件
│
├── Source/
│   └── UE_VR/
│       ├── UE_VR.Build.cs                  // 模块构建规则
│       ├── UE_VR.h                         // 模块头文件
│       ├── UE_VR.cpp                       // 模块入口
│       │
│       ├── Core/
│       │   ├── InteractableInterface.h     // 交互接口（UInterface）
│       │   └── VRGameMode.h               // 全局 GameMode（关卡跳转入口）
│       │
│       ├── Interaction/
│       │   ├── InteractionManager.h        // 交互管理器（GameInstanceSubsystem）
│       │   └── InteractionManager.cpp
│       │
│       ├── Physics/
│       │   ├── CoriolisForceComponent.h    // 科里奥利力物理组件
│       │   └── CoriolisForceComponent.cpp
│       │
│       ├── UI/
│       │   ├── QuizComponent.h             // 答题组件（DataTable驱动）
│       │   ├── QuizComponent.cpp
│       │   ├── QuizData.h                  // FQuizRow 数据结构
│       │   └── SliderDragComponent.h       // 滑块拖拽组件（可选）
│       │
│       ├── Scene/
│       │   ├── Scene1DiskManager.h         // 场景1 管理器（Actor）
│       │   ├── Scene2EarthManager.h        // 场景2 管理器（Actor）
│       │   └── Scene3NatureManager.h       // 场景3 管理器（Actor）
│       │
│       └── Level/
│           └── LevelTransitionSubsystem.h  // 关卡异步切换子系统
│
├── Content/
│   ├── Blueprints/
│   │   ├── Character/
│   │   │   └── BP_FirstPersonCharacter.uasset    // 第一人称角色（基于模板）
│   │   │
│   │   ├── UI/
│   │   │   ├── BP_FloatingUI_Base.uasset         // 漂浮UI基类
│   │   │   ├── BP_UI_Button.uasset               // 按钮
│   │   │   ├── BP_UI_Slider.uasset               // 滑块
│   │   │   ├── BP_UI_Popup.uasset                // 弹窗
│   │   │   └── BP_UI_Marker.uasset               // 点位标记
│   │   │
│   │   ├── Scene1/
│   │   │   ├── BP_Rotate_Disk.uasset             // 旋转圆盘
│   │   │   └── BP_Scene1_Ball.uasset             // 场景1小球
│   │   │
│   │   ├── Scene2/
│   │   │   ├── BP_Virtual_Earth.uasset           // 虚拟地球
│   │   │   └── BP_Scene2_Ball.uasset             // 场景2小球
│   │   │
│   │   └── Scene3/
│   │       ├── BP_Typhoon_Anim.uasset            // 台风动画
│   │       └── BP_River_Anim.uasset              // 河流动画
│   │
│   ├── Data/
│   │   └── DT_QuizData.uasset                    // 答题数据表
│   │
│   ├── Maps/
│   │   ├── MainMenu.umap                         // 主菜单/起始关卡
│   │   ├── Scene1_DiskLevel.umap                 // 场景1关卡
│   │   ├── Scene2_EarthLevel.umap                // 场景2关卡
│   │   └── Scene3_NatureLevel.umap               // 场景3关卡
│   │
│   ├── Materials/
│   │   └── ...                                   // UI材质、地球材质等
│   │
│   └── Audio/
│       └── ...                                   // 交互音效、背景音乐
│
└── Config/
    ├── DefaultEngine.ini         // 引擎配置（碰撞通道、Subsystem注册等）
    ├── DefaultGame.ini           // 游戏配置（GameMode、默认地图等）
    └── DefaultInput.ini          // 输入配置（鼠标/键盘绑定）
```

---

## 附录 A：v1.0 vs v2.0 对比总表

| 维度 | v1.0（原版纯蓝图） | v2.0（优化版 C++混合） |
|------|-------------------|----------------------|
| **实现方式** | 100% 蓝图 | C++ 核心逻辑 + 蓝图表现层 |
| **射线检测频率** | 每帧 Tick 执行 LineTrace | 视角变化时触发，静置时低频轮询 |
| **UI 朝向更新** | N 个 UI × 每帧 Tick 计算 LookAt | 1 个定时器批量更新全部 UI（0.05s 间隔） |
| **物理计算** | sin(纬度) * 系数，if/else 硬编码左右方向 | 标准叉积公式 -2m(omega x v)，叉积自动决定方向 |
| **答题系统** | 三个场景各实现一套完整的 ShowQuiz→判断→ShowResult 蓝图链 | DataTable + UQuizComponent，场景管理器仅需 ShowQuiz("RowName") |
| **交互架构** | 全部逻辑塞在 BP_FirstPersonCharacter Tick 中 | IInteractableInterface 解耦 + InteractionManager 集中管理 |
| **弹窗互斥** | 角色蓝图内 Cast 检查 + 手动 Destroy | InteractionManager::SetActivePopup() 自动管理 |
| **关卡加载** | 同步 Open Level + 固定 2 秒等待 | 异步加载 + 实际进度反馈 + 淡入淡出 |
| **新增场景** | 需修改角色蓝图、复制答题蓝图 | 新建 SceneManager 类 + DataTable 加一行 |
| **性能预估** | 每帧：1×LineTrace + N×FindLookAtRotation + M×BallTick(蓝图) | 按需：LineTrace + 1×BatchUpdate + M×BallTick(C++) |
| **可版本控制** | 蓝图 .uasset 二进制，不可 diff | C++ .h/.cpp 文本文件，标准 git diff |
| **VR 扩展** | 需大量重写（输入、射线源、UI交互） | 接口层预留，切换射线源 + 输入映射即可 |

---

## 附录 B：推荐实现顺序

按依赖关系排列，每步完成后可独立验证。

```
第 1 步 ─ 创建 C++ 项目，搭好 Source/ 目录结构
         验证：VS/Rider 编译通过，UE 编辑器可打开

第 2 步 ─ 实现 IInteractableInterface（接口定义）
         验证：编译通过，可被蓝图实现

第 3 步 ─ 实现 UInteractionManager（交互管理器）
         验证：射线检测日志输出正常，UI 注册/注销正常

第 4 步 ─ 实现 BP_FloatingUI_Base + 4 个子类蓝图
         验证：射线悬浮高亮、点击触发、离开恢复

第 5 步 ─ 实现 UCoriolisForceComponent（物理组件）
         验证：发射小球，观察偏转（先用固定参数测试）

第 6 步 ─ 搭建场景 1（不含答题）
         验证：圆盘旋转 → 调转速 → 发射小球 → 观察偏转差异
         
第 7 步 ─ 搭建场景 2（不含答题）
         验证：选择 5 个点位 → 发射小球 → 观察不同纬度偏转差异

第 8 步 ─ 实现 UQuizComponent + DT_QuizData
         验证：ShowQuiz("Scene1") → 正确/错误均有正确反馈

第 9 步 ─ 实现 ULevelTransitionSubsystem（异步关卡切换）
         验证：场景1 答对 → 过渡动画 → 场景2 加载完成

第10步 ─ 搭建场景 3 + 最终联调
         验证：全部三个场景流程跑通，结束界面正常
```
