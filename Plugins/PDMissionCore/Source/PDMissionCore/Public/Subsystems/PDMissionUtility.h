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

#include "PDMissionCommon.h"

#include "CoreMinimal.h"
#include <Engine/NetDriver.h>
#include <Engine/DataTable.h>

#include "PDMissionUtility.generated.h"

class UPDMissionTracker;
struct FPDMissionNetDatum;

USTRUCT(BlueprintType, Blueprintable)
struct PDMISSIONCORE_API FPDMissionUtility final
{
	GENERATED_BODY()

public:
// UTILITY	
	int32 ResolveMIDViaTag(const FGameplayTag& BaseTag) const;
	
	/** @brief Gets the tracker associated with the 'ActorID' */
	UPDMissionTracker* GetActorTracker(int32 ActorID) const;
	
	/** @brief Gets active mission data for mission with 'mID' on the calling actor associated with the 'ActorID' */
	FPDMissionNetDatum* GetMissionDatum(int32 ActorID, int32 SID) const;
	
	/** @brief Gets the mission conditions */
	const FPDMissionRules* GetMissionRules(const FGameplayTag& BaseTag) const;
	
	/** @brief Get the mission default data associated with param 'mID' */
	FPDMissionRow* GetDefaultBase(const int32 mID) const;
	
	/** @brief Get the mission rules associated with param 'mID' */
	FPDMissionRules* GetMissionRules(const int32 mID) const;  

	/** @brief Get the mission metadata associated with param 'mID' */
	const FPDMissionMetadata& GetMetadataBase(const int32 mID) const;  

	/** @brief Checks if param 'mID' is associated with valid mission or not. @return true if valid | false if not*/
	bool IsValidMission(const int32 mID) const;   
	
	/** @brief Get the mission default data associated with param 'BaseTag' */
	FPDMissionRow* GetDefaultBaseViaTag(const FGameplayTag& BaseTag) const;   

	/** @brief Get the mission rules associated with param 'BaseTag' */
	FPDMissionRules* GetMissionRulesViaTag(const FGameplayTag& BaseTag) const;  

	/** @brief Get the mission metadata associated with param 'BaseTag' */
	const FPDMissionMetadata& GetMetadataBaseViaTag(const FGameplayTag& BaseTag ) const; 

	/** @brief Checks if param 'BaseTag' is associated with valid mission or not. @return true if valid | false if not*/
	bool IsValidMissionViaTag(const FGameplayTag& BaseTag) const;   
	
	/** @brief Get the level percentage */ 
	float CurrentMissionPercentage(const FGameplayTag& BaseTag, int32 ActorID) const;

	/** @brief Sets a new mission datum on the calling tracker  */ 
	void SetNewMissionDatum(UPDMissionTracker* MissionTracker, int32 SID, const FPDMissionNetDatum& Datum) const;

	/** @brief Overwrite a mission datum on the calling tracker  */ 
	void OverwriteMissionDatum(UPDMissionTracker* MissionTracker, int32 SID, const FPDMissionNetDatum& NewDatum, bool ForceDefault = false) const;
	
// SETUP
	/** @brief Called on creation to setup data */
	void InitializeMissionSubsystem();                           
	
	/** @brief Increments a replicated actorID */
	static int32 RequestNewActorID(int32& LatestCreatedActorID); 
	
	/** @brief Registers users tracker events */
	void RegisterUser(UPDMissionTracker* Tracker);               
	
	/** @brief Deregisters users tracker events */
	void DeRegisterUser(const UPDMissionTracker* Tracker);       
	
	/** @brief Reads and fills the lookup maps for the missions */
	void ProcessTablesForFastLookup();                           
	
	/** @brief Only call after ProcessTablesForFastLookup, as it will generate empty settings for each mapped mID */
	void InitializeTracker(const int32 ActorID);                 
	
	/** @brief Set a assigned mission event */
	void BindMissionEvent(int32 ActorID, int32 mID, const FPDUpdateMission& MissionEventDelegate);

	/** @brief Execute an assigned mission event */
	bool ExecuteBoundMissionEvent(const int32 ActorID, const int32 mID, const EPDMissionState NewState);

public:
	/** @brief Map associating ActorIDs with their mission tracker  */
	UPROPERTY()
	TMap<int32, UPDMissionTracker*> MissionTrackerMap;
	
	/** @brief Fast lookups of mission row-handles, keyed by their mID  */
	TMap<int32, FDataTableRowHandle> MissionLookup {};
	
	/** @brief Fast lookups. Associating Gameplay tags with mIDs */
	TMap<FGameplayTag, int32> MissionTagToMIDLookup {};

	/**< @brief Fast lookups. Associating rownames with rowhandles */
	TMap<FName, FDataTableRowHandle> MissionLookupViaRowName {};
	
protected:
	/** @brief Datatable of row-type FPDMissionRow */
	UPROPERTY(EditAnywhere, Category = "Mission Subsystem", Meta = (RequiredAssetDataTags="RowStructure=/Script/PDMissionCore.PDMissionRow"))
	TArray<UDataTable*> MissionTables {};
	
	/** @brief  Nested map of Mission events. TMap<ActorID, TMap<mID, Event Signature>> */
	TMap<int32, FMissionTreeMap> BoundMissionEvents {};
	
private:
	FPDMissionMetadata DummyMetadata = {FText::GetEmpty(), FText::GetEmpty()};
	friend class UPDMissionSubsystem;
};