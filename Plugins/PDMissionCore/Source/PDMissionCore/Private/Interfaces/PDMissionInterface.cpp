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

#include "Interfaces/PDMissionInterface.h"												 
#include "Components/PDMissionTracker.h"
#include "Subsystems/PDMissionSubsystem.h"

#ifdef MGETTRACKER_EXITNONAUTH
#undef MGETTRACKER_EXITNONAUTH
#endif

#ifndef MGETTRACKER_EXITNONAUTH
#define MGETTRACKER_EXITNONAUTH(CallingActor, Tracker, BLOCK) \
CallingActor->FindComponentByClass<UPDMissionTracker>(); \
\
	switch (CallingActor->GetLocalRole()) \
	{ \
	case ROLE_MAX: \
	case ROLE_None: \
	case ROLE_Authority: \
		break; \
	case ROLE_SimulatedProxy: \
	case ROLE_AutonomousProxy: \
		BLOCK; \
	} \
if (Tracker == nullptr) { BLOCK; }

#endif

struct PDMISSIONCORE_API FPDPrivateMissionHandler 
{
public:
	static void _GrantMissionToActor(const AActor* CallingActor, FName MissionName);
	static void _RemoveMissionFromActor(const AActor* CallingActor, FName MissionName);
	static void _AddTagsToContainer(TArray<FGameplayTag> NewTags, TSet<FGameplayTag>& ExistingTags);	
	static void _RemoveTagsToContainer(TArray<FGameplayTag> DeleteNewTags, TSet<FGameplayTag>& ExistingTags);

	
protected:
};

void IPDMissionInterface::GrantMissionToActor_Implementation(AActor* CallingActor, FName MissionName)
{
	FPDPrivateMissionHandler::_GrantMissionToActor(CallingActor, MissionName);
}

void IPDMissionInterface::RemoveMissionFromActor_Implementation(AActor* CallingActor, FName MissionName)
{
	FPDPrivateMissionHandler::_RemoveMissionFromActor(CallingActor, MissionName);
}

void IPDMissionInterface::AddTagsToContainer_Implementation(TArray<FGameplayTag>& Tags)
{
	FPDPrivateMissionHandler::_AddTagsToContainer(Tags, TagContainer);
}

void IPDMissionInterface::RemoveTagsToContainer_Implementation(TArray<FGameplayTag>& DeleteTags)
{
	FPDPrivateMissionHandler::_RemoveTagsToContainer(DeleteTags, TagContainer);
}

void FPDPrivateMissionHandler::_GrantMissionToActor(const AActor* CallingActor, FName MissionName)  
{
	UPDMissionTracker* MissionTracker = MGETTRACKER_EXITNONAUTH(CallingActor, MissionTracker, return);
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return; }

	const int32 ActorID = MissionTracker->GetActorID();

	const FString BuildString = FString::Printf(TEXT("GrantSkillToPlayer -- ActorID: %i"), ActorID);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *BuildString);
	
	//
	// Grant the skills, if they are not already granted
	const FGameplayTag MissionTag = FGameplayTag::RequestGameplayTag(MissionName, false);

	//
	// @note Find skill by row-name if called from console, allows to skipping to having to input the full tag hierarchy. @note conflicting names are not handled
	const FPDMissionRow* FoundMissionData = nullptr; 
	if (MissionSubsystem->Utility.MissionLookupViaRowName.Contains(MissionName))
	{
		const FDataTableRowHandle* FoundHandle = MissionSubsystem->Utility.MissionLookupViaRowName.Find(MissionName);
		FoundMissionData = FoundHandle->GetRow<FPDMissionRow>("");
	}

	int32 mID = INDEX_NONE; 
	if (MissionTag != FGameplayTag())     { mID = MissionSubsystem->Utility.ResolveMIDViaTag(MissionTag); }
	else if (FoundMissionData != nullptr) { mID = FoundMissionData->Base.mID; }
	
	mID = mID == INDEX_NONE && FoundMissionData != nullptr ? FoundMissionData->Base.mID : INDEX_NONE;
	if (mID == INDEX_NONE) // Are we still INDEX_NONE? Invalid skill 
	{
		UE_LOG(LogTemp, Warning, TEXT("%s,  Found no mission by the name of '%s'"), *BuildString, *MissionName.ToString());
		return;
	}

	const FPDMissionNetDatum* ExistingDatum = MissionTracker->GetDatum(mID);
	if (ExistingDatum == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s, Enabling mission by the ID of '%i' and by name of '%s'"), *BuildString , mID, *MissionName.ToString());
		const FPDMissionBase PersistentDatum{MissionTag, mID, 0b0000};
		MissionSubsystem->SetMission(ActorID, PersistentDatum);
		return;
	}

	FPDMissionNetDatum OverwriteDatum = *ExistingDatum;
	if (OverwriteDatum.State.CurrentFlags == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s, Enabling mission by the ID of '%i' and by name of '%s'"), *BuildString , mID, *MissionName.ToString());
		OverwriteDatum.State.CurrentFlags = 0b000;
		MissionSubsystem->Utility.OverwriteMissionDatum(MissionTracker, mID, OverwriteDatum);
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s, Mission(%s) was already enabled.'"), *BuildString, *MissionName.ToString());
}

void FPDPrivateMissionHandler::_RemoveMissionFromActor(const AActor* CallingActor, FName MissionName)
{
	const UPDMissionTracker* MissionTracker = MGETTRACKER_EXITNONAUTH(CallingActor, MissionTracker, return);
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return; }

	const int32 ActorID = MissionTracker->GetActorID();

	const FString BaseString = FString::Printf(TEXT("RemoveMissionFromActor -- ActorID: %i"), ActorID);
	
	//
	// Grant the skills, @todo  if they are not already granted
	const FGameplayTag MissionTag = FGameplayTag::RequestGameplayTag(MissionName, false);

	const FPDMissionRow* FoundMissionDefaultData = nullptr; 
	if (MissionSubsystem->Utility.MissionLookupViaRowName.Contains(MissionName))
	{
		const FDataTableRowHandle* FoundHandle = MissionSubsystem->Utility.MissionLookupViaRowName.Find(MissionName);
		FoundMissionDefaultData = FoundHandle->GetRow<FPDMissionRow>("");
	}

	const int32 PotentialSID = MissionSubsystem->Utility.ResolveMIDViaTag(MissionTag);
	const int32 mID = MissionTag != FGameplayTag() && PotentialSID != INDEX_NONE ? PotentialSID : FoundMissionDefaultData != nullptr ? FoundMissionDefaultData->Base.mID : INDEX_NONE; 
	if (mID  == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s -- Found no mission by the name of '%s'"), *BaseString, *MissionName.ToString());
		return;
	}

	// Clear mission state and set its flags to INDEX_NONE to makr it as invalid, this way we can skip adding and removing from the map needlessly
	const FPDMissionBase PersistentDatum{MissionTag, mID, INDEX_NONE};
	MissionSubsystem->SetMission(ActorID, PersistentDatum);
}

void FPDPrivateMissionHandler::_AddTagsToContainer(TArray<FGameplayTag> NewTags, TSet<FGameplayTag>& ExistingTags)
{
	ExistingTags.Append(NewTags);
}

void FPDPrivateMissionHandler::_RemoveTagsToContainer(TArray<FGameplayTag> DeleteTags, TSet<FGameplayTag>& ExistingTags)
{
	for (const FGameplayTag& TagToDelete : DeleteTags)
	{
		ExistingTags.Remove(TagToDelete);
	}
}
