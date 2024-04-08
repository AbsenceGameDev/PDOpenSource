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

#include "MissionGraph/PDMissionGraph.h"
#include "MissionGraph/PDMissionGraphNode.h"
#include "PDMissionEditor.h"
#include "PDMissionGraphTypes.h"

#include "Containers/Array.h"
#include "Containers/EnumAsByte.h"

#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"

#include "HAL/PlatformMath.h"
#include "Logging/LogCategory.h"
#include "Logging/LogMacros.h"

#include "Serialization/Archive.h"
#include "Templates/Casts.h"
#include "Trace/Detail/Channel.h"

#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

UPDMissionGraph::UPDMissionGraph(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bHaltRefresh = false;
	GraphVersion = INDEX_NONE;
}

void UPDMissionGraph::UpdateData(int32 UpdateFlags)
{
	if (bHaltRefresh) { return; }

	// Fix up the parent node pointers (which are marked transient for some reason)
	for (UEdGraphNode* Node : Nodes)
	{
		UPDMissionGraphNode* AINode = Cast<UPDMissionGraphNode>(Node);
		
		if (AINode == nullptr) { continue; }
		
		for (UPDMissionGraphNode* SubNode : AINode->SubNodes)
		{
			if (SubNode == nullptr) { continue; }

			SubNode->ParentNode = AINode;
		}
	}

	// UDataTable* MissionTable = CastChecked<UDataTable>(GetOuter());
	// FPDMissionBuilder::RebuildData(MissionTable);
}

void UPDMissionGraph::OnCreated()
{
	MarkVersion();
}

void UPDMissionGraph::OnLoaded()
{
	UpdateDeprecatedClasses();
	UpdateUnknownNodeClasses();
}

void UPDMissionGraph::Initialize()
{
	UpdateVersion();
}

void UPDMissionGraph::UpdateVersion()
{
	if (GraphVersion == 1)
	{
		return;
	}

	MarkVersion();
	Modify();
}

void UPDMissionGraph::MarkVersion()
{
	GraphVersion = 1;
}

bool UPDMissionGraph::UpdateUnknownNodeClasses()
{
	bool bUpdated = false;
	for (int32 NodeIdx = 0; NodeIdx < Nodes.Num(); NodeIdx++)
	{
		UPDMissionGraphNode* MyNode = Cast<UPDMissionGraphNode>(Nodes[NodeIdx]);
		if (MyNode)
		{
			const bool bUpdatedNode = MyNode->RefreshNodeClass();
			bUpdated = bUpdated || bUpdatedNode;

			for (int32 SubNodeIdx = 0; SubNodeIdx < MyNode->SubNodes.Num(); SubNodeIdx++)
			{
				if (MyNode->SubNodes[SubNodeIdx])
				{
					const bool bUpdatedSubNode = MyNode->SubNodes[SubNodeIdx]->RefreshNodeClass();
					bUpdated = bUpdated || bUpdatedSubNode;
				}
			}
		}
	}

	return bUpdated;
}

void UpdateMissionGraphNodeErrorMessage(UPDMissionGraphNode& Node)
{
	// Broke out setting error message in to own function so it can be reused when iterating nodes collection.
	if (Node.NodeInstance)
	{
		Node.ErrorMessage = ""; // @todo 

		// Only check for node-specific errors if the node is not deprecated
		if (Node.ErrorMessage.IsEmpty())
		{
			Node.UpdateErrorMessage();

			// For node-specific validation we don't want to spam the log with errors
			return;
		}
	}
	else
	{
		// Null instance. Do we have any meaningful class data?
		FString StoredClassName = Node.ClassData.GetDataEntryName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));

		if (!StoredClassName.IsEmpty())
		{
			// There is class data here but the instance was not be created.
			static const FString IsMissingClassMessage(" class missing. Referenced by ");
			Node.ErrorMessage = StoredClassName + IsMissingClassMessage + Node.GetFullName();
		}
	}

	if (Node.HasErrors())
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Node.ErrorMessage);
	}
}

void UPDMissionGraph::UpdateDeprecatedClasses()
{
	// This function sets error messages and logs errors about nodes.

	for (int32 Idx = 0, IdxNum = Nodes.Num(); Idx < IdxNum; ++Idx)
	{
		UPDMissionGraphNode* Node = Cast<UPDMissionGraphNode>(Nodes[Idx]);
		if (Node != nullptr)
		{
			UpdateMissionGraphNodeErrorMessage(*Node);
			
			for (int32 SubIdx = 0, SubIdxNum = Node->SubNodes.Num(); SubIdx < SubIdxNum; ++SubIdx)
			{
				if (Node->SubNodes[SubIdx] != nullptr)
				{
					UpdateMissionGraphNodeErrorMessage(*Node->SubNodes[SubIdx]);
				}
			}
		}
	}
}

