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

#include "PDMissionCommon.h"

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
// Progress status handler

void FPDMissionStatusHandler::AccumulateData(const FGameplayTag& InTag, FPDFMissionModData& DataCompound) const
{
}

void FPDMissionRules::IterateStatusHandlers(const FGameplayTag& Tag, FPDFMissionModData& OutStatVariables)
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

const void FPDMissionTagCompound::AppendUserTags(TArray<FGameplayTag> AppendTags)
{
	OptionalUserTags.Append(AppendTags);
}

const void FPDMissionTagCompound::RemoveUserTags(TArray<FGameplayTag> TagsToRemove)
{
	// Remove in order from last to first to avoid  having to recalculate indices
	for (const FGameplayTag& Tag : TagsToRemove)
	{
		OptionalUserTags.Remove(Tag);
	}
}

const void FPDMissionTagCompound::RemoveUserTag(FGameplayTag TagToRemove)
{
	OptionalUserTags.Remove(TagToRemove);
}

const void FPDMissionTagCompound::ClearUserTags(AActor* Caller)
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
