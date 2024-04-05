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

bool UPDMissionTracker::SetMissionDatum(const FGameplayTag& BaseTag, const FPDMissionNetDatum& OverrideDatum)
{
	if (GetOwnerRole() != ROLE_Authority) { return false; }

	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return false; }

	const int32 mID = MissionSubsystem->Utility.ResolveMIDViaTag(BaseTag);
	const FPDMissionRow* DefaultData = MissionSubsystem->Utility.GetDefaultBase(mID);
	if (DefaultData == nullptr) { return false; }
	
	MARK_PROPERTY_DIRTY_FROM_NAME(UPDMissionTracker, State, this);
	int32 ArrayIndex = mIDToReplIdMap.Contains(mID) ? *mIDToReplIdMap.Find(mID) -1 : INDEX_NONE;
	if (ArrayIndex != INDEX_NONE && State.Items.IsValidIndex(ArrayIndex) == false)
	{
		mIDToReplIdMap.Remove(mID); // clear
		ArrayIndex = INDEX_NONE;
	}
	
	const FPDMissionState& NewState = OverrideDatum.State;
	if (ArrayIndex != INDEX_NONE)
	{
		State.Items[ArrayIndex].State.Current = NewState.Current;
		State.Items[ArrayIndex].State.MissionConditionHandler = NewState.MissionConditionHandler;
		State.MarkItemDirty(State.Items[ArrayIndex]);
	}
	else
	{
		FPDMissionNetDatum& AddedDatum = State.Items.Add_GetRef(OverrideDatum);
		State.MarkItemDirty(AddedDatum);
		mIDToReplIdMap.Add(mID, AddedDatum.ReplicationID);
	}

	Server_OnMissionUpdated.Broadcast(DefaultData->Base.mID, NewState.Current);

	return true;
}

void UPDMissionTracker::FinalizeOverwriteRef(const FGameplayTag& MissionBaseTag, FPDMissionNetDatum& OverwriteDatum, const FPDMissionBranchBehaviour& BranchBehaviour)
{
	switch (BranchBehaviour.Type)
	{
	default: // Resolve default as trigger
	case ETrigger:
		// Go from locked/inactive to active
		OverwriteDatum.State.Current = EPDMissionState::EActive;
		break;
	case EUnlock:
		// Go from locked to inactive
			OverwriteDatum.State.Current = EPDMissionState::EInactive;
		break;
	}
	SetMissionDatum(MissionBaseTag, OverwriteDatum);	
}

void UPDMissionTracker::FinalizeOverwriteCopy(FGameplayTag MissionBaseTag, FPDMissionNetDatum OverwriteDatum, FPDMissionBranchBehaviour BranchBehaviour)
{
	FinalizeOverwriteRef(MissionBaseTag, OverwriteDatum, BranchBehaviour);
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

bool UPDMissionTracker::AddMissionDatum(const FPDMissionNetDatum& Mission)
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
		return true;
	}
	
	FPDMissionNetDatum& AddedDatum = State.Items.Add_GetRef(Mission);
	State.MarkItemDirty(AddedDatum);
	mIDToReplIdMap.Add(Mission.mID, AddedDatum.ReplicationID);

	return true;
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

TEnumAsByte<EPDMissionState> UPDMissionTracker::GetStateSelector(const FGameplayTag& BaseTag) const
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return EPDMissionState::EINVALID_STATE; }
	
	const FPDMissionRow* Data = MissionSubsystem->Utility.GetDefaultBaseViaTag(BaseTag);
	if (Data == nullptr) { return EPDMissionState::EINVALID_STATE; }
	
	const int32 ArrayIndex = mIDToReplIdMap.Contains(Data->Base.mID) ? *mIDToReplIdMap.Find(Data->Base.mID) - 1 : INDEX_NONE;
	if (State.Items.IsValidIndex(ArrayIndex) == false) { return EPDMissionState::EINVALID_STATE; }

	return State.Items[ArrayIndex].State.Current;
}

void UPDMissionTracker::OnDatumUpdated(const FPDMissionNetDatum* UpdatedMissionDatum) const
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (UpdatedMissionDatum == nullptr || MissionSubsystem == nullptr) { return; }
	
	const FPDMissionRow* DefaultDatum = MissionSubsystem->Utility.GetDefaultBase(UpdatedMissionDatum->mID);
	if (DefaultDatum == nullptr) { return; }
	
	OnMissionUpdated.Broadcast(DefaultDatum->Base.mID, UpdatedMissionDatum->State.Current);
}

