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

#include "Subsystems/PDMissionUtility.h"
#include "Components/PDMissionTracker.h"
#include "Net/MissionDatum.h"

#include <Curves/CurveFloat.h>

#ifndef MGETTRACKER_EXITNONAUTH 
#define MGETTRACKER_EXITNONAUTH(ID, BLOCK) \
GetActorTracker(ID); \
	if (MissionTracker == nullptr || MissionTracker->GetOwnerRole() != ROLE_Authority) \
	{ \
		BLOCK; \
	}
#endif
/* USER INTERFACE */

const FPDMissionMetadata& FPDMissionUtility::GetMetadataBase(const int32 mID) const
{
	FPDMissionRow* StatRow = GetDefaultBase(mID);
	return StatRow != nullptr ? StatRow->Metadata : DummyMetadata;
}

const FPDMissionMetadata& FPDMissionUtility::GetMetadataBaseViaTag(const FGameplayTag& BaseTag) const
{
	FPDMissionRow* MissionRow = GetDefaultBaseViaTag(BaseTag);
	return MissionRow != nullptr ? MissionRow->Metadata : DummyMetadata;
}

bool FPDMissionUtility::IsValidMission(const int32 SID) const
{
	return MissionLookup.Contains(SID);
}

bool FPDMissionUtility::IsValidMissionViaTag(const FGameplayTag& BaseTag) const
{
	return MissionTagToMIDLookup.Contains(BaseTag);
}

int32 FPDMissionUtility::ResolveMIDViaTag(const FGameplayTag& BaseTag) const
{
	return MissionTagToMIDLookup.Contains(BaseTag) ? *MissionTagToMIDLookup.Find(BaseTag) : INDEX_NONE;
}

FPDMissionRow* FPDMissionUtility::GetDefaultBase(const int32 SID) const
{
	const FDataTableRowHandle* RowHandle = MissionLookup.Find(SID);
	if (RowHandle == nullptr) { return nullptr; }

	return RowHandle->GetRow<FPDMissionRow>(FString::Printf(TEXT("GetDefaultBase() - MID: %i"), SID));
}

FPDMissionRow* FPDMissionUtility::GetDefaultBaseViaTag(const FGameplayTag& BaseTag) const
{
	const int32* MID = MissionTagToMIDLookup.Find(BaseTag);
	const FDataTableRowHandle* RowHandle = MissionLookup.Find(MID != nullptr ? *MID : INDEX_NONE);
	
	if (RowHandle == nullptr) { return nullptr; }

	return RowHandle->GetRow<FPDMissionRow>(FString::Printf(TEXT("GetDefaultBase() - MID: %i"), *MID));
}

FPDMissionRules* FPDMissionUtility::GetMissionRules(const int32 SID) const
{
	FPDMissionRow* MissionRow = GetDefaultBase(SID);
	return MissionRow != nullptr ? &MissionRow->ProgressRules : nullptr;
}

FPDMissionRules* FPDMissionUtility::GetMissionRulesViaTag(const FGameplayTag& BaseTag) const
{
	FPDMissionRow* MissionRow = GetDefaultBaseViaTag(BaseTag);
	return MissionRow != nullptr ? &MissionRow->ProgressRules : nullptr;
}

UPDMissionTracker* FPDMissionUtility::GetActorTracker(int32 ActorID) const
{
	UPDMissionTracker* const* MissionMap = MissionTrackerMap.Find(ActorID);
	if (MissionMap == nullptr || (*MissionMap) == nullptr || (*MissionMap)->IsValidLowLevel() == false)
	{
		return nullptr;
	}
	return *MissionMap;
}

float FPDMissionUtility::CurrentMissionPercentage(const FGameplayTag& BaseTag, int32 ActorID) const
{
	// @todo A simple check of how many flags out of how many total for the mission have been checked. 
	// const FPDMissionRow* DefaultBase = GetDefaultBaseViaTag(BaseTag);
	// if (DefaultBase == nullptr || DefaultBase->ProgressRules.SomeField == nullptr) { return 0.0f; }

	return -1.0f;	
}

const FPDMissionRules* FPDMissionUtility::GetMissionRules(const FGameplayTag& BaseTag) const
{
	const FPDMissionRow* Base = GetDefaultBaseViaTag(BaseTag);
	return Base != nullptr ? &Base->ProgressRules : nullptr;
}

FPDMissionNetDatum* FPDMissionUtility::GetMissionDatum(int32 ActorID, int32 SID) const
{
	const UPDMissionTracker* MissionTracker = GetActorTracker(ActorID);
	return MissionTracker != nullptr ? const_cast<FPDMissionNetDatum*>(MissionTracker->GetDatum(SID)) : nullptr;
}

void FPDMissionUtility::SetNewMissionDatum(UPDMissionTracker* MissionTracker, int32 SID, const FPDMissionNetDatum& Datum) const
{
	MissionTracker->SetMissionValue(GetDefaultBase(SID)->Base.MissionBaseTag, Datum);
}

