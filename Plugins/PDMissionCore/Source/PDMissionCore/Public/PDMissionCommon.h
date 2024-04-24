/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include <Curves/CurveFloat.h>
#include <Engine/DataTable.h>

#include "PDMissionCommon.generated.h"

/* Forward declarations */
class UPDMissionTracker;

UENUM()
enum EPDMissionBranchBehaviour
{
	ETrigger, // Apply and enable the mission, either with a delay or right away
	EUnlock,  // Unlock the mission, either with a delay or right away, but do not apply nor enable the mission. 
};

UENUM()
enum EPDMissionState
{
	ECompleted,      // Finished successfully
	EFailed,         // Finished unsuccessfully  
	EActive,         // Still active  
	EInactive,       // User has not triggered/enabled the mission  
	ELocked,         // User has not unlocked the mission  
	EPending,        // Mission is in a pending state, i.e. transitioning from one state to another with a delay  
	EINVALID_STATE,  // Mission is in a pending state, i.e. transitioning from one state to another with a delay  
};

/* Delegates */
/** @brief Called when a mission updated, used it's mID */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDUpdateMission, int32, mID, EPDMissionState, vNewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDTickMission, int32, mID, FPDUpdateMission, UpdateFunction);
typedef TMap<int32, FPDUpdateMission> FPDMissionTreeMap;


/**
 *	@brief Static functions exposed to blueprint
 */
UCLASS(BlueprintType)
class PDMISSIONCORE_API UPDMissionStatics : public UObject
{
	GENERATED_BODY()

public:
	/** @brief Retrieves a subsystem of given type */
	UFUNCTION(BlueprintCallable)
	static class UPDMissionSubsystem* GetMissionSubsystem();

	/** @brief Creates a row-handle structure */
	UFUNCTION(BlueprintCallable)
	static FDataTableRowHandle CreateRowHandle(class UDataTable* Table, FName RowName);
	
private:	
};


/**
 * @brief Simple structure that holds a friendly name along with a descriptor
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionMetadata
{
	GENERATED_BODY()

	/**< @brief */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Metadata")
	FText Name {};

	/**< @brief */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Metadata")
	FText Descriptor {};
};

/**
 * @brief Structure that holds the tick behaviour settings of a given mission type. This includes the delta-value, the interval between tick of the mission, 
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionTickBehaviour
{
	GENERATED_BODY()
	
	/** @brief Amount to re-generate every 're-generation tick'. 0 disables this behaviour */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Tick") 
	int32 DeltaValue = 0;

	/** @brief Number of seconds between each tick interval. 0 to disable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Tick") 
	float Interval = 1.0f;

	/** @brief When set the ticker will not tick it's internals */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Tick") 
	bool  bIsPaused = false;
};

/**
 * @brief Structure that holds the tick behaviour settings of a given mission type. This includes the delta-value, the interval between each 'tick' 
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionTagCompound
{
	GENERATED_BODY()
	FPDMissionTagCompound(TArray<FGameplayTag> _OptionalUserTags = {}) : OptionalUserTags(_OptionalUserTags) {}
	FPDMissionTagCompound(const FPDMissionTagCompound& Other) : OptionalUserTags(Other.OptionalUserTags), RequiredMissionTags(Other.RequiredMissionTags) {}

	bool CallerHasRequiredTags(const AActor* Caller) const;

	void AppendUserTags(const TArray<FGameplayTag>& AppendTags);
	void RemoveUserTags(const TArray<FGameplayTag>& TagsToRemove);
	void RemoveUserTag(FGameplayTag TagToRemove);
	void ClearUserTags(AActor* Caller);

	bool operator==(const FPDMissionTagCompound& Other) const;
	bool operator==(const FPDMissionTagCompound&& Other) const;

	/** @brief Optional tags that need to exist on the actor requesting this mission for it to be approved, Set to not being editable so they are greyed out from the datatable editor */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Mission|Datum")
	TSet<FGameplayTag> OptionalUserTags{};

private:
	/** @brief Missing-tags that need to exist on the actor requesting this mission for it to be approved */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Datum", Meta = (AllowPrivateAccess="true"))
	TSet<FGameplayTag> RequiredMissionTags{};	
};



