/*
 * @copyright Permafrost Development (MIT license) 
 * Authors: Ario Amin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include <Curves/CurveFloat.h>
#include <Engine/DataTable.h>

#include "PDMissionCommon.generated.h"

/* Delegates */
/** @brief Called when a mission updated, used it's mID */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDUpdateMission, int32, mID, uint8, bNewFlag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPDTickMission, int32, mID, FPDUpdateMission, UpdateFunction);
typedef TMap<int32, FPDUpdateMission> FMissionTreeMap;

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
 * @brief Structure that holds the tick behaviour settings of a given mission type. This includes the delta-value, the interval between tick of the stat how long the 
 */
USTRUCT(BlueprintType)
struct PDMISSIONCORE_API FPDMissionTagCompound
{
	GENERATED_BODY()
	FPDMissionTagCompound(TArray<FGameplayTag> _OptionalUserTags = {}) : OptionalUserTags(_OptionalUserTags) {}
	FPDMissionTagCompound(const FPDMissionTagCompound& Other) : OptionalUserTags(Other.OptionalUserTags), RequiredMissionTags(Other.RequiredMissionTags) {}

	bool CallerHasRequiredTags(AActor* Caller);

	const void AppendUserTags(TArray<FGameplayTag> AppendTags);
	const void RemoveUserTags(TArray<FGameplayTag> TagsToRemove);
	const void RemoveUserTag(FGameplayTag TagToRemove);
	const void ClearUserTags(AActor* Caller);

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
	FPDMissionState(int32 _Flags = 0x0, TArray<FGameplayTag> _RequiredMissionTags = {}) : CurrentFlags(_Flags), MissionConditionHandler(_RequiredMissionTags) {};
	FPDMissionState(int32 _Flags, const FPDMissionTagCompound&_OtherHandler) : CurrentFlags(_Flags), MissionConditionHandler(_OtherHandler) {};
	
	/** @brief Current mission flags */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	int32 CurrentFlags = 0x0;
	
	/** @brief Flags that need to exist on the actor requesting this mission for it to be approved */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	FPDMissionTagCompound MissionConditionHandler{};
};


/**
 * @brief Structure that holds modifiers for the mission
 *  @todo write initial implementation
 */
USTRUCT(BlueprintType)
struct FPDFMissionModData
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
	void AccumulateData(const FGameplayTag& InTag, FPDFMissionModData& DataCompound) const;
	
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
	void IterateStatusHandlers(const FGameplayTag& Tag, FPDFMissionModData& OutStatVariables);

	/** @brief Flags that need to exist on the actor requesting this mission for it to be approved */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Mission|Datum")
	FPDMissionTagCompound MissionConditionHandler{};
};

/**
 *  @brief Base data, such as ID & Tag, Mission flags and mission type/category
 */
USTRUCT(BlueprintType, Blueprintable)
struct PDMISSIONCORE_API FPDMissionBase
{
	GENERATED_BODY()

	FPDMissionBase(const FGameplayTag& _MissionBaseTag = FGameplayTag::EmptyTag, int32 _mID = INDEX_NONE, int32 _Flags = 0x0)
		: MissionBaseTag(_MissionBaseTag), mID(_mID), MissionFlags(_Flags), MissionTypeTag(_MissionBaseTag.RequestDirectParent()) {}
	
	void ResolveMissionTypeTag();

	/** @brief mission tag, expected format 'Mission.<DirectParent>.<MissionBaseTag>' DirectParent should be the mission or type */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FGameplayTag MissionBaseTag{};
	
	/** @brief mission ID (mID), generated from the mission tag */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = StatData)
	int32 mID = 0x0;
	
	/** @brief Value representation, holds a start value, value limits and value divisor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	int32 MissionFlags{};

private:	
	/** @brief Type/Category tag. Is the direct parent tag of the MissionBaseTag */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = StatData, Meta = (AllowPrivateAccess="true"))
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FPDMissionBase Base{};
	
	/** @brief Minimum/Maximum values for this stat */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FPDMissionTickBehaviour TickSettings{};
	
	/** @brief Mission rules, what happens when leveling up, what stats are modified, etc */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FPDMissionRules ProgressRules{};
	
	/** @brief Metadata, Friendly Name & Description */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = StatData)
	FPDMissionMetadata Metadata {};
};
