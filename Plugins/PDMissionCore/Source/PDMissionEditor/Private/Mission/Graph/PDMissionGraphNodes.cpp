/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Mission/Graph/PDMissionGraphNodes.h"
#include "Mission/Graph/PDMissionGraphSchema.h"
#include "Mission/Slate/PDMissionView.h"

#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistry/AssetData.h"
#include "EdGraph/EdGraphSchema.h"
#include "ScopedTransaction.h"
#include "SGraphNodeKnot.h"
#include "Mission/Graph/PDMissionGraphNodeBase.h"

#define LOCTEXT_NAMESPACE "MissionGraph"

static const char* PC_Wildcard = "wildcard";

//
// UMissionGraphNode_Knot
UPDMissionGraphNode_Knot::UPDMissionGraphNode_Knot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCanRenameNode = true;
}

void UPDMissionGraphNode_Knot::AllocateDefaultPins()
{
	const FName InputPinName(TEXT("InputPin"));
	const FName InputDataPin0_Name(TEXT("DataPin0"));
	const FName OutputPinName(TEXT("OutputPin"));

	CreatePin(EGPD_Input, PC_Wildcard, InputPinName)->bDefaultValueIsIgnored = true;
	CreatePin(EGPD_Input, PC_Wildcard, InputDataPin0_Name);
	CreatePin(EGPD_Output, PC_Wildcard, OutputPinName);
}

FText UPDMissionGraphNode_Knot::GetTooltipText() const
{
	//@TODO: Should pull the tooltip from the source pin
	return LOCTEXT("KnotTooltip", "Reroute Node (reroutes wires)");
}

FText UPDMissionGraphNode_Knot::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	switch (TitleType)
	{
	case ENodeTitleType::EditableTitle:
		return FText::FromString(NodeComment);
	case ENodeTitleType::MenuTitle:
		return LOCTEXT("KnotListTitle", "Add Reroute Node...");
	
	default:	
	case ENodeTitleType::FullTitle:
	case ENodeTitleType::ListView:
	case ENodeTitleType::MAX_TitleTypes:
		return LOCTEXT("KnotTitle", "Reroute Node");
	}
}

bool UPDMissionGraphNode_Knot::ShouldOverridePinNames() const
{
	return true;
}

FText UPDMissionGraphNode_Knot::GetPinNameOverride(const UEdGraphPin& Pin) const
{
	// Keep the pin size tiny
	return FText::GetEmpty();
}

void UPDMissionGraphNode_Knot::OnRenameNode(const FString& NewName)
{
	NodeComment = NewName;
}

bool UPDMissionGraphNode_Knot::CanSplitPin(const UEdGraphPin* Pin) const
{
	return false;
}

TSharedPtr<class INameValidatorInterface> UPDMissionGraphNode_Knot::MakeNameValidator() const
{
	// Comments can be duplicated, etc...
	return MakeShareable(new FDummyNameValidator(EValidatorResult::Ok));
}

UEdGraphPin* UPDMissionGraphNode_Knot::GetPassThroughPin(const UEdGraphPin* FromPin) const
{
	const bool bValidPin = FromPin != nullptr && Pins.Contains(FromPin);
	return bValidPin ? (FromPin == Pins[0] ? Pins[1] : Pins[0]) : nullptr;
}

TSharedPtr<SGraphNode> UPDMissionGraphNode_Knot::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
}

bool UPDMissionGraphNode_Knot::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	check(Schema);
	return Schema->GetClass()->IsChildOf(UPDMissionGraphSchema::StaticClass());
}


UPDMissionTransitionNode::UPDMissionTransitionNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PriorityOrder = 1;
}

void UPDMissionTransitionNode::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, TEXT("Transition"), TEXT("In"));
	Inputs->bHidden = true;
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, TEXT("Transition"), TEXT("Out"));
	Outputs->bHidden = true;
}

void UPDMissionTransitionNode::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();
}

void UPDMissionTransitionNode::PostLoad()
{
	Super::PostLoad();
}


void UPDMissionTransitionNode::PostPasteNode()
{
	Super::PostPasteNode();

	// We don't want to paste nodes in that aren't fully linked (transition nodes have fixed pins as they
	// really describe the connection between two other nodes). If we find one missing link, get rid of the node.
	for(const UEdGraphPin* Pin : Pins)
	{
		if (Pin->LinkedTo.IsEmpty() == false) { continue; }
		
		DestroyNode();
		break;
	}
}

FText UPDMissionTransitionNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UPDMissionGraphNode* OwningMission = GetOwningMission();
	UPDMissionGraphNode* TargetMission = GetTargetMission();
	FFormatNamedArguments Args;

	if ((OwningMission != nullptr) && (TargetMission != nullptr))
	{
		Args.Add(TEXT("PrevState"), FText::FromString(OwningMission->GetMissionName()) );
		Args.Add(TEXT("NextState"), FText::FromString(TargetMission->GetMissionName()) );

		return FText::Format(LOCTEXT("PrevStateToNewState", "{PrevState} to {NextState}"), Args);
	}
	
	// @todo set up logic for the owning/bound mission 
	Args.Add(TEXT("BoundGraph"), LOCTEXT("FinishMe", "(finish me)") );
	// @TODO: FText::Format() is slow, and we could benefit from caching
	return FText::Format(LOCTEXT("TransitionMission", "Transition {BoundMission}}"), Args);
}

