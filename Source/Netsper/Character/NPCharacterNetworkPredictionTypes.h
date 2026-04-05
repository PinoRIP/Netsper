// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MoverSimulationTypes.h"
#include "NetworkPredictionStateTypes.h"
#include "UObject/Object.h"
#include "NPCharacterNetworkPredictionTypes.generated.h"

struct FNetSerializeParams;

USTRUCT()
struct NETSPER_API FNPCharacterNetworkPredictionInput
{
	GENERATED_BODY()

	UPROPERTY()
	FMoverInputCmdContext Mover;

	void NetSerialize(const FNetSerializeParams& P) { Mover.NetSerialize(P); }
	void ToString(FAnsiStringBuilderBase& Out) const { Mover.ToString(Out); }
};


USTRUCT()
struct NETSPER_API FNPCharacterNetworkPredictionSyncState
{
	GENERATED_BODY()

	UPROPERTY()
	FMoverSyncState Mover;

	void NetSerialize(const FNetSerializeParams& P) { Mover.NetSerialize(P); }
	void ToString(FAnsiStringBuilderBase& Out) const { Mover.ToString(Out); }
	bool ShouldReconcile(const FNPCharacterNetworkPredictionSyncState& Authority) const { return Mover.ShouldReconcile(Authority.Mover); }
	void Interpolate(const FNPCharacterNetworkPredictionSyncState* From, const FNPCharacterNetworkPredictionSyncState* To, float Pct) { Mover.Interpolate(&From->Mover, &To->Mover, Pct); }
};


USTRUCT()
struct NETSPER_API FNPCharacterNetworkPredictionAuxState
{
	GENERATED_BODY()

	UPROPERTY()
	FMoverAuxStateContext Mover;

	void NetSerialize(const FNetSerializeParams& P) { Mover.NetSerialize(P); }
	void ToString(FAnsiStringBuilderBase& Out) const { Mover.ToString(Out); }
	bool ShouldReconcile(const FNPCharacterNetworkPredictionAuxState& Authority) const { return Mover.ShouldReconcile(Authority.Mover); }
	void Interpolate(const FNPCharacterNetworkPredictionAuxState* From, const FNPCharacterNetworkPredictionAuxState* To, float Pct) { Mover.Interpolate(&From->Mover, &To->Mover, Pct); }
};


using NPCharacterNetworkPredictionTypes = TNetworkPredictionStateTypes<FNPCharacterNetworkPredictionInput, FNPCharacterNetworkPredictionSyncState, FNPCharacterNetworkPredictionAuxState>;