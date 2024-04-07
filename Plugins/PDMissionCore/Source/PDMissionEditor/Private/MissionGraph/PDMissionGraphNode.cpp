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

	UStruct* NodeStruct = ClassData.GetStruct(true);
	if (NodeStruct && (NodeInstance == nullptr))
	{
		UEdGraph* MyGraph = GetGraph();
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;
		if (GraphOwner)
		{
			// StructOnScopePropertyOwner = NewObject<UStruct>(GraphOwner, NodeStruct);
			NodeInstance = NewObject<UStruct>(GetTransientPackage(), TEXT("StructOnScope"), RF_Transient);
			NodeInstance->SetFlags(RF_Transactional);
			InitializeInstance();
		}
	}
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
	if (NodeInstance)
	{
		// Temporarily take ownership of the node instance, so that it is not deleted when cutting
		NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
	}
}
#if WITH_EDITOR

void UPDMissionGraphNode::PostEditImport()
{
	ResetNodeOwner();

	if (NodeInstance)
	{
		InitializeInstance();
	}
}

void UPDMissionGraphNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
	ResetNodeOwner();
	
	if (ParentNode)
	{
		ParentNode->SubNodes.AddUnique(this);
	}
}

#endif

void UPDMissionGraphNode::PostCopyNode()
{
	ResetNodeOwner();
}

void UPDMissionGraphNode::ResetNodeOwner()
{
	if (NodeInstance)
	{
		UEdGraph* MyGraph = GetGraph();
		UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;

		NodeInstance->Rename(NULL, GraphOwner, REN_DontCreateRedirectors | REN_DoNotDirty);
		NodeInstance->ClearFlags(RF_Transient);

		for (auto& SubNode : SubNodes)
		{
			SubNode->ResetNodeOwner();
		}
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
	FText TooltipDesc;

	if (!NodeInstance)
	{
		FString StoredClassName = ClassData.GetDataEntryName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		TooltipDesc = FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}
	else
	{
		if (ErrorMessage.Len() > 0)
		{
			TooltipDesc = FText::FromString(ErrorMessage);
		}
		else
		{
			if (NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
			{
				FAssetData AssetData(NodeInstance->GetClass()->ClassGeneratedBy);
				FString Description = AssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription));
				if (!Description.IsEmpty())
				{
					Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
					TooltipDesc = FText::FromString(MoveTemp(Description));
				}
			}
			else
			{
				TooltipDesc = NodeInstance->GetClass()->GetToolTipText();
			}
		}
	}

	return TooltipDesc;
}

UEdGraphPin* UPDMissionGraphNode::GetInputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Input)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return nullptr;
}

UEdGraphPin* UPDMissionGraphNode::GetOutputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return nullptr;
}

void UPDMissionGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
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
	// override in child class
	return false;
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
		Prop->ExportTextItem_Direct(ExportedStringValue, PropertyAddr, NULL, NULL, PPF_PropertyWindow, NULL);
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

	if (UPDMissionGraphNode* OtherGraphNode = Cast<UPDMissionGraphNode>(OtherNode))
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
	if (ParentNode)
	{
		ParentNode->RemoveSubNode(this);
	}

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
		if (FPDMissionDataNodeHelper::IsClassKnown(ClassData))
		{
			PostPlacedNewNode();
			bUpdated = (NodeInstance != nullptr);
		}
		else
		{
			FPDMissionDataNodeHelper::AddUnknownClass(ClassData);
		}
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
		if (UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy))
		{
			UpdatedData = FPDMissionNodeData(BPOwner->GetName(), BPOwner->GetOutermost()->GetName(), InstanceClass->GetName(), InstanceClass);
		}
		else if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(InstanceClass))
		{
			UpdatedData = FPDMissionNodeData(BPGC->GetClassPathName(), BPGC);
		}
		else
		{
			UpdatedData = FPDMissionNodeData(InstanceClass, FPDMissionDataNodeHelper::GetDeprecationMessage(InstanceClass));
		}
	}
}

bool UPDMissionGraphNode::HasErrors() const
{
	return ErrorMessage.Len() > 0 || NodeInstance == nullptr;
}

#undef LOCTEXT_NAMESPACE