FText UPDMissionTransitionNode::GetTooltipText() const
{
	return LOCTEXT("StateTransitionTooltip", "This is a state transition");
}

UPDMissionGraphNode* UPDMissionTransitionNode::GetLinkedNode(TArray<UEdGraphPin*>& Links, int32 LinkIdx)
{
	return (Links.IsEmpty() == false) ? Cast<UPDMissionGraphNode>(Links[LinkIdx]->GetOwningNode()) : nullptr;
}

UPDMissionGraphNode* UPDMissionTransitionNode::GetOwningMission() const
{
	return GetLinkedNode(Pins[0]->LinkedTo, 0);
}

UPDMissionGraphNode* UPDMissionTransitionNode::GetTargetMission() const
{
	return GetLinkedNode(Pins[1]->LinkedTo, 0);
}

FLinearColor UPDMissionTransitionNode::GetNodeTitleColor() const
{
	return FColorList::Red;
}

void UPDMissionTransitionNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin->LinkedTo.IsEmpty() == false) { return; }
	
	// Destroy if no links; transitions must always have an input and output connection
	Modify();

	// Our parent graph will have our graph in SubGraphs so needs to be modified to record that.
	UEdGraph* ParentGraph = GetGraph();
	if (ParentGraph != nullptr) { ParentGraph->Modify(); }
	
	DestroyNode();
}

void UPDMissionTransitionNode::CreateConnections(const UPDMissionGraphNode* PreviousState, const UPDMissionGraphNode* NextState)
{
	// Previous to this
	Pins[0]->Modify();
	Pins[0]->LinkedTo.Empty();

	PreviousState->GetOutputPin()->Modify();
	Pins[0]->MakeLinkTo(PreviousState->GetOutputPin());

	// This to next
	Pins[1]->Modify();
	Pins[1]->LinkedTo.Empty();

	NextState->GetInputPin()->Modify();
	Pins[1]->MakeLinkTo(NextState->GetInputPin());
}

void UPDMissionTransitionNode::RelinkHead(const UPDMissionGraphNode* NewTargetState)
{
	const UPDMissionGraphNode* SourceState = GetOwningMission();
	const UPDMissionGraphNode* TargetStateBeforeRelinking = GetTargetMission();

	// Remove the incoming transition from the previous target state
	TargetStateBeforeRelinking->GetInputPin()->Modify();
	TargetStateBeforeRelinking->GetInputPin()->BreakLinkTo(SourceState->GetOutputPin());

	// Add the new incoming transition to the new target state
	NewTargetState->GetInputPin()->Modify();
	NewTargetState->GetInputPin()->MakeLinkTo(SourceState->GetOutputPin());

	// Relink the target state of the transition node
	Pins[1]->Modify();
	Pins[1]->BreakLinkTo(TargetStateBeforeRelinking->GetInputPin());
	Pins[1]->MakeLinkTo(NewTargetState->GetInputPin());
}

TArray<UPDMissionTransitionNode*> UPDMissionTransitionNode::GetListTransitionNodesToRelink(const UEdGraphPin* SourcePin, const UEdGraphPin* OldTargetPin, const TArray<UEdGraphNode*>& InSelectedGraphNodes)
{
	UPDMissionGraphNode* SourceMission = Cast<UPDMissionGraphNode>(SourcePin->GetOwningNode());
	if (SourceMission == nullptr || SourceMission->GetInputPin() == nullptr || SourceMission->GetOutputPin() == nullptr)
	{
		return {};
	}

	// Collect all transition nodes starting at the source state
	TArray<UPDMissionTransitionNode*> TransitionNodeCandidates;
	SourceMission->GetMissionTransitions(TransitionNodeCandidates);

	// Remove the transition nodes from the candidates that are linked to a different target state.
	for (int i = TransitionNodeCandidates.Num() - 1; i >= 0; i--)
	{
		UPDMissionTransitionNode* CurrentTransition = TransitionNodeCandidates[i];

		// Get the actual target states from the transition nodes
		const UEdGraphNode* TransitionTargetNode = CurrentTransition->GetTargetMission();
		const UPDMissionTransitionNode* CastedOldTarget = Cast<UPDMissionTransitionNode>(OldTargetPin->GetOwningNode());
		const UEdGraphNode* OldTargetNode = CastedOldTarget->GetTargetMission();

		// Compare the target states rather than comparing against the transition nodes
		if (TransitionTargetNode == OldTargetNode) { continue; }
		
		TransitionNodeCandidates.Remove(CurrentTransition);
	}

	// Collect the subset of selected transitions from the list of possible transitions to be relinked
	TSet<UPDMissionTransitionNode*> SelectedTransitionNodes;
	for (UEdGraphNode* GraphNode : InSelectedGraphNodes)
	{
		UPDMissionTransitionNode* TransitionNode = Cast<UPDMissionTransitionNode>(GraphNode);
		if (TransitionNode == nullptr || TransitionNodeCandidates.Find(TransitionNode) == INDEX_NONE)
		{
			continue;
		}
		
		SelectedTransitionNodes.Add(TransitionNode);
	}

	TArray<UPDMissionTransitionNode*> Result;
	Result.Reserve(TransitionNodeCandidates.Num());
	for (UPDMissionTransitionNode* TransitionNode : TransitionNodeCandidates)
	{
		// Only relink the selected transitions. If none are selected, relink them all.
		if (SelectedTransitionNodes.IsEmpty() == false && SelectedTransitionNodes.Find(TransitionNode) == nullptr)
		{
			continue;
		}

		Result.Add(TransitionNode);
	}

	return Result;
}

