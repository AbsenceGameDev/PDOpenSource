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
#include "Components/PDMissionTracker.h"
#include "Subsystems/PDMissionSubsystem.h"
#include "Net/MissionDatum.h"

#include <Engine/NetDriver.h>
#include <Net/UnrealNetwork.h>


void UPDMissionTracker::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	// @todo Rewrite to allow public and private as-well as protected versions of the 'State' property that does not get replicated to everyone, with of them only ever staying on the server
	
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.Condition    = COND_None;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(UPDMissionTracker, State, SharedParams);
}

void UPDMissionTracker::SetMissionValue(const FGameplayTag& BaseTag, const FPDMissionNetDatum& OverrideDatum)
{
	if (GetOwnerRole() != ROLE_Authority) { return; }

	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return; }

	const int32 mID = MissionSubsystem->Utility.ResolveMIDViaTag(BaseTag);
	const FPDMissionRow* DefaultData = MissionSubsystem->Utility.GetDefaultBase(mID);
	if (DefaultData == nullptr) { return; }
	
	FPDMissionNetDatum ClampedDatum = OverrideDatum;
	FPDMissionState& NewState = ClampedDatum.State;

	// Check limits and clamp if needed
	if (NewState.MissionConditionHandler.CallerHasRequiredTags(GetOwner()) == false)
	{
		return; // can't set mission progress, does not have required tags 
	}
	
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDMissionTracker, State, this);
	int32 ArrayIndex = mIDToReplIdMap.Contains(mID) ? *mIDToReplIdMap.Find(mID) -1 : INDEX_NONE;
	if (ArrayIndex != INDEX_NONE && State.Items.IsValidIndex(ArrayIndex) == false)
	{
		mIDToReplIdMap.Remove(mID); // clear
		ArrayIndex = INDEX_NONE;
	}
	
	if (ArrayIndex != INDEX_NONE)
	{
		State.Items[ArrayIndex].State.CurrentFlags = NewState.CurrentFlags;
		State.Items[ArrayIndex].State.MissionConditionHandler = NewState.MissionConditionHandler;
		State.MarkItemDirty(State.Items[ArrayIndex]);
	}
	else
	{
		FPDMissionNetDatum& AddedDatum = State.Items.Add_GetRef(ClampedDatum);
		State.MarkItemDirty(AddedDatum);
		mIDToReplIdMap.Add(mID, AddedDatum.ReplicationID);
	}

	Server_OnMissionUpdated.Broadcast(DefaultData->Base.mID, NewState.CurrentFlags);
}


TArray<FPDMissionNetDatum>& UPDMissionTracker::GetUserMissions()
{
	// If any clients		
	// LastAccumulatedStatCompounds.Append(State.Items);

	// // If allowed clients @todo set up some way to mark permission groups
	// LastAccumulatedStatCompounds.Append(ProtectedProgress.Items);
	//
	// // If owning clients @todo need support for 'ProtectedProgress' & 'PrivateProgress'
	// if (GetOwnerRole() == ENetRole::ROLE_Authority)
	// {
	// 	LastAccumulatedStatCompounds.Append(PrivateProgress.Items);
	// }
		
	return State.Items;
}

void UPDMissionTracker::AddMissionDatum(const FPDMissionNetDatum& Mission)
{
	int32 ArrayIndex = mIDToReplIdMap.Contains(Mission.mID) ? *mIDToReplIdMap.Find(Mission.mID) -1 : INDEX_NONE;
	if (ArrayIndex != INDEX_NONE && State.Items.IsValidIndex(ArrayIndex) == false)
	{
		mIDToReplIdMap.Remove(Mission.mID); // clear
		ArrayIndex = INDEX_NONE;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UPDMissionTracker, State, this);
	if (ArrayIndex != INDEX_NONE)
	{
		State.Items[ArrayIndex].State = Mission.State;
		State.MarkItemDirty(State.Items[ArrayIndex]);
		return;
	}
	
	FPDMissionNetDatum& AddedDatum = State.Items.Add_GetRef(Mission);
	State.MarkItemDirty(AddedDatum);
	mIDToReplIdMap.Add(Mission.mID, AddedDatum.ReplicationID);
}

const FPDMissionNetDatum* UPDMissionTracker::GetDatum(int32 SID) const
{
	const int32* ReplicationID = mIDToReplIdMap.Find(SID);
	if (ReplicationID == nullptr) { return nullptr; }
	
	const int32 ArrayIndex = *ReplicationID - 1;
	if (State.Items.IsValidIndex(ArrayIndex) == false) { return nullptr; }
	
	return &State.Items[ArrayIndex];
}

const FPDMissionNetDatum* UPDMissionTracker::GetDatum(const FGameplayTag& BaseTag) const
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return nullptr; }
	
	const FPDMissionRow* Data = MissionSubsystem->Utility.GetDefaultBaseViaTag(BaseTag);
	if (Data == nullptr) { return nullptr; }
	
	const int32 ArrayIndex = mIDToReplIdMap.Contains(Data->Base.mID) ? *mIDToReplIdMap.Find(Data->Base.mID) - 1 : INDEX_NONE;
	if (State.Items.IsValidIndex(ArrayIndex) == false) { return nullptr; }

	return &State.Items[ArrayIndex];
}

int32 UPDMissionTracker::GetValue(const FGameplayTag& BaseTag) const
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return INDEX_NONE; }
	
	const FPDMissionRow* Data = MissionSubsystem->Utility.GetDefaultBaseViaTag(BaseTag);
	if (Data == nullptr) { return INDEX_NONE; }
	
	const int32 ArrayIndex = mIDToReplIdMap.Contains(Data->Base.mID) ? *mIDToReplIdMap.Find(Data->Base.mID) - 1 : INDEX_NONE;
	if (State.Items.IsValidIndex(ArrayIndex) == false) { return INDEX_NONE; }

	return State.Items[ArrayIndex].State.CurrentFlags;
}

void UPDMissionTracker::OnDatumUpdated(const FPDMissionNetDatum* UpdatedMissionDatum) const
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (UpdatedMissionDatum == nullptr || MissionSubsystem == nullptr) { return; }
	
	const FPDMissionRow* DefaultStat = MissionSubsystem->Utility.GetDefaultBase(UpdatedMissionDatum->mID);
	if (DefaultStat == nullptr) { return; }
	
	OnMissionUpdated.Broadcast(DefaultStat->Base.mID, UpdatedMissionDatum->State.CurrentFlags);
}