void FPDMissionUtility::OverwriteMissionDatum(UPDMissionTracker* MissionTracker, int32 SID, const FPDMissionNetDatum& NewDatum, bool ForceDefault) const
{
	const FPDMissionRow* DefaultMissionBaseDatum = GetDefaultBase(SID);
	
	ForceDefault = ForceDefault && DefaultMissionBaseDatum != nullptr;
	FPDMissionNetDatum Datum;
		Datum.mID = SID;
		Datum.State.CurrentFlags = ForceDefault ? DefaultMissionBaseDatum->Base.MissionFlags : NewDatum.State.CurrentFlags;
		Datum.State.MissionConditionHandler = ForceDefault ? DefaultMissionBaseDatum->ProgressRules.MissionConditionHandler : NewDatum.State.MissionConditionHandler;
	
	MissionTracker->SetMissionValue(GetDefaultBase(SID)->Base.MissionBaseTag, Datum);
}



//
// SETUP

void FPDMissionUtility::InitializeMissionSubsystem()
{
	ProcessTablesForFastLookup();
}

int32 FPDMissionUtility::RequestNewActorID(int32& LatestCreatedActorID)
{
	return ++LatestCreatedActorID;
}

void FPDMissionUtility::RegisterUser(UPDMissionTracker* Tracker)
{
	const int32 ActorID = Tracker->GetActorID();

	MissionTrackerMap.Add(ActorID, Tracker);
	if (BoundMissionEvents.Find(ActorID) == nullptr)
	{
		BoundMissionEvents.Add(ActorID, {}); // {Event-List} = const FMissionTreeMap,
	}

	InitializeTracker(ActorID); // @todo Load from storage instead of a clean Init, if user data is available 
}

void FPDMissionUtility::DeRegisterUser(const UPDMissionTracker* Tracker)
{
	const int32 ActorID = Tracker->GetActorID();
	if (BoundMissionEvents.Find(ActorID) != nullptr) { BoundMissionEvents.Remove(ActorID); }

	UE_LOG(LogLevel, Log, TEXT("FPDMissionUtility::DeRegisterUser (%i)"), ActorID);
}

void FPDMissionUtility::ProcessTablesForFastLookup()
{
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	int32  SuccessCounter  = 0;
	FString fSuccessCounter = "Succeeded";
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	
	for (UDataTable* StatDataTable : MissionTables)
	{
		const TMap<FName, uint8*>& AllItems = StatDataTable->GetRowMap();
		for (TMap<FName, uint8*>::TConstIterator RowMapIter(AllItems.CreateConstIterator()); RowMapIter; ++RowMapIter)
		{
			const FPDMissionRow* TableRow = reinterpret_cast<FPDMissionRow*>(RowMapIter.Value());
			if (TableRow == nullptr) { continue; }
			
			FDataTableRowHandle RowHandle = UPDMissionStatics::CreateRowHandle(StatDataTable, RowMapIter.Key());
			MissionLookup.Add(TableRow->Base.mID, RowHandle);
			MissionTagToMIDLookup.Add(TableRow->Base.MissionBaseTag, TableRow->Base.mID);
			MissionLookupViaRowName.Add(RowHandle.RowName, RowHandle);

			
#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
			SuccessCounter++;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT

		}
	}
	
	// @todo Cycle through the tables a second time to populate some lookups based on mission rules
	for (UDataTable* MissionTable : MissionTables)
	{
		const TMap<FName, uint8*>& AllItems = MissionTable->GetRowMap();

		for (TMap<FName, uint8*>::TConstIterator RowMapIter(AllItems.CreateConstIterator()); RowMapIter; ++RowMapIter)
		{
			FPDMissionRow* TableRow = reinterpret_cast<FPDMissionRow*>(RowMapIter.Value());
			if (TableRow == nullptr) { continue; }
			
			FPDMissionRules& MissionRules = TableRow->ProgressRules;
			TArray<FGameplayTag> StatTagList;

		}
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	fSuccessCounter = FString{SuccessCounter != 0 ? "Succeeded" : "Failed"} + FString{" at creating stat lookup maps \n"};
	SuccessCounter = 0;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void FPDMissionUtility::InitializeTracker(const int32 ActorID)
{
	UPDMissionTracker* MissionTracker = MGETTRACKER_EXITNONAUTH(ActorID, return);

	for (TTuple<int32, FDataTableRowHandle>& BaseStat : MissionLookup)
	{
		const FPDMissionRow* DefaulMission = BaseStat.Value.GetRow<FPDMissionRow>(TEXT(""));
		if (DefaulMission == nullptr) { continue; }
		
		FPDMissionNetDatum Mission{DefaulMission->Base.mID, FPDMissionState{DefaulMission->Base.MissionFlags, DefaulMission->ProgressRules.MissionConditionHandler}};
		MissionTracker->AddMissionDatum(Mission);
	}
}

void FPDMissionUtility::BindMissionEvent(int32 ActorID, int32 mID, const FPDUpdateMission& MissionEventDelegate)
{
	if (mID == INDEX_NONE) { return; }

	FMissionTreeMap* UserStatEvents = BoundMissionEvents.Find(ActorID);
	if (UserStatEvents) { UserStatEvents->Emplace(mID, MissionEventDelegate); }
}

bool FPDMissionUtility::ExecuteBoundMissionEvent(const int32 ActorID, const int32 mID, const int32 NewFlags)
{
	// Do we have and event map for this user/actor?
	FMissionTreeMap* UserEventMapPtr = BoundMissionEvents.Find(ActorID);
	if (UserEventMapPtr == nullptr) { return false; }

	// Find if there is any delegate bound to this specific SID, fire if true
	const FPDUpdateMission* TempDelegate = UserEventMapPtr->Find(mID);
	if (TempDelegate == nullptr) { return false; }

	TempDelegate->Broadcast(mID, NewFlags);
	return true;
}