/**
 * @brief Structure that holds the state information of an active MissionNetDatum. This includes the current value and the current limits of the stat for the said owning mission tracker.
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionState
{
	GENERATED_BODY()
	FPDMissionState(EPDMissionState _CurrrentState = EPDMissionState::EInactive, TArray<FGameplayTag> _RequiredMissionTags = {}) : Current(_CurrrentState), MissionConditionHandler(_RequiredMissionTags) {};
	FPDMissionState(EPDMissionState _CurrrentState, const FPDMissionTagCompound&_OtherHandler) : Current(_CurrrentState), MissionConditionHandler(_OtherHandler) {};
	
	/** @brief Current mission state */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	TEnumAsByte<EPDMissionState> Current;
	
	/** @brief Tags that need to exist on the actor requesting this mission for it to be approved */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	FPDMissionTagCompound MissionConditionHandler{};
	
};

/**
 * @brief Structure that holds modifiers for the mission
 *  @todo write initial implementation
 */
USTRUCT(BlueprintType)
struct FPDFPDMissionModData
{
	GENERATED_USTRUCT_BODY()

public:
	/** @brief Modifer values for the data values */
	
};

/**
 *  @brief Mission Status (effect) handler
 *  @todo write initial implementation
 */
USTRUCT(BlueprintType)
struct FPDMissionStatusHandler
{
	GENERATED_BODY()
public:
	/** @brief Targets DataCompound and accumulates a value based on the selected ModifierType */
	void AccumulateData(const FGameplayTag& InTag, FPDFPDMissionModData& DataCompound) const;
	
};

/**
 * @brief Tells us how we want to treat the branch, as trigger or unlock, as delayed or immediate
 */
USTRUCT(Blueprintable)
struct FPDMissionBranchBehaviour
{
	GENERATED_BODY()
	
	/** @brief Trigger or unlock? */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem")
	TEnumAsByte<EPDMissionBranchBehaviour> Type = EPDMissionBranchBehaviour::ETrigger;

	/** @brief Delay or immediate? */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem")	
	float DelayTime = 0.0f;
};

/**
 * @brief Structure that holds the state information of an active MissionNetDatum. This includes the current value and the current limits of the stat for the said owning mission tracker.
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDDelayMissionFunctor
{
	GENERATED_BODY()

	FPDDelayMissionFunctor() : bHasRun(false) {};
	FPDDelayMissionFunctor(uint8 _bHasRun) : bHasRun(_bHasRun) {};
	FPDDelayMissionFunctor(UPDMissionTracker* Tracker, const FDataTableRowHandle& Target, const FPDMissionBranchBehaviour& TargetBehaviour);

	UPROPERTY()
	uint8 bHasRun : 1;
	
	TMap<FTimerHandle, FTimerDelegate> OutHandlesMap{};
};


/**
 * @brief 
 */
USTRUCT(Blueprintable)
struct FPDMissionBranchElement
{
	GENERATED_BODY()

	/** @brief  'Target' is the mission we might branch to */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem", Meta = (RowType="/Script/PDMissionCore.PDMissionRow"))
	FDataTableRowHandle Target;

	/** @brief Actual condition to be able to branch to the target */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	FPDMissionTagCompound BranchConditions;

	/** @brief true means it's a direct branch, i.e. 'same questline', false means it's a new questline */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem")
	uint8 bIsDirectBranch : 1;

	/** @brief Tells us how we want to treat the branch */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem")
	FPDMissionBranchBehaviour TargetBehaviour;
};

/**
 * @brief 
 */
USTRUCT(Blueprintable)
struct FPDMissionBranch
{
	GENERATED_BODY()

	/** @brief Order means priority, idx0 has highest order. So we branch to the highest priority missions if possible and ignore the rest */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem")
	TArray<FPDMissionBranchElement> Branches;
};


/**
 *  @brief Structure that defines mission rules.
 *  @done Write some form of type (FPDMissionTagCompound) that handles comparing the the mission tags with the user tags
 *  @todo Write some type that can handle  
 */
USTRUCT(BlueprintType)
struct FPDMissionRules
{
	GENERATED_BODY()

public:
	FPDMissionRules(): bRepeatable(0) {};
	FPDMissionRules(FPDMissionTagCompound _MissionConditionHandler, FPDMissionBranch _NextMissionBranch, uint8 bRepeatable)
		: MissionConditionHandler(_MissionConditionHandler), NextMissionBranch(_NextMissionBranch) ,bRepeatable(0) {};
	
	void IterateStatusHandlers(const FGameplayTag& Tag, FPDFPDMissionModData& OutStatVariables);

	/** @brief Flags that need to exist on the actor requesting this mission for it to be approved */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Rules")
	FPDMissionTagCompound MissionConditionHandler{};