void UPDMissionGraph::Serialize(FArchive& Ar)
{
	// Overridden to flags up errors in the behavior tree while cooking.
	Super::Serialize(Ar);

	// Execute UpdateDeprecatedClasses only when saving to persistent storage,
	// otherwise node instances might not be fully created (i.e. transaction buffer while loading the asset).
	if ((Ar.IsSaving() && Ar.IsPersistent()) || Ar.IsCooking())
	{
		// Logging of errors happens in UpdateDeprecatedClasses
		UpdateDeprecatedClasses();
	}
}

void UPDMissionGraph::UpdateClassData()
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UPDMissionGraphNode* Node = Cast<UPDMissionGraphNode>(Nodes[Idx]);
		if (Node)
		{
			Node->UpdateNodeClassData();

			for (int32 SubIdx = 0; SubIdx < Node->SubNodes.Num(); SubIdx++)
			{
				if (UPDMissionGraphNode* SubNode = Node->SubNodes[SubIdx])
				{
					SubNode->UpdateNodeClassData();
				}
			}
		}
	}
}

void UPDMissionGraph::CollectAllNodeInstances(TSet<UObject*>& NodeInstances)
{
	for (int32 Idx = 0; Idx < Nodes.Num(); Idx++)
	{
		UPDMissionGraphNode* MyNode = Cast<UPDMissionGraphNode>(Nodes[Idx]);
		if (MyNode)
		{
			NodeInstances.Add(MyNode->NodeInstance);

			for (int32 SubIdx = 0; SubIdx < MyNode->SubNodes.Num(); SubIdx++)
			{
				if (MyNode->SubNodes[SubIdx])
				{
					NodeInstances.Add(MyNode->SubNodes[SubIdx]->NodeInstance);
				}
			}
		}
	}
}

bool UPDMissionGraph::CanRemoveNestedObject(UObject* TestObject) const
{
	return !TestObject->IsA(UEdGraphNode::StaticClass()) &&
		!TestObject->IsA(UEdGraph::StaticClass()) &&
		!TestObject->IsA(UEdGraphSchema::StaticClass());
}

void UPDMissionGraph::RemoveOrphanedNodes()
{
	TSet<UObject*> NodeInstances;
	CollectAllNodeInstances(NodeInstances);

	NodeInstances.Remove(nullptr);

	// Obtain a list of all nodes actually in the asset and discard unused nodes
	TArray<UObject*> AllInners;
	GetObjectsWithOuter(GetOuter(), AllInners, false);
	for (auto InnerIt = AllInners.CreateConstIterator(); InnerIt; ++InnerIt)
	{
		UObject* TestObject = *InnerIt;
		if (!NodeInstances.Contains(TestObject) && CanRemoveNestedObject(TestObject))
		{
			OnNodeInstanceRemoved(TestObject);

			TestObject->SetFlags(RF_Transient);
			TestObject->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional | REN_ForceNoResetLoaders);
		}
	}
}

void UPDMissionGraph::OnNodeInstanceRemoved(UObject* NodeInstance)
{
	// empty in base class
}

void UPDMissionGraph::OnNodesPasted(const FString& ImportStr)
{
	// empty in base class
}

UEdGraphPin* UPDMissionGraph::FindGraphNodePin(UEdGraphNode* Node, EEdGraphPinDirection Dir)
{
	UEdGraphPin* Pin = nullptr;
	for (int32 Idx = 0; Idx < Node->Pins.Num(); Idx++)
	{
		if (Node->Pins[Idx]->Direction == Dir)
		{
			Pin = Node->Pins[Idx];
			break;
		}
	}

	return Pin;
}

bool UPDMissionGraph::IsLocked() const
{
	return bHaltRefresh;
}

void UPDMissionGraph::LockUpdates()
{
	bHaltRefresh = true;
}

void UPDMissionGraph::UnlockUpdates()
{
	bHaltRefresh = false;
	UpdateData();
}

void UPDMissionGraph::OnSubNodeDropped()
{
	NotifyGraphChanged();
}