void UPDMissionTransitionNode::PrepareForCopying()
{
	Super::PrepareForCopying();
}

void UPDMissionTransitionNode::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//
	//

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

FString UPDMissionTransitionNode::GetMissionName() const
{
	return "finish me"; // @todo 1. rename to getmission name then 2. set up logic for the owning mission 
}

void UPDMissionTransitionNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar.UsingCustomVersion(FAnimPhysObjectVersion::GUID);
	Ar.UsingCustomVersion(FUE5MainStreamObjectVersion::GUID);
}

void UPDMissionTransitionNode::DestroyNode()
{
	// handle removing transition
	Super::DestroyNode();
}

#undef LOCTEXT_NAMESPACE


/**
Business Source License 1.1

Parameters

Licensor:             Ario Amin (@ Permafrost Development)
Licensed Work:        PDOpenSource (Source available on github)
                      The Licensed Work is (c) 2024 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make commercial use of the Licensed Work provided these three additional conditions as met; 
                      	1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      	2. The Licensed Work must be Compiled before being redistributed.
                      	3. The Licensed Work Source may not be packaged into the product or service being sold

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.
                      
                      "Source" form means the source code (.h & .cpp files) contained in the different modules in PDOpenSource.
                      This will usually be written in human-readable format.

                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of the files or codes therein..  

Change Date:          2028-04-17

Change License:       Apache License, Version 2.0

For information about alternative licensing arrangements for the Software,
please visit: N/A

Notice

The Business Source License (this document, or the “License”) is not an Open Source license.
However, the Licensed Work will eventually be made available under an Open Source License, as stated in this License.

License text copyright (c) 2017 MariaDB Corporation Ab, All Rights Reserved.
“Business Source License” is a trademark of MariaDB Corporation Ab.

-----------------------------------------------------------------------------

Business Source License 1.1

Terms

The Licensor hereby grants you the right to copy, modify, create derivative works, redistribute, and make non-production use of the Licensed Work.
The Licensor may make an Additional Use Grant, above, permitting limited production use.

Effective on the Change Date, or the fourth anniversary of the first publicly available distribution of a specific version of the Licensed Work under this License,
whichever comes first, the Licensor hereby grants you rights under the terms of the Change License, and the rights granted in the paragraph above terminate.

If your use of the Licensed Work does not comply with the requirements currently in effect as described in this License, you must purchase a
commercial license from the Licensor, its affiliated entities, or authorized resellers, or you must refrain from using the Licensed Work.

All copies of the original and modified Licensed Work, and derivative works of the Licensed Work, are subject to this License. This License applies
separately for each version of the Licensed Work and the Change Date may vary for each version of the Licensed Work released by Licensor.

You must conspicuously display this License on each original or modified copy of the Licensed Work. If you receive the Licensed Work
in original or modified form from a third party, the terms and conditions set forth in this License apply to your use of that work.

Any use of the Licensed Work in violation of this License will automatically terminate your rights under this License for the current
and all other versions of the Licensed Work.

This License does not grant you any right in any trademark or logo of Licensor or its affiliates (provided that you may use a
trademark or logo of Licensor as expressly required by this License).

TO THE EXTENT PERMITTED BY APPLICABLE LAW, THE LICENSED WORK IS PROVIDED ON AN “AS IS” BASIS. LICENSOR HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS,
EXPRESS OR IMPLIED, INCLUDING (WITHOUT LIMITATION) WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND TITLE.

MariaDB hereby grants you permission to use this License’s text to license your works, and to refer to it using the trademark
“Business Source License”, as long as you comply with the Covenants of Licensor below.

Covenants of Licensor

In consideration of the right to use this License’s text and the “Business Source License” name and trademark,
Licensor covenants to MariaDB, and to all other recipients of the licensed work to be provided by Licensor:

1. To specify as the Change License the GPL Version 2.0 or any later version, or a license that is compatible with GPL Version 2.0
   or a later version, where “compatible” means that software provided under the Change License can be included in a program with
   software provided under GPL Version 2.0 or a later version. Licensor may specify additional Change Licenses without limitation.

2. To either: (a) specify an additional grant of rights to use that does not impose any additional restriction on the right granted in
   this License, as the Additional Use Grant; or (b) insert the text “None”.

3. To specify a Change Date.

4. Not to modify this License in any other way.
 **/