	/** @brief Branching conditions for this mission  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Rules")
	FPDMissionBranch NextMissionBranch;

	/** @brief If we get the mission again after finishing it, are we allowed to retrigger it? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Rules")
	TEnumAsByte<EPDMissionState> EStartState = EPDMissionState::EInactive; 
	
	/** @brief If we get the mission again after finishing it, are we allowed to retrigger it? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Rules")
	uint8 bRepeatable : 1; 
};

/**
 *  @brief Base data, such as ID & Tag, Mission flags and mission type/category
 */
USTRUCT(BlueprintType, Blueprintable)
struct PDMISSIONCORE_API FPDMissionBase
{
	GENERATED_BODY()

	FPDMissionBase(const FGameplayTag& _MissionBaseTag = FGameplayTag::EmptyTag, int32 _mID = INDEX_NONE, int32 _Flags = 0x0)
		: MissionBaseTag(_MissionBaseTag), mID(_mID), MissionTypeTag(_MissionBaseTag.RequestDirectParent()) {}
	
	void ResolveMissionTypeTag();
	const FGameplayTag& GetMissionTypeTag() { return MissionTypeTag; }

	/** @brief mission tag, expected format 'Mission.<DirectParent>.<MissionBaseTag>' DirectParent should be the mission or type */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Data|Base")
	FGameplayTag MissionBaseTag{};
	
	/** @brief mission ID (mID), generated from the mission tag */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Data|Base")
	int32 mID = 0x0;

private:	
	/** @brief Type/Category tag. Is the direct parent tag of the MissionBaseTag */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mission|Data|Base", Meta = (AllowPrivateAccess="true"))
	FGameplayTag MissionTypeTag{};
};

/**
 *  @brief Mission row in the mission table, allows to set starting flags, progress rules
 */
USTRUCT(BlueprintType, Blueprintable)
struct PDMISSIONCORE_API FPDMissionRow : public FTableRowBase
{
	GENERATED_BODY()
	
	/** @brief mission tag */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Data")
	FPDMissionBase Base{};
	
	/** @brief tick settings for this mission */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Data")
	FPDMissionTickBehaviour TickSettings{};
	
	/** @brief Mission rules, what are the conditions to finish the mission, what are it's sub-objectives, what is the branching possibilities, etc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Data")
	FPDMissionRules ProgressRules{};

	/** @brief Metadata, Friendly Name & Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission|Data")
	FPDMissionMetadata Metadata {};
};


//
// Legal according to ISO due to explicit template instantiation bypassing access rules

// Creates a static data member of type Tag::TType in which to store the address of a private member.
template <class Tag>
class TPrivateAccessor
{
public:
	static typename Tag::PrivateType TypeValue;
}; 
template <class Tag> 
typename Tag::PrivateType TPrivateAccessor<Tag>::TypeValue;

// Create a static data member whose constructor initializes TPrivateAccessor<Tag>::TypeValue with the private member we want to access
template <class TTag, typename TTag::PrivateType PrivateType>
class TTagPrivateMember
{
public:
	TTagPrivateMember() { TPrivateAccessor<TTag>::TypeValue = PrivateType; }
	static TTagPrivateMember ExplicitInstance;
};
template <class TTag, typename TTag::PrivateType PrivateType> 
TTagPrivateMember<TTag,PrivateType> TTagPrivateMember<TTag,PrivateType>::ExplicitInstance;

template <typename TTypeMember>
class TAccessorTypeHandler { public: typedef TTypeMember* PrivateType; };

// Usage 
// Each distinct private member to access should have its own tag.
// Each tag should contain a nested ::PrivateType that is the corresponding pointer-to-member type.

struct FPDExamplePrivateMemberHolder
{
private:
	static inline int8 _CatchMeIfYouCan_Int8 = 1;
	static inline int64 _CatchMeIfYouCan_Int64 = 2;
	static inline float _CatchMeIfYouCan_Float = 3;
	static inline double _CatchMeIfYouCan_Double = 4;
	static inline FPDMissionRow _CatchMeIfYouCan_FPDMissionRow{};
	
};

//  Tag private member for safe access
template class TTagPrivateMember<TAccessorTypeHandler<int8> , &FPDExamplePrivateMemberHolder::_CatchMeIfYouCan_Int8>;
template class TTagPrivateMember<TAccessorTypeHandler<int64> , &FPDExamplePrivateMemberHolder::_CatchMeIfYouCan_Int64>;
template class TTagPrivateMember<TAccessorTypeHandler<float> , &FPDExamplePrivateMemberHolder::_CatchMeIfYouCan_Float>;
template class TTagPrivateMember<TAccessorTypeHandler<double> , &FPDExamplePrivateMemberHolder::_CatchMeIfYouCan_Double>;
template class TTagPrivateMember<TAccessorTypeHandler<FPDMissionRow> , &FPDExamplePrivateMemberHolder::_CatchMeIfYouCan_FPDMissionRow>;
inline void ExampleFunction()
{
	int8* PrivateInt8Ptr = TPrivateAccessor<TAccessorTypeHandler<int8>>::TypeValue;
	int64* PrivateInt64Ptr = TPrivateAccessor<TAccessorTypeHandler<int64>>::TypeValue;
	float* PrivateFloatPtr = TPrivateAccessor<TAccessorTypeHandler<float>>::TypeValue;
	double* PrivateDoublePtr = TPrivateAccessor<TAccessorTypeHandler<double>>::TypeValue;
	FPDMissionRow* PrivateMissionRowPtr = TPrivateAccessor<TAccessorTypeHandler<FPDMissionRow>>::TypeValue;
}

/**
Business Source License 1.1

Parameters

Licensor:             Ario Amin (@ Permafrost Development)
Licensed Work:        PDOpenSource (Source available on github)
                      The Licensed Work is (c) 2024 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make commercial use of the Licensed Work provided these three additional conditions as met; 
                      	1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      	2. The Licensed Work must be Compiled before being redistributed.
                      	3. The Licensed Work Source may not be packaged into the product or service being sold

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.
                      
                      "Source" form means the source code (.h & .cpp files) contained in the different modules in PDOpenSource.
                      This will usually be written in human-readable format.

                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of the files or codes therein..  

Change Date:          2028-04-17

Change License:       Apache License, Version 2.0

For information about alternative licensing arrangements for the Software,
please visit: N/A

Notice

The Business Source License (this document, or the “License”) is not an Open Source license.
However, the Licensed Work will eventually be made available under an Open Source License, as stated in this License.

License text copyright (c) 2017 MariaDB Corporation Ab, All Rights Reserved.
“Business Source License” is a trademark of MariaDB Corporation Ab.

-----------------------------------------------------------------------------

Business Source License 1.1

Terms

The Licensor hereby grants you the right to copy, modify, create derivative works, redistribute, and make non-production use of the Licensed Work.
The Licensor may make an Additional Use Grant, above, permitting limited production use.

Effective on the Change Date, or the fourth anniversary of the first publicly available distribution of a specific version of the Licensed Work under this License,
whichever comes first, the Licensor hereby grants you rights under the terms of the Change License, and the rights granted in the paragraph above terminate.

If your use of the Licensed Work does not comply with the requirements currently in effect as described in this License, you must purchase a
commercial license from the Licensor, its affiliated entities, or authorized resellers, or you must refrain from using the Licensed Work.

All copies of the original and modified Licensed Work, and derivative works of the Licensed Work, are subject to this License. This License applies
separately for each version of the Licensed Work and the Change Date may vary for each version of the Licensed Work released by Licensor.

You must conspicuously display this License on each original or modified copy of the Licensed Work. If you receive the Licensed Work
in original or modified form from a third party, the terms and conditions set forth in this License apply to your use of that work.

Any use of the Licensed Work in violation of this License will automatically terminate your rights under this License for the current
and all other versions of the Licensed Work.

This License does not grant you any right in any trademark or logo of Licensor or its affiliates (provided that you may use a
trademark or logo of Licensor as expressly required by this License).

TO THE EXTENT PERMITTED BY APPLICABLE LAW, THE LICENSED WORK IS PROVIDED ON AN “AS IS” BASIS. LICENSOR HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
EXPRESS OR IMPLIED, INCLUDING (WITHOUT LIMITATION) WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND TITLE.

MariaDB hereby grants you permission to use this License’s text to license your works, and to refer to it using the trademark
“Business Source License”, as long as you comply with the Covenants of Licensor below.

Covenants of Licensor

In consideration of the right to use this License’s text and the “Business Source License” name and trademark,
Licensor covenants to MariaDB, and to all other recipients of the licensed work to be provided by Licensor:

1. To specify as the Change License the GPL Version 2.0 or any later version, or a license that is compatible with GPL Version 2.0
   or a later version, where “compatible” means that software provided under the Change License can be included in a program with
   software provided under GPL Version 2.0 or a later version. Licensor may specify additional Change Licenses without limitation.

2. To either: (a) specify an additional grant of rights to use that does not impose any additional restriction on the right granted in
   this License, as the Additional Use Grant; or (b) insert the text “None”.

3. To specify a Change Date.

4. Not to modify this License in any other way.
 **/