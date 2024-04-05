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

void UPDMissionSubsystem::FinishMission(int32 ActorID, const FPDMissionBase& PersistentDatum)
{
	const FPDMissionRow* DefaultData = Utility.GetDefaultBase(PersistentDatum.mID);
	UPDMissionTracker* Tracker = Utility.GetActorTracker(ActorID);
	const AActor* TrackerOwner = Tracker != nullptr ? Tracker->GetOwner() : nullptr;
	if (DefaultData == nullptr || TrackerOwner  == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Tracker valid: %i, Tracker Owner valid: %i ,  DefaultData valid: %i"), Tracker != nullptr, TrackerOwner != nullptr, DefaultData != nullptr);
		return;
	}

	const TArray<FPDMissionBranchElement>& BranchRef = DefaultData->ProgressRules.NextMissionBranch.Branches;
	const int32 LastIdx = BranchRef.Num() - 1;
	for (int32 Idx = 0; Idx <= LastIdx; Idx++)
	{
		const FPDMissionBranchElement& CurrentBranch = BranchRef[Idx];
		// Pick first branch we match against, skip any up until that point
		if (CurrentBranch.BranchConditions.CallerHasRequiredTags(TrackerOwner) == false)
		{
			continue;
		}

		
		// @todo Delay system? mission timer manager? will think on it a little
		switch (CurrentBranch.TargetBehaviour)
		{
		case ETriggerImmediately:
			break;
		case ETriggerWithDelay:
			break;
		case EUnlockImmediately:
			break;
		case EUnlockWithDelay:
			break;
		default: break;
		}

	}
	
}
