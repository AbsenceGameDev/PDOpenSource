/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

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