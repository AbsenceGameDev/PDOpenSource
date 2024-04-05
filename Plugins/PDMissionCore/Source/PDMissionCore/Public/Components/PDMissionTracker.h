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
#include "Net/MissionDatum.h"
#include "PDMissionTracker.generated.h"


/**
 * @brief Tracks public and private progress
 */
UCLASS(BlueprintType)
class PDMISSIONCORE_API UPDMissionTracker : public UActorComponent
{
	GENERATED_BODY()
public:
	FORCEINLINE int32 GetActorID() const { return ActorID; }
	
	/*
	 *	Set a stat value with exp, value and level support
	 */
	/** @brief Sets the value of the replicated datum with the value of the parameter OverrideDatum. Will clamp it based on limits */
	UFUNCTION(BlueprintCallable) void  SetMissionValue(const FGameplayTag& BaseTag, const FPDMissionNetDatum& OverrideDatum);
	/** @brief Gets the value of the replicated datum  */
	UFUNCTION(BlueprintCallable) int32 GetValue(const FGameplayTag& BaseTag) const;
	
	/** @brief  Gets the replicated datum from it's SID */
	const FPDMissionNetDatum* GetDatum(int32 SID) const;
	/** @brief  Gets the replicated datum from it's tag */
	const FPDMissionNetDatum* GetDatum(const FGameplayTag& BaseTag) const;
	
	/** @brief  Get all the tracked mission data */
	TArray<FPDMissionNetDatum>& GetUserMissions();
	
	/** @brief Adds and tracks new mission data */
	void AddMissionDatum(const FPDMissionNetDatum& Mission);

	/** @brief  Function that resolves to dispatching the OnUpdated delegate if possible*/
	void OnDatumUpdated(const FPDMissionNetDatum* CallingStat) const;
public:
	
	/**<@brief List of tags of stats to be shared with all clients */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FGameplayTag> PublicMissionTags;     
	/**<@brief List of tags of stats to be shared with specific groups of clients */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FGameplayTag> ProtectedMissionTags;  
	/**<@brief List of tags of stats to only be shared with the owning client */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FGameplayTag> PrivateMissionTags;    
	/**<@brief List of tags of stats that only exists on the server */
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FGameplayTag> HiddenMissionTags;     
	
	/** @brief  Actual replicated data, shared with all clients */
	UPROPERTY(Replicated) FPDMissionNetDataCompound State;                         
	/** @brief  Actual replicated data, shared with specific groups of clients */
	UPROPERTY(Replicated) FPDMissionNetDataCompound ProtectedMissionsState;             
	/** @brief  Actual replicated data, shared with only the owning client	 */
	UPROPERTY(Replicated) FPDMissionNetDataCompound PrivateMissionsState;               
	/** @brief  Non-replicated data, exists only on the server */
	UPROPERTY()           FPDMissionNetDataCompound HiddenMissionState;                

	/** @brief Generated ID of owning actor */
	int32 ActorID = INDEX_NONE;                    
	/** @brief Map to associate an SID to its replication id in the fast-array */
	UPROPERTY() TMap<int32, int32> mIDToReplIdMap; 

	// Delegate bindings
	/** @brief Broadcasts an event any time a mission updates */
	UPROPERTY(BlueprintAssignable) FPDUpdateMission  OnMissionUpdated;       
	/** @brief Broadcasts an event when a mission ticks */
	UPROPERTY(BlueprintAssignable) FPDTickMission    OnMissionTick;

	UPROPERTY(BlueprintAssignable) FPDUpdateMission  Server_OnMissionUpdated; 

};

