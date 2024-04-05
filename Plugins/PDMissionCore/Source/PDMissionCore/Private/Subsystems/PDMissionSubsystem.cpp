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


#include "Subsystems/PDMissionSubsystem.h"

#include "Components/PDMissionTracker.h"

void UPDMissionSubsystem::SetMission(int32 ActorID, const FPDMissionBase& PersistentDatum)
{
}

bool UPDMissionSubsystem::FinishMission(int32 ActorID, const FPDMissionBase& PersistentDatum)
{
	const FPDMissionRow* DefaultData = Utility.GetDefaultBase(PersistentDatum.mID);
	UPDMissionTracker* Tracker = Utility.GetActorTracker(ActorID);
	const AActor* TrackerOwner = Tracker != nullptr ? Tracker->GetOwner() : nullptr;
	if (DefaultData == nullptr || TrackerOwner  == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Tracker valid: %i, Tracker Owner valid: %i ,  DefaultData valid: %i"),
			Tracker != nullptr, TrackerOwner != nullptr, DefaultData != nullptr);
		return false;
	}
	

	// can't set mission progress, does not have required tags too finish the mission 
	if (DefaultData->ProgressRules.MissionConditionHandler.CallerHasRequiredTags(TrackerOwner) == false)
	{
		return false;
	}

	switch (Tracker->GetDatum(PersistentDatum.mID)->State.Current)
	{
	case ECompleted:
	case EFailed:
		// @todo Complete/Fail path needs to check if it is repeatable and if yes, how long should it be delayed
		return false;
	case EPending:
		// Avoid retriggering while pending
		return false;
	case ELocked:
		// @todo Locked path needs to check if conditions for immediate or delayed 'unlocking + completion' have been met  
		return false;
	case EInactive:
		// @todo Inactive path needs to check if conditions for immediate or delayed 'unlocking + completion' have been met		return false;
		return false;
	case EActive:
		break;
	default: ;
	}

	const TArray<FPDMissionBranchElement>& BranchRef = DefaultData->ProgressRules.NextMissionBranch.Branches;
	const bool MissionHasBranches = BranchRef.IsEmpty() == false;
	
	FPDDelayMissionFunctor NewMissionDispatch;
	const int32 LastIdx = BranchRef.Num() - 1;
	TArray<FTimerHandle> OutHandles{};
	OutHandles.SetNum(BranchRef.Num());
	TArray<FTimerHandle, FTimerDelegate> OutHandlesMap{};
	
	for (int32 Idx = 0; Idx <= LastIdx; Idx++)
	{
		const FPDMissionBranchElement& CurrentBranch = BranchRef[Idx];
		
		// Pick first branch we match against, skip any up until that point
		if (CurrentBranch.BranchConditions.CallerHasRequiredTags(TrackerOwner) == false)
		{
			continue;
		}
		
		// @todo Delay system? mission timer manager? will think on it a little
		NewMissionDispatch = FPDDelayMissionFunctor{Tracker, CurrentBranch.Target, CurrentBranch.TargetBehaviour, OutHandles[Idx]};
		break; // exit loop after constructing the functor
	}

	// If no event was fired at all, output something to the log to notify any mission designers that they need to fix their mission-design
	// and fix their mission rules as the current settings has gotten it soft-locked
	if (NewMissionDispatch.bHasRun == false && MissionHasBranches)
	{
		UE_LOG(LogTemp, Error, TEXT("CRITICAL ERROR; SOFTLOCK. NO BRANCHING PATH MET CONDITIONS TO BRANCH."));
		return false;
	}

	for (const FTimerHandle& TimerHandle : OutHandles)
	{
		if (TimerHandle.IsValid() == false) { continue; }

		// Timer is valid,
		// @todo Start: 1. Store the handles so we can later make
		// @todo Cont.: 2  Delay for given time
		// @todo Cont.: 3. When exiting game, before destroying the UWorld,
		// @todo Cont.: 4. Check which timers have remaining time left by using the stored handles,
		// @todo Cont.: 5. Store their remaining time alongside with timer delegate parameters and delegate in a format that we'll serialize 
	}
	

	
	return true; // Either successfully passed to another branch or no branch left and was last mission in the current branching path 
}
