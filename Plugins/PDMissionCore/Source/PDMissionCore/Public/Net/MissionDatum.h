// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PDMissionCommon.h"


#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 2 
#include "Net/Serialization/FastArraySerializer.h"
#else 
#include "Engine/NetSerialization.h"
#endif

#include "MissionDatum.generated.h"

struct FPDMissionNetDataCompound;
class UPDMissionTracker;

/**
 *  @brief Replicated datum for missions. This is the minimum amount of data we want to send per packet related to mission tracking.
 *  @note Replicated. Size: 200 bytes, alignment: 8
 *
 *  @todo Generate mIDs from tags at subsystem startup?
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionNetDatum : public FFastArraySerializerItem
{
	GENERATED_BODY()

public:
	FPDMissionNetDatum() = default;
	FPDMissionNetDatum(int32 _SID, FPDMissionState _State) : mID(_SID), State(_State) {};
	
	/** @brief Called by it's serializer when this item has been removed */
	void PreReplicatedRemove(const FPDMissionNetDataCompound& InArraySerializer);

	/** @brief Called by it's serializer when this item has been added */
	void PostReplicatedAdd(const FPDMissionNetDataCompound& InArraySerializer);

	/** @brief Called by it's serializer when this item has been modified */
	void PostReplicatedChange(const FPDMissionNetDataCompound& InArraySerializer);

	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = NetDatum)
	int32 mID = 0x0; /**< @brief Unique SID (StatID). */
	
	/** @brief Current State (value and limits) */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = NetDatum)
	FPDMissionState State = {0x0, FPDMissionTagCompound{}};
	
	/** @brief Current Tick behaviour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FPDMissionTickBehaviour TickSettings;

	friend bool operator==(const FPDMissionNetDatum& A, const FPDMissionNetDatum& B)
	{
		return A.mID == B.mID && A.State.CurrentFlags == B.State.CurrentFlags && A.State.MissionConditionHandler == B.State.MissionConditionHandler;
	}

	friend bool operator!=(const FPDMissionNetDatum& A, const FPDMissionNetDatum& B)
	{
		return (A == B) == false;
	}
};

/**
 *  @brief Fast array serializer for replicated mission data
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionNetDataCompound : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

public:
	/** @brief List of attributes, skills and effects */
	UPROPERTY()
	TArray<FPDMissionNetDatum> Items;

	/** @brief Owning mission tracker. Responsible for replicating changes to 'Items' */
	UPROPERTY()
	UPDMissionTracker* OwnerTracker;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);
};


/**
 * @brief Typetraits for FFastArraySerializers. TStructOpsTypeTraits Needed for structs using NetSerialize, such as fastarray types
 */
template<>
struct TStructOpsTypeTraits<FPDMissionNetDataCompound> : public TStructOpsTypeTraitsBase2<FPDMissionNetDataCompound>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};