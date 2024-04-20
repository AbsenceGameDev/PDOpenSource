﻿/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */


#include "MissionGraph/PDMissionGraphNode.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "AssetRegistry/AssetData.h"
#include "EdGraph/EdGraphSchema.h"
#include "MissionGraph/PDMissionGraph.h"
#include "DiffResults.h"
#include "ScopedTransaction.h"
#include "BlueprintNodeHelpers.h"
#include "GameplayTagContainer.h"
#include "SGraphNodeKnot.h"
#include "MissionGraph/PDMissionGraphSchema.h"

#define LOCTEXT_NAMESPACE "MissionGraph"

UPDMissionGraphNode::UPDMissionGraphNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeInstance = nullptr;
	CopySubNodeIndex = 0;
	bIsReadOnly = false;
	bIsSubNode = false;
}

void UPDMissionGraphNode::InitializeInstance()
{
	// empty in base class
}

void UPDMissionGraphNode::PostPlacedNewNode()
{
	// NodeInstance can be already spawned by paste operation, don't override it

	const UClass* NodeClass = ClassData.GetClass(true);
	if (NodeClass == nullptr || (NodeInstance != nullptr)) { return; }
	
	const UEdGraph* MyGraph = GetGraph();
	const UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;
	if (GraphOwner == nullptr) { return;}
		
	// StructOnScopePropertyOwner = NewObject<UStruct>(GraphOwner, NodeStruct);
	NodeInstance = NewObject<UStruct>(GetTransientPackage(), TEXT("StructOnScope"), RF_Transient);
	NodeInstance->SetFlags(RF_Transactional);
	InitializeInstance();
}

bool UPDMissionGraphNode::CanDuplicateNode() const
{
	return bIsReadOnly ? false : Super::CanDuplicateNode();
}

bool UPDMissionGraphNode::CanUserDeleteNode() const
{
	return bIsReadOnly ? false : Super::CanUserDeleteNode();
}

void UPDMissionGraphNode::PrepareForCopying()
{
	if (NodeInstance == nullptr) { return; }
	
	// Temporarily take ownership of the node instance, so that it is not deleted when cutting
	NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UPDMissionGraphNode::PostEditImport()
{
	ResetNodeOwner();
	if (NodeInstance == nullptr) { return; }

	InitializeInstance();
}

void UPDMissionGraphNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
	ResetNodeOwner();
	if (ParentNode == nullptr) { return; }

	ParentNode->SubNodes.AddUnique(this);
}

void UPDMissionGraphNode::PostCopyNode()
{
	ResetNodeOwner();
}

void UPDMissionGraphNode::ResetNodeOwner()
{
	if (NodeInstance == nullptr) { return; }
	
	const UEdGraph* MyGraph = GetGraph();
	UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;

	NodeInstance->Rename(nullptr, GraphOwner, REN_DontCreateRedirectors | REN_DoNotDirty);
	NodeInstance->ClearFlags(RF_Transient);

	for (const TObjectPtr<UPDMissionGraphNode>& SubNode : SubNodes)
	{
		SubNode->ResetNodeOwner();
	}
}

void UPDMissionGraphNode::CreateMissionPin()
{
	FEdGraphTerminalType ValueTerminalType;
	ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Struct;
	ValueTerminalType.TerminalSubCategory = NAME_None;
	ValueTerminalType.TerminalSubCategoryObject = FPDMissionRow::StaticStruct();
		
	UEdGraphNode::FCreatePinParams PinParam;
	PinParam.Index = 2;
	PinParam.bIsReference = false;
	PinParam.ContainerType = EPinContainerType::None;
	PinParam.ValueTerminalType = ValueTerminalType;
		
	FEdGraphPinType PinType_Mission(FPDMissionGraphTypes::PinCategory_MissionRow, NAME_None, FPDMissionRow::StaticStruct(), PinParam.ContainerType, PinParam.bIsReference, PinParam.ValueTerminalType);
	PinType_Mission.bIsConst = PinParam.bIsConst;
	CreatePin(EGPD_Input, PinType_Mission, TEXT("Mission"), PinParam.Index);

	// uncomment next commit
	// FEdGraphPinType PinType_DataRef(FPDMissionGraphTypes::PinCategory_MissionDataRef, NAME_None, FPDMissionRow::StaticStruct(), PinParam.ContainerType, PinParam.bIsReference, PinParam.ValueTerminalType);
	// PinType_DataRef.bIsConst = PinParam.bIsConst;
	// CreatePin(EGPD_Input, PinType_DataRef, TEXT("DataRef"));	
}

void UPDMissionGraphNode::RefreshDataRefPins(FName MissionRowName)
{
	if (MissionRowName == "--New Mission Row--")
	{
		// Generate pin

		FEdGraphTerminalType ValueTerminalType;
		ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Struct;
		ValueTerminalType.TerminalSubCategory = NAME_None;
		ValueTerminalType.TerminalSubCategoryObject = FPDMissionRow::StaticStruct();
		
		UEdGraphNode::FCreatePinParams PinParam;
		PinParam.Index = 3;
		PinParam.bIsReference = false;
		PinParam.ContainerType = EPinContainerType::None;
		PinParam.ValueTerminalType = ValueTerminalType;
		

		FEdGraphPinType PinType_Key(FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder, NAME_None, FPDMissionRow::StaticStruct(), PinParam.ContainerType, PinParam.bIsReference, PinParam.ValueTerminalType);
		PinType_Key.bIsConst = PinParam.bIsConst;
		CreatePin(EGPD_Input, PinType_Key, TEXT("New data key"), PinParam.Index);			
	}
	else if (Pins.Num() > 3)
	{
		RemovePinAt(3, EGPD_Input);
	}

}


FText UPDMissionGraphNode::GetDescription() const
{
	FString StoredClassName = ClassData.GetDataEntryName();
	StoredClassName.RemoveFromEnd(TEXT("_C"));
	
	return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
}

FText UPDMissionGraphNode::GetTooltipText() const
{
	if (NodeInstance == nullptr)
	{
		FString StoredClassName = ClassData.GetDataEntryName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));
		return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}
	
	if (ErrorMessage.Len() > 0)
	{
		return FText::FromString(ErrorMessage);
	}

	if (NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) == false)
	{
		return NodeInstance->GetClass()->GetToolTipText();
	}
	
	FText TooltipDesc;
	const FAssetData AssetData(NodeInstance->GetClass()->ClassGeneratedBy);
	FString Description = AssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription));
	if (Description.IsEmpty() == false)
	{
		Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
		TooltipDesc = FText::FromString(MoveTemp(Description));
	}

	return TooltipDesc;
}

UEdGraphPin* UPDMissionGraphNode::GetInputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction != EGPD_Input) { continue; }
		
		if (InputIndex == FoundInputs) { return Pins[PinIndex]; }
		
		FoundInputs++;
	}

	return nullptr;
}

UEdGraphPin* UPDMissionGraphNode::GetOutputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction != EGPD_Output) { continue; }
		
		if (InputIndex == FoundInputs) { return Pins[PinIndex]; }
		
		FoundInputs++;
	}

	return nullptr;
}

void UPDMissionGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin == nullptr) { return; }
	
	UEdGraphPin* OutputPin = GetOutputPin();
	if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
	{
		FromPin->GetOwningNode()->NodeConnectionListChanged();
	}
	else if (OutputPin != nullptr && GetSchema()->TryCreateConnection(OutputPin, FromPin))
	{
		NodeConnectionListChanged();
	}
}

UPDMissionGraph* UPDMissionGraphNode::GetMissionGraph()
{
	return CastChecked<UPDMissionGraph>(GetGraph());
}

bool UPDMissionGraphNode::IsSubNode() const
{
	return bIsSubNode || (ParentNode != nullptr);
}

void UPDMissionGraphNode::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	GetMissionGraph()->UpdateData();
}

bool UPDMissionGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	check(DesiredSchema);
	return DesiredSchema->GetClass()->IsChildOf(UPDMissionGraphSchema::StaticClass());
}

FString DescribeProperty(const FProperty* Prop, const uint8* PropertyAddr)
{
	FString ExportedStringValue;
	const FStructProperty* StructProp = CastField<const FStructProperty>(Prop);
	const FFloatProperty* FloatProp = CastField<const FFloatProperty>(Prop);

	if (StructProp && StructProp->Struct && StructProp->Struct->IsChildOf(TBaseStructure<FGameplayTag>::Get()))
	{
		ExportedStringValue = ((const FGameplayTag*)PropertyAddr)->ToString();

#if WITH_EDITOR
		const FString CategoryLimit = StructProp->GetMetaData(TEXT("Categories"));
		if (!CategoryLimit.IsEmpty() && ExportedStringValue.StartsWith(CategoryLimit))
		{
			ExportedStringValue.MidInline(CategoryLimit.Len(), MAX_int32, EAllowShrinking::No);
		}
#endif
	}
	else if (FloatProp)
	{
		// special case for floats to remove unnecessary zeros
		const float FloatValue = FloatProp->GetPropertyValue(PropertyAddr);
		ExportedStringValue = FString::SanitizeFloat(FloatValue);
	}
	else
	{
		Prop->ExportTextItem_Direct(ExportedStringValue, PropertyAddr, nullptr, nullptr, PPF_PropertyWindow, nullptr);
	}

	const bool bIsBool = Prop->IsA(FBoolProperty::StaticClass());
	return FString::Printf(TEXT("%s: %s"), *FName::NameToDisplayString(Prop->GetName(), bIsBool), *ExportedStringValue);
}

FString UPDMissionGraphNode::GetPropertyNameAndValueForDiff(const FProperty* Prop, const uint8* PropertyAddr) const
{
	return DescribeProperty(Prop, PropertyAddr);
}

void UPDMissionGraphNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	if (const UPDMissionGraphNode* OtherGraphNode = Cast<UPDMissionGraphNode>(OtherNode))
	{
		if (NodeInstance && OtherGraphNode->NodeInstance)
		{
			FDiffSingleResult Diff;
			Diff.Diff = EDiffType::NODE_PROPERTY;
			Diff.Node1 = this;
			Diff.Node2 = OtherNode;
			Diff.ToolTip = LOCTEXT("DIF_NodeInstancePropertyToolTip", "A property of the node instance has changed");
			Diff.Category = EDiffType::MODIFICATION;

			DiffProperties(NodeInstance->GetClass(), OtherGraphNode->NodeInstance->GetClass(), NodeInstance, OtherGraphNode->NodeInstance, Results, Diff);
		}
	}
}

void UPDMissionGraphNode::AddSubNode(UPDMissionGraphNode* SubNode, class UEdGraph* ParentGraph)
{
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	Modify();

	SubNode->SetFlags(RF_Transactional);

	// set outer to be the graph so it doesn't go away
	SubNode->Rename(nullptr, ParentGraph, REN_NonTransactional);
	SubNode->ParentNode = this;

	SubNode->CreateNewGuid();
	SubNode->PostPlacedNewNode();
	SubNode->AllocateDefaultPins();
	SubNode->AutowireNewNode(nullptr);

	SubNode->NodePosX = 0;
	SubNode->NodePosY = 0;

	SubNodes.Add(SubNode);
	OnSubNodeAdded(SubNode);

	ParentGraph->NotifyGraphChanged();
	GetMissionGraph()->UpdateData();
}

void UPDMissionGraphNode::OnSubNodeAdded(UPDMissionGraphNode* SubNode)
{
	// empty in base class
}

void UPDMissionGraphNode::RemoveSubNode(UPDMissionGraphNode* SubNode)
{
	Modify();
	SubNodes.RemoveSingle(SubNode);

	OnSubNodeRemoved(SubNode);
}

void UPDMissionGraphNode::RemoveAllSubNodes()
{
	SubNodes.Reset();
}

void UPDMissionGraphNode::OnSubNodeRemoved(UPDMissionGraphNode* SubNode)
{
	// empty in base class
}

int32 UPDMissionGraphNode::FindSubNodeDropIndex(UPDMissionGraphNode* SubNode) const
{
	const int32 InsertIndex = SubNodes.IndexOfByKey(SubNode);
	return InsertIndex;
}

void UPDMissionGraphNode::InsertSubNodeAt(UPDMissionGraphNode* SubNode, int32 DropIndex)
{
	if (DropIndex > -1)
	{
		SubNodes.Insert(SubNode, DropIndex);
	}
	else
	{
		SubNodes.Add(SubNode);
	}
}

void UPDMissionGraphNode::DestroyNode()
{
	if (ParentNode) { ParentNode->RemoveSubNode(this); }

	UEdGraphNode::DestroyNode();
}

bool UPDMissionGraphNode::UsesBlueprint() const
{
	return NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
}

bool UPDMissionGraphNode::RefreshNodeClass()
{
	bool bUpdated = false;
	if (NodeInstance == nullptr)
	{
		// @todo replace as FPDMissionDataNodeHelper was unneeded
		// if (FPDMissionDataNodeHelper::IsClassKnown(ClassData))
		// {
		// 	PostPlacedNewNode();
		// 	bUpdated = (NodeInstance != nullptr);
		// }
		// else
		// {
		// 	FPDMissionDataNodeHelper::AddUnknownClass(ClassData);
		// }
	}

	return bUpdated;
}

void UPDMissionGraphNode::UpdateNodeClassData()
{
	if (NodeInstance)
	{
		UpdateNodeDataFrom(NodeInstance->GetClass(), ClassData);
		UpdateErrorMessage();
	}
}

void UPDMissionGraphNode::UpdateErrorMessage()
{
	ErrorMessage = ClassData.GetDeprecatedMessage();
}

void UPDMissionGraphNode::UpdateNodeDataFrom(UClass* InstanceClass, FPDMissionNodeData& UpdatedData)
{
	if (InstanceClass)
	{
		if (const UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy))
		{
			UpdatedData = FPDMissionNodeData(BPOwner->GetName(), BPOwner->GetOutermost()->GetName(), InstanceClass->GetName(), InstanceClass);
		}
		else if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(InstanceClass))
		{
			UpdatedData = FPDMissionNodeData(BPGC->GetClassPathName(), BPGC);
		}
		else
		{
			UpdatedData = FPDMissionNodeData(InstanceClass, "");
		}
	}
}

bool UPDMissionGraphNode::HasErrors() const
{
	return ErrorMessage.Len() > 0 || NodeInstance == nullptr;
}

#undef LOCTEXT_NAMESPACE




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
	if(FromPin && Pins.Contains(FromPin))
	{
		return FromPin == Pins[0] ? Pins[1] : Pins[0];
	}

	return nullptr;
}

TSharedPtr<SGraphNode> UPDMissionGraphNode_Knot::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
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