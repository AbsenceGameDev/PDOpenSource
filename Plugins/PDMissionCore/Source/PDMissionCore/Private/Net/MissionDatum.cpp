/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */


#include "Net/MissionDatum.h"
#include "Components/PDMissionTracker.h"

void FPDMissionNetDatum::PreReplicatedRemove(const FPDMissionNetDataCompound& InArraySerializer)
{
	check(InArraySerializer.OwnerTracker != nullptr);
}

void FPDMissionNetDatum::PostReplicatedAdd(const FPDMissionNetDataCompound& InArraySerializer)
{
	check(InArraySerializer.OwnerTracker != nullptr);
	InArraySerializer.OwnerTracker->OnDatumUpdated(this);
}

void FPDMissionNetDatum::PostReplicatedChange(const FPDMissionNetDataCompound& InArraySerializer)
{
	check(InArraySerializer.OwnerTracker != nullptr);
	InArraySerializer.OwnerTracker->OnDatumUpdated(this);
}

bool FPDMissionNetDataCompound::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FPDMissionNetDatum, FPDMissionNetDataCompound>(Items, DeltaParams, *this);
}
