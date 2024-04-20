/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Subsystems/PDMissionUtility.h"
#include "Components/PDMissionTracker.h"
#include "Net/MissionDatum.h"

#include <Curves/CurveFloat.h>

#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataTableFactory.h"

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
	MissionTracker->SetMissionDatum(GetDefaultBase(SID)->Base.MissionBaseTag, Datum);
}

void FPDMissionUtility::OverwriteMissionDatum(UPDMissionTracker* MissionTracker, int32 SID, const FPDMissionNetDatum& NewDatum, bool ForceDefault) const
{
	const FPDMissionRow* DefaultMissionBaseDatum = GetDefaultBase(SID);
	
	ForceDefault = ForceDefault && DefaultMissionBaseDatum != nullptr;
	FPDMissionNetDatum Datum;
		Datum.mID = SID;
		Datum.State.Current = ForceDefault ? DefaultMissionBaseDatum->ProgressRules.EStartState : NewDatum.State.Current;
		Datum.State.MissionConditionHandler = ForceDefault ? DefaultMissionBaseDatum->ProgressRules.MissionConditionHandler : NewDatum.State.MissionConditionHandler;
	
	SetNewMissionDatum(MissionTracker, SID, Datum);
}



//
// SETUP

void FPDMissionUtility::InitializeMissionSubsystem()
{
	ProcessTablesForFastLookup();
}

const TArray<UDataTable*>& FPDMissionUtility::GetAllTables() const
{
	return MissionTables;
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
		BoundMissionEvents.Add(ActorID, {}); // {Event-List} = const FPDMissionTreeMap,
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
	
	for (UDataTable* MissionTable : MissionTables)
	{
		if (MissionTable == nullptr) { return; }

		const int32 CurrentID = MissionTable->GetUniqueID();
		MissionTable->OnDataTableChanged().AddLambda
		(
			[&, CurrentID]()
			{
				TableRevisions.FindOrAdd(CurrentID)++;
			}	
		);

		int32 MissionID = 0x0;
		bool bPackageWasDirtied = false;
		const TMap<FName, uint8*>& AllItems = MissionTable->GetRowMap();

		if (AllItems.IsEmpty() == false)
		{
			MissionTable->MarkPackageDirty();
		}
		
		for (TMap<FName, uint8*>::TConstIterator RowMapIter(AllItems.CreateConstIterator()); RowMapIter; ++RowMapIter)
		{
			FPDMissionRow* TableRow = reinterpret_cast<FPDMissionRow*>(RowMapIter.Value());
			if (TableRow == nullptr) { continue; }

			bPackageWasDirtied = true;

			// modify the table entry
			TableRow->Base.mID = ++MissionID;
			TableRow->Base.ResolveMissionTypeTag();
			MissionTable->HandleDataTableChanged(RowMapIter.Key());
			
			UE_LOG(LogTemp, Warning, TEXT("TableRow->Base.MissionTag: %s"), *TableRow->Base.MissionBaseTag.ToString())
			UE_LOG(LogTemp, Warning, TEXT("TableRow->Base.MissionCategory: %s"), *TableRow->Base.GetMissionTypeTag().ToString())
			UE_LOG(LogTemp, Warning, TEXT("TableRow->Base.mID: %i"), TableRow->Base.mID)
			
			FDataTableRowHandle RowHandle = UPDMissionStatics::CreateRowHandle(MissionTable, RowMapIter.Key());
			MissionLookup.Add(TableRow->Base.mID, RowHandle);
			MissionTagToMIDLookup.Add(TableRow->Base.MissionBaseTag, TableRow->Base.mID);
			MissionLookupViaRowName.Add(RowHandle.RowName, RowHandle);

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
			SuccessCounter++;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
		}

		if (bPackageWasDirtied && MissionTable->MarkPackageDirty() == false)
		{
			UE_LOG(LogTemp, Error, TEXT("MissionTable->MarkPackageDirty() failed. in mission subsystem initialize codepath"))
		}
		if (bPackageWasDirtied)
		{
			MissionTable->PreEditChange(nullptr);
			MissionTable->PostEditChange();	
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
			TArray<FGameplayTag> MissionTags;
			// @todo

		}
	}

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
	fSuccessCounter = FString{SuccessCounter != 0 ? "Succeeded" : "Failed"} + FString{" at creating mission lookup maps \n"};
	SuccessCounter = 0;
#endif // UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT
}

void FPDMissionUtility::InitializeTracker(const int32 ActorID)
{
	UPDMissionTracker* MissionTracker = GetActorTracker(ActorID);
	if (MissionTracker == nullptr || MissionTracker->GetOwnerRole() != ROLE_Authority)
	{
		return;
	};

	for (TTuple<int32, FDataTableRowHandle>& BaseMission : MissionLookup)
	{
		const FPDMissionRow* DefaulMission = BaseMission.Value.GetRow<FPDMissionRow>(TEXT(""));
		if (DefaulMission == nullptr) { continue; }
		
		FPDMissionNetDatum Mission{DefaulMission->Base.mID, FPDMissionState{DefaulMission->ProgressRules.EStartState, DefaulMission->ProgressRules.MissionConditionHandler}};
		MissionTracker->AddMissionDatum(Mission);
	}
}

void FPDMissionUtility::BindMissionEvent(int32 ActorID, int32 mID, const FPDUpdateMission& MissionEventDelegate)
{
	if (mID == INDEX_NONE) { return; }

	FPDMissionTreeMap* UserEventMapPtr = BoundMissionEvents.Find(ActorID);
	if (UserEventMapPtr) { UserEventMapPtr->Emplace(mID, MissionEventDelegate); }
}

bool FPDMissionUtility::ExecuteBoundMissionEvent(const int32 ActorID, const int32 mID, const EPDMissionState NewState)
{
	// Do we have and event map for this user/actor?
	FPDMissionTreeMap* UserEventMapPtr = BoundMissionEvents.Find(ActorID);
	if (UserEventMapPtr == nullptr) { return false; }

	// Find if there is any delegate bound to this specific SID, fire if true
	const FPDUpdateMission* TempDelegate = UserEventMapPtr->Find(mID);
	if (TempDelegate == nullptr) { return false; }

	TempDelegate->Broadcast(mID, NewState);
	return true;
}

// @todo call whenever intermediary table rows are changing
void FPDMissionUtility::FillIntermediaryMissionList(bool bOverwrite)
{
#if WITH_EDITOR
	
	if (bOverwrite == false && MissionRowNameList.IsEmpty() == false)
	{
		return;
	}
	
	if (bOverwrite && MissionRowNameList.IsEmpty() == false)
	{
		MissionRowNameList.Empty();
	}
	
	// PopulateMissionList
	TArray<FName> MissionRowNames;
	MissionLookupViaRowName.GenerateKeyArray(MissionRowNames);	

	IndexToName.Empty();
	IndexToName.FindOrAdd(
	MissionRowNameList.Emplace(MakeShared<FString>("--New Mission Row--")));

	for (const FName& MissionName : MissionRowNames)
	{
		const FPDMissionRow* MissionRow = MissionLookupViaRowName.FindRef(MissionName).GetRow<FPDMissionRow>("");
		FString BuildString = MissionRow->Base.MissionBaseTag.GetTagName().ToString() + " (" + MissionName.ToString() + ") ";
		
		MissionConcatList.Emplace(MakeShared<FString>(BuildString));
		
		IndexToName
		.FindOrAdd(MissionRowNameList.Emplace(MakeShared<FString>(MissionName.ToString())))
		= MissionName;
	}
#endif // #if WITH_EDITOR
	
}