---
description: "Use when editing Build.cs module definitions, Target.cs target files, adding module dependencies, configuring PCH usage, or troubleshooting build/link errors. Covers UBT module setup, plugin dependencies, API macros, and module architecture."
applyTo: "**/*.Build.cs, **/*.Target.cs"
---

# Unreal Build System — Build.cs & Target.cs

## Module Definition (Build.cs)

### Current Module: Netsper

```csharp
using UnrealBuildTool;

public class Netsper : ModuleRules
{
	public Netsper(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
```

### Required Dependencies for This Project

When implementing features, add the correct module dependency BEFORE using its types:

| Feature | Module Dependency | Visibility |
|---------|------------------|------------|
| Mover 2.0 | `"Mover"` | Public |
| Network Prediction Plugin | `"NetworkPrediction"` | Public |
| Enhanced Input | `"EnhancedInput"` | Public (already added) |
| Gameplay Tags | `"GameplayTags"` | Public |
| Niagara VFX | `"Niagara"` | Private |
| UMG/Slate UI | `"UMG"`, `"Slate"`, `"SlateCore"` | Private |
| Animation | `"AnimGraphRuntime"` | Private |
| Physics | `"PhysicsCore"`, `"Chaos"` | Private |
| AI (Behavior Trees) | `"AIModule"` | Private |
| Navigation | `"NavigationSystem"` | Private |
| Online/Sessions | `"OnlineSubsystem"`, `"OnlineSubsystemUtils"` | Private |
| Net Core | `"NetCore"` | Public |

### Public vs Private Dependencies

- **Public**: Types appear in your module's public headers. Any module that depends on yours also needs these.
- **Private**: Types only used in `.cpp` files or private headers. Don't leak to dependents.

```csharp
// Public — exposed in headers, consumers inherit the dependency
PublicDependencyModuleNames.AddRange(new string[] {
	"Core",
	"CoreUObject",
	"Engine",
	"InputCore",
	"EnhancedInput",
	"Mover",
	"NetworkPrediction",
	"GameplayTags"
});

// Private — used only in .cpp, not exposed
PrivateDependencyModuleNames.AddRange(new string[] {
	"Niagara",
	"UMG",
	"Slate",
	"SlateCore",
	"AnimGraphRuntime",
	"NetCore"
});
```

### PCH Configuration

```csharp
// Explicit PCH — recommended, each file includes what it needs
PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

// For faster compilation of large modules, create a shared PCH:
SharedPCHHeaderFile = "Netsper.h";
```

### Module API Macro

The `NETSPER_API` macro is required on all classes that may be accessed from other modules or by UHT:

```cpp
class NETSPER_API ANPCharacterPawn : public APawn { ... };
class NETSPER_API UNPStaminaComponent : public UActorComponent { ... };
struct NETSPER_API FNPMoverInputCmd { ... };
```

If the project ever splits into multiple modules, the API macro ensures proper DLL export/import.

---

## Target Files (Target.cs)

### Game Target

```csharp
using UnrealBuildTool;

public class NetsperTarget : TargetRules
{
	public NetsperTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Netsper");
	}
}
```

### Editor Target

```csharp
using UnrealBuildTool;

public class NetsperEditorTarget : TargetRules
{
	public NetsperEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Netsper");
	}
}
```

---

## Plugin Dependencies

Plugins must be enabled in the `.uproject` file AND their modules added to Build.cs:

```json
// In Netsper.uproject — "Plugins" array
{
	"Name": "Mover",
	"Enabled": true
},
{
	"Name": "NetworkPrediction",
	"Enabled": true
},
{
	"Name": "EnhancedInput",
	"Enabled": true
},
{
	"Name": "GameplayTagsEditor",
	"Enabled": true,
	"TargetAllowList": ["Editor"]
}
```

---

## Module Initialization

```cpp
// Netsper.h
#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogNP, Log, All);

// Netsper.cpp
#include "Netsper.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogNP);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Netsper, "Netsper");
```

---

## Common Build Errors and Fixes

| Error | Cause | Fix |
|-------|-------|-----|
| `unresolved external symbol` | Missing module dependency | Add module to Build.cs dependencies |
| `cannot open include file` | Missing module or wrong include path | Add dependency AND verify include path |
| `XXX_API not defined` | Missing API macro on class | Add `NETSPER_API` to class declaration |
| `generated.h not found` | UHT hasn't run or include order wrong | Ensure `.generated.h` is last include |
| `GENERATED_BODY() missing` | Forgot macro in UCLASS/USTRUCT | Add `GENERATED_BODY()` inside class/struct |
| `Plugin not found` | Plugin not in .uproject | Add to `Plugins` array in .uproject |

# Building / Compiling

& "D:\Epic\Unreal\UE_5.7\Engine\Build\BatchFiles\Build.bat"  NetsperEditor Win64 Development "D:\ProjectRepos\Netsper\Netsper.uproject"