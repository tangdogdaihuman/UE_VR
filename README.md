# UE_VR

UE 5.6 VR 第一人称交互式教育项目，用于演示**科里奥利力（地转偏向力）**的物理原理。

## 功能

### 教育演示（三个渐进场景）
1. **旋转圆盘** — 调节转速，发射小球观察偏转
2. **虚拟地球** — 5个纬度标记（90°N~90°S），验证 sin(纬度) 偏转规律
3. **现实案例** — 台风（逆时针旋转）与河流（右岸侵蚀）动画

物理公式：`F_c = -2m(ω × v) × sin(latitude) × BaseForceCoefficient`

### 游戏模式
- **Horror** — 体力冲刺 + 手电筒
- **Shooter** — 武器系统、HP/死亡、AI（StateTree/EQS）、团队计分

## 技术架构

- **交互系统** — `IInteractableInterface` 接口驱动，射线检测 + 点击派发
- **子系统注册** — `InteractionManager`（GameInstance）、`LevelTransitionSubsystem`（关卡过渡）、`VRAssetGenerator`（Editor自动生成蓝图）
- **输入系统** — Enhanced Input，运行时创建 InputAction，无需 .uasset
- **测验系统** — `UQuizComponent` 支持硬编码 TMap 或 DataTable
- **UI优化** — 批量朝向更新，N个UI只需1个定时器

## 操作

| 按键 | 普通模式 | 测验模式 |
|-----|---------|---------|
| 1 | 生成小球 | 答案A |
| 2 | 加速圆盘 | 答案B |
| 3 | 减速圆盘 | 答案C |
| 鼠标点击 | 射线交互 | — |

## 构建

```powershell
# 编译（Development Editor, Win64）
UnrealBuildTool.exe VREditor Win64 Development -Project="VR.uproject" -WaitMutex

# 命令行验证
UnrealEditor-Cmd.exe "VR.uproject" -unattended -nopause -nullrhi -log

# 打开编辑器
UnrealEditor.exe "VR.uproject"
```

## 项目结构

```
VR/Source/VR/
├── Core/              — IInteractableInterface
├── Interaction/       — UInteractionManager
├── Physics/           — UCoriolisForceComponent
├── UI/                — UQuizComponent
├── Scene/             — Scene1DiskManager / Scene2EarthManager / Scene3NatureManager
├── Level/             — ULevelTransitionSubsystem
├── VRCharacter        — 基础第一人称角色
├── Variant_Horror/    — 恐怖模式
└── Variant_Shooter/   — 射击模式
```

## 文档

- `docs/requirements_v2.md` — v2.0 架构与API
- `docs/需求文档_优化版.md` — 中文架构摘要
- `ONBOARDING.md` — 新成员上手指南
