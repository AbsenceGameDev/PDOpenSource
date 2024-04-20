/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "PDMissionCommon.h"

#include "Components/PDMissionTracker.h"
#include "Interfaces/PDMissionInterface.h"
#include "Subsystems/PDMissionSubsystem.h"

//
// Progress statics

UPDMissionSubsystem* UPDMissionStatics::GetMissionSubsystem()
{
	return GEngine != nullptr ? GEngine->GetEngineSubsystem<UPDMissionSubsystem>() : nullptr;
}

FDataTableRowHandle UPDMissionStatics::CreateRowHandle(UDataTable* Table, FName RowName)
{
	FDataTableRowHandle RowHandle;
	RowHandle.DataTable = Table;
	RowHandle.RowName = RowName;
	return RowHandle;
}

//
// Mission delay functor
FPDDelayMissionFunctor::FPDDelayMissionFunctor(UPDMissionTracker* Tracker, const FDataTableRowHandle& Target, const FPDMissionBranchBehaviour& TargetBehaviour)
{
	bHasRun = false;
	const FPDMissionRow* MissionRow = Target.GetRow<FPDMissionRow>("");
	if (Tracker == nullptr || Tracker->IsValidLowLevelFast() == false || MissionRow == nullptr || Tracker->GetWorld() == nullptr)
	{
		return;
	}
	
	const FGameplayTag& MissionBaseTag = MissionRow->Base.MissionBaseTag;
	const FPDMissionNetDatum* MissionDatum = Tracker->GetDatum(MissionBaseTag);
	if (MissionDatum == nullptr) { return; }

	// Set change immediately
	FPDMissionNetDatum OverwriteDatum = *MissionDatum;
	if (TargetBehaviour.DelayTime <= SMALL_NUMBER)
	{
		Tracker->FinalizeOverwriteRef(MissionBaseTag, OverwriteDatum, TargetBehaviour);
	}
	else
	{
		// Set to pending state
		OverwriteDatum.State.Current = EPDMissionState::EPending;
		Tracker->SetMissionDatum(MissionBaseTag, OverwriteDatum);

		// Dispatch timer
		FTimerHandle SavedHandle;
		FTimerManager& WorldTimer = Tracker->GetWorld()->GetTimerManager();
		const FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Tracker, &UPDMissionTracker::FinalizeOverwriteCopy, MissionBaseTag, OverwriteDatum, TargetBehaviour);
		WorldTimer.SetTimer(SavedHandle, Delegate, TargetBehaviour.DelayTime, false);

		OutHandlesMap.Emplace(SavedHandle, Delegate);
	}


	bHasRun = true;
}

//
// Progress status handler
void FPDMissionStatusHandler::AccumulateData(const FGameplayTag& InTag, FPDFPDMissionModData& DataCompound) const
{
}

void FPDMissionRules::IterateStatusHandlers(const FGameplayTag& Tag, FPDFPDMissionModData& OutStatVariables)
{
}

//
// Mission base helpers
void FPDMissionBase::ResolveMissionTypeTag()
{
	MissionTypeTag = MissionBaseTag.RequestDirectParent();
}

//
// Progress rules - required tags

void FPDMissionTagCompound::AppendUserTags(const TArray<FGameplayTag>& AppendTags)
{
	OptionalUserTags.Append(AppendTags);
}

void FPDMissionTagCompound::RemoveUserTags(const TArray<FGameplayTag>& TagsToRemove)
{
	// Remove in order from last to first to avoid  having to recalculate indices
	for (const FGameplayTag& Tag : TagsToRemove)
	{
		OptionalUserTags.Remove(Tag);
	}
}

void FPDMissionTagCompound::RemoveUserTag(const FGameplayTag TagToRemove)
{
	OptionalUserTags.Remove(TagToRemove);
}

void FPDMissionTagCompound::ClearUserTags(AActor* Caller)
{
	OptionalUserTags.Empty();
}

bool FPDMissionTagCompound::operator==(const FPDMissionTagCompound& Other) const
{
	return  OptionalUserTags.Difference(Other.OptionalUserTags).Num() == 0 && RequiredMissionTags.Difference(Other.RequiredMissionTags).Num() == 0;
}
bool FPDMissionTagCompound::operator==(const FPDMissionTagCompound&& Other) const
{
	return  OptionalUserTags.Difference(Other.OptionalUserTags).Num() == 0 && RequiredMissionTags.Difference(Other.RequiredMissionTags).Num() == 0;
}

bool FPDMissionTagCompound::CallerHasRequiredTags(const AActor* Caller) const
{
	if (Caller == nullptr || Caller->IsValidLowLevelFast() == false || Caller->Implements<UPDMissionInterface>() == false)
	{
		return false;
	}
	
	const IPDMissionInterface* AsInterface = Cast<const IPDMissionInterface>(Caller);
	const TSet<FGameplayTag>& UserTagContainer = AsInterface->GetTagContainer();

	for (const FGameplayTag& Tag : OptionalUserTags)
	{
		if (UserTagContainer.Contains(Tag) == false) { return false; }
	}

	for (const FGameplayTag& Tag : RequiredMissionTags)
	{
		if (UserTagContainer.Contains(Tag) == false) { return false; }
	}

	return true;
}
