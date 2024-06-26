﻿/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
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
	
	/** @brief Sets the value of the replicated datum with the value of the parameter OverrideDatum. Will clamp it based on limits */
	UFUNCTION(BlueprintCallable)
	bool SetMissionDatum(const FGameplayTag& BaseTag, const FPDMissionNetDatum& OverrideDatum);

	/** @brief Called when finalizing a overwrite from FinishMission(), used for immediate transition */
	void FinalizeOverwriteRef(const FGameplayTag& MissionBaseTag, FPDMissionNetDatum& OverwriteDatum, const FPDMissionBranchBehaviour& BranchBehaviour);
	
	/** @brief Called when finalizing a overwrite from FinishMission(), used for delayed transition*/
	void FinalizeOverwriteCopy(FGameplayTag MissionBaseTag, FPDMissionNetDatum OverwriteDatum, FPDMissionBranchBehaviour BranchBehaviour); 
	
	/** @brief Gets the value of the replicated datum  */
	UFUNCTION(BlueprintCallable)
	TEnumAsByte<EPDMissionState> GetStateSelector(const FGameplayTag& BaseTag) const;
	
	/** @brief  Gets the replicated datum from it's SID */
	const FPDMissionNetDatum* GetDatum(int32 SID) const;
	/** @brief  Gets the replicated datum from it's tag */
	const FPDMissionNetDatum* GetDatum(const FGameplayTag& BaseTag) const;
	
	/** @brief  Get all the tracked mission data */
	TArray<FPDMissionNetDatum>& GetUserMissions();
	
	/** @brief Adds and tracks new mission data */
	bool AddMissionDatum(const FPDMissionNetDatum& Mission);

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
	/** @brief Broadcasts an event when a mission updates, runs only on server */
	UPROPERTY(BlueprintAssignable) FPDUpdateMission  Server_OnMissionUpdated; 
};


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
