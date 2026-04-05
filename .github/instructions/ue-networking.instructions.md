---
description: "This project uses Unreal Engine's built-in multiplayer networking with the **Network Prediction Plugin** (NPP) for client prediction and rollback simulation. All networking code must follow **server-authoritative architecture** and Unreal replication best practices. Prediction-sensitive gameplay must use the **Network Prediction framework instead of custom prediction implementations**."
applyTo: "Source/**/*.h, Source/**/*.cpp"
---

# Networking Model

Use Unreal's standard authoritative model.

Server:

* owns authoritative gameplay state
* validates player actions
* replicates state

Clients:

* send input
* simulate prediction
* receive corrections

Never trust client gameplay state.

---

# Network Prediction Plugin

This project uses the **Network Prediction Plugin** for latency-sensitive gameplay.

Prefer Network Prediction for:

* character abilities
* projectiles
* physics-driven movement
* gameplay systems requiring rollback
* competitive gameplay mechanics

Avoid implementing custom prediction systems.

Prediction systems should use:

FNetworkPredictionModelDef
FNetworkPredictionStateTypes
FNetworkPredictionSimulation
FNetworkPredictionProxy

---

# Prediction Architecture

Prediction systems should follow the Network Prediction model:

Input → Simulation → State → Replication → Reconciliation

Simulation must be deterministic.

Simulation logic must not depend on:

* random numbers without seeded RNG
* timers
* non-deterministic physics queries
* asynchronous engine events

---

# Simulation State

Prediction simulations must define state types.

Typical layout:

Sync State
Aux State
Input Command

Example structure pattern:

Sync state → replicated authoritative state
Aux state → static configuration
Input → player commands

Simulation logic should modify **Sync State only**.

---

# Deterministic Simulation

All prediction simulations must be deterministic.

Rules:

* avoid floating point drift where possible
* avoid reading world state during simulation
* avoid accessing external actor state
* avoid gameplay side effects

Simulation must only operate on:

Input state
Sync state
Aux state

---

# Authority Checks

Gameplay state changes must only occur on the server.

Preferred pattern:

if (HasAuthority())
{
// authoritative logic
}

Clients should only run prediction simulations.

---

# Replicated Properties

Use replication for persistent state.

Example:

UPROPERTY(Replicated)
float Health;

Implement:

GetLifetimeReplicatedProps()

Use:

DOREPLIFETIME(ClassName, PropertyName);

Do not replicate prediction simulation state manually unless required by the prediction framework.

---

# RepNotify

Use RepNotify for visual updates.

Example:

UPROPERTY(ReplicatedUsing=OnRep_Health)
float Health;

RepNotify functions should only:

* update UI
* trigger effects
* refresh visuals

Never place gameplay logic in RepNotify.

---

# RPC Usage

RPCs represent gameplay events.

Server RPC → client requests action
Client RPC → server notifies owning client
NetMulticast → cosmetic broadcast

Example:

UFUNCTION(Server, Reliable)
void ServerActivateAbility();

UFUNCTION(Client, Reliable)
void ClientPlayDamageEffect();

UFUNCTION(NetMulticast, Unreliable)
void MulticastExplosionFX();

---

# Server RPC Rules

Server RPCs must validate input.

Never trust client values such as:

* damage values
* inventory state
* cooldowns
* gameplay state

Server determines authoritative results.

---

# NetMulticast Rules

Use NetMulticast only for cosmetic events.

Good examples:

* particle effects
* audio playback
* cosmetic animations

Do not replicate persistent gameplay state via NetMulticast.

---

# Actor Replication

Actors must enable replication.

Example:

bReplicates = true;

Movement replication should rely on built-in systems or network prediction models.

Avoid implementing custom movement replication unless required.

---

# Spawning Actors

Replicated actors must be spawned on the server.

Correct pattern:

if (HasAuthority())
{
GetWorld()->SpawnActor<AProjectile>(ProjectileClass);
}

Clients must never spawn replicated actors.

---

# Player Controllers

PlayerController exists on:

* server
* owning client

PlayerState exists on all clients.

Store replicated player information in PlayerState.

---

# Game Mode vs Game State

GameMode:

* exists only on server
* contains rules and match flow

GameState:

* replicated to clients
* contains match state

Never access GameMode from clients.

---

# Ownership

Actors may have owners.

Example ownership chain:

Pawn → PlayerController

Ownership is used for:

Owner-only RPCs
Owner-only replication

Example condition:

COND_OwnerOnly

---

# Bandwidth Optimization

Avoid replicating:

* frequently changing large data
* arrays with frequent updates
* unnecessary gameplay state

Prefer:

* event-driven RPCs
* compressed data
* prediction systems

---

# Network Prediction Best Practices

When writing predicted gameplay:

Always:

* isolate simulation logic
* keep simulation deterministic
* separate simulation state from actor state
* replicate only minimal authoritative results

Never:

* run world queries during simulation
* trigger gameplay events inside simulation
* spawn actors inside prediction simulation
* modify unrelated actor state

---

# Prediction Reconciliation

Network Prediction will automatically:

* replay inputs
* reconcile prediction errors
* correct client state

Gameplay systems must tolerate corrections.

Avoid storing gameplay state outside simulation state that prediction depends on.

---

# Reliable vs Unreliable RPCs

Reliable RPCs:

* critical gameplay actions
* ability activation
* inventory changes

Unreliable RPCs:

* cosmetic events
* temporary effects
* high frequency updates

---

# Network Debugging

Useful commands:

stat net

np.Debug

np.ShowCorrections

net pktlag

net pktloss

These help debug prediction and networking issues.

---

# Copilot Code Generation Rules

When generating multiplayer code:

ALWAYS:

* maintain server authority
* use Network Prediction for latency-sensitive gameplay
* replicate only necessary state
* validate client RPC input
* spawn replicated actors on the server

NEVER:

* implement custom prediction frameworks
* trust client gameplay state
* replicate prediction simulation state unnecessarily
* run non-deterministic logic inside simulations

---

# Preferred Multiplayer Architecture

Client input
→ Network Prediction simulation
→ Server validation
→ Authoritative state update
→ Replication
→ Client reconciliation

This architecture ensures stable multiplayer gameplay with low latency.

---

# Design Goals

Networking systems should prioritize:

* deterministic simulation
* minimal bandwidth
* predictable correction
* server authority
* scalable multiplayer gameplay

Avoid designs that depend on unsynchronized client state.
