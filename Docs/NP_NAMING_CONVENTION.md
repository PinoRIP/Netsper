# NP Naming Convention Guide

## Quick Reference

All Netsper classes use the **`NP`** prefix instead of the full project name.

### Types

```cpp

UMyClass -> UNPMyClass
IMyInterface -> INPMyInterface
FMyStruct -> FNPMyStruct
EMyEnum -> ENPMyEnum

```

### GameplayTags

Tags don't use the NP prefix (they use the feature name):

```cpp
// ✅ Correct
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("Movement.Ground.Sprint"));
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("Ability.Grapple"));

// ❌ Wrong
FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName("NP.Movement.Ground.Sprint"));
```

---

## Summary Checklist

- [ ] All new classes use `NP` prefix (U/A/I prefix + NP + feature name)
- [ ] File names match class names (NPClassName.h / NPClassName.cpp)
- [ ] Struct names use `FNP` prefix
- [ ] Interface names use `INP` prefix
- [ ] GameplayTags use feature names (no NP prefix)
- [ ] Includes reference correct NP paths
- [ ] Forward declarations use full NP names
- [ ] Comments refer to classes by their NP names

---
