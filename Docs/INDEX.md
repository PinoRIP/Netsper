# Netsper Documentation Index

**Last Updated:** April 3, 2026  
**Status:** All Documents Complete & Validated

---

## 📚 Core Documents

### 1. Game Design Document (GDD)
**File:** `Docs/GDD.md` (822 lines)

**Purpose:** High-level design vision and gameplay requirements  
**Audience:** Designers, producers, all team members  
**Contents:**
- Elevator pitch and core vision
- Design pillars
- Combat system overview
- Movement mechanics (high-level)
- Ability descriptions
- Game modes (Touchdown, Chaser, 1v1 Arena, BetterRoyal)
- Progression and cosmetics

**Key Reference Points:**
- Section 4: Design Pillars (Movement Is Skill)
- Section 4: Core Movement Mechanics (all 11 mechanics listed)
- Section 8: eSper Ability System (5 abilities defined)

---

### 2. Character Implementation Plan
**File:** `Docs/CharacterImplementationPlan.md` (1,392 lines)

**Purpose:** Complete technical specification for character system  
**Audience:** Programmers, technical leads, agents  
**Contents:**
- Project configuration (plugins, dependencies)
- Architecture overview (component map, data flow)
- 8 development phases with detailed pseudocode
- All class/struct definitions
- Networking topology
- 23-phase implementation order

**Key Sections:**
- **Section 3:** Architecture Overview
- **Section 4:** Source File Structure
- **Section 5-12:** 8 Phases (Pawn → Camera)
- **Section 15:** Implementation Order (23 phases)

**Status:** ✅ Updated with NP naming convention

---

### 3. NP Naming Convention Guide
**File:** `Docs/NP_NAMING_CONVENTION.md` (NEW)

**Purpose:** Coding standard and naming rules  
**Audience:** Programmers, code reviewers  
**Contents:**
- Naming convention rules (classes, structs, interfaces)
- File naming and organization
- Code examples (includes, casting, delegates)
- Common patterns
- GameplayTag conventions
- Summary checklist

**Best For:** Ensuring code consistency, standardization

**Key Rule:** `NP` prefix for all project classes (40% shorter than `Netsper`)

## 📑 Quick Find Table

| Question | Answer Location |
|----------|-----------------|
| What should I name my class? | NP_NAMING_CONVENTION.md - "Naming Convention Rules" |
| What's the GDD requirement? | GDD.md - relevant section by mechanic |
| How is the network model? | CharacterImplementationPlan.md - Section 14 |
| Where are the code examples? | NP_NAMING_CONVENTION.md - "Using the NP Prefix in Code" |


## ✅ Validation Checklist

Before starting implementation, verify:

- [x] GDD.md read (design understanding)
- [x] NP_NAMING_CONVENTION.md understood (coding style)

---

## 📝 Notes

- **All documents are in Markdown** for version control and easy reading
- **All code examples provided** in C++ for UE5.7
- **No external dependencies** — all information self-contained
- **Print-friendly format** — suitable for physical documentation

---