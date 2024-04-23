/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

// Mission editor
#include "Mission/Graph/PDMissionGraphSchemaActions.h"
#include "Mission/Graph/PDMissionGraphNodes.h"
#include "Mission/Graph/PDMissionGraph.h"
#include "PDMissionGraphTypes.h"

// Edgraph
#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "Textures/SlateIcon.h"

// 
#include "Settings/EditorStyleSettings.h"

// Transations and Actions
#include "ToolMenu.h"
#include "ScopedTransaction.h"
#include "ToolMenuSection.h"

// Commands
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "MissionGraph"

namespace
{
	// Maximum distance a drag can be off a node edge to require 'push off' from node
	constexpr int32 NodeDistance = 60;
}


struct FPDCompareNodeXLocation
{
	FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
	{
		const UEdGraphNode* NodeA = A.GetOwningNode();
		const UEdGraphNode* NodeB = B.GetOwningNode();

		const bool bCompY = NodeA->NodePosX == NodeB->NodePosX;
		return bCompY ? NodeA->NodePosY < NodeB->NodePosY : NodeA->NodePosX < NodeB->NodePosX;
	}
};


namespace MissionGraphHelpers
{
	struct FIntIntPair
	{
		int32 FirstIdx;
		int32 LastIdx;
	};
	
	void ClearRootLevelFlags(UPDMissionGraph* Graph)
	{
		for (int32 Index = 0; Index < Graph->Nodes.Num(); Index++)
		{
			UPDMissionGraphNode* MissionNode = Cast<UPDMissionGraphNode>(Graph->Nodes[Index]);
			if (MissionNode) { MissionNode->bIsSubNode = true; }
		}
	}

	void RebuildExecutionOrder(UPDMissionGraphNode* RootEdNode, uint16* ExecutionIndex, uint8 TreeDepth)
	{
		if (RootEdNode == nullptr) { return; }

		// gather all nodes
		for (int32 PinIdx = 0; PinIdx < RootEdNode->Pins.Num(); PinIdx++)
		{
			const UEdGraphPin* Pin = RootEdNode->Pins[PinIdx];
			if (Pin->Direction != EGPD_Output) { continue; }

			// sort connections so that they're organized the same as user can see in the editor
			TArray<UEdGraphPin*> SortedPins = Pin->LinkedTo;
			SortedPins.Sort(FPDCompareNodeXLocation());

			for (int32 Index = 0; Index < SortedPins.Num(); ++Index)
			{
				const UPDMissionGraphNode* GraphNode = Cast<UPDMissionGraphNode>(SortedPins[Index]->GetOwningNode());
				if (GraphNode == nullptr) { continue; }
				
				// @todo 
			}
		}
	}

	UEdGraphPin* FindGraphNodePin(UEdGraphNode* Node, EEdGraphPinDirection Dir)
	{
		for (int32 Idx = 0, PinLimit = Node->Pins.Num(); Idx < PinLimit; Idx++)
		{
			if (Node->Pins[Idx]->Direction != Dir) { continue; }

			return Node->Pins[Idx];
		}
		return nullptr;
	}
	
} // namespace MissionGraphHelpers


namespace MissionAutoArrangeHelpers
{
	struct FNodeBoundsInfo
	{
		FDeprecateSlateVector2D SubGraphBBox;
		TArray<FNodeBoundsInfo> Children;
	};

	void AutoArrangeNodes(UPDMissionGraphNode* ParentNode, FNodeBoundsInfo& BBoxTree, float PosX, float PosY)
	{
		int32 BBoxIndex = 0;

		const UEdGraphPin* Pin = MissionGraphHelpers::FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin == nullptr) { return; }
		
		SGraphNode::FNodeSet NodeFilter;
		TArray<UEdGraphPin*> TempLinkedTo = Pin->LinkedTo;
		for (int32 Idx = 0; Idx < TempLinkedTo.Num(); Idx++)
		{
			UPDMissionGraphNode* GraphNode = Cast<UPDMissionGraphNode>(TempLinkedTo[Idx]->GetOwningNode());
			if (GraphNode && BBoxTree.Children.IsEmpty()) { continue; }
			
			AutoArrangeNodes(GraphNode, BBoxTree.Children[BBoxIndex], PosX, PosY + GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
			GraphNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FDeprecateSlateVector2D(BBoxTree.Children[BBoxIndex].SubGraphBBox.X / 2.f - GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X / 2.f + PosX, PosY), NodeFilter);
			PosX += BBoxTree.Children[BBoxIndex].SubGraphBBox.X + 20.f;
			BBoxIndex++;
		}
	}

	void GetNodeSizeInfo(UPDMissionGraphNode* ParentNode, FNodeBoundsInfo& BBoxTree)
	{
		BBoxTree.SubGraphBBox = ParentNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize();
		float LevelWidth = 0;
		float LevelHeight = 0;

		UEdGraphPin* Pin = MissionGraphHelpers::FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin == nullptr) { return; }
		
		Pin->LinkedTo.Sort(FPDCompareNodeXLocation());
		for (int32 Idx = 0; Idx < Pin->LinkedTo.Num(); Idx++)
		{
			UPDMissionGraphNode* GraphNode = Cast<UPDMissionGraphNode>(Pin->LinkedTo[Idx]->GetOwningNode());
			if (GraphNode == nullptr) { continue; }
			
			const int32 ChildIdx = BBoxTree.Children.Add(FNodeBoundsInfo());
			FNodeBoundsInfo& ChildBounds = BBoxTree.Children[ChildIdx];
			GetNodeSizeInfo(GraphNode, ChildBounds);
			LevelWidth += ChildBounds.SubGraphBBox.X + 20.f;
			LevelHeight = FMath::Max<float>(ChildBounds.SubGraphBBox.Y, LevelHeight);
		}
		BBoxTree.SubGraphBBox.X = FMath::Max<float>(LevelWidth, BBoxTree.SubGraphBBox.X);
		BBoxTree.SubGraphBBox.Y += LevelHeight;
	}
}


UEdGraphNode* FMissionSchemaAction_AddComment::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode_Comment* const CommentTemplate = NewObject<UEdGraphNode_Comment>();

	FVector2D SpawnLocation = Location;
	FSlateRect Bounds;

	const TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(ParentGraph);
	if (GraphEditorPtr.IsValid() && GraphEditorPtr->GetBoundsForSelectedNodes(/*out*/ Bounds, 50.0f))
	{
		CommentTemplate->SetBounds(Bounds);
		SpawnLocation.X = CommentTemplate->NodePosX;
		SpawnLocation.Y = CommentTemplate->NodePosY;
	}

	UEdGraphNode* const NewNode = FEdGraphSchemaAction_NewNode::SpawnNodeFromTemplate<UEdGraphNode_Comment>(ParentGraph, CommentTemplate, SpawnLocation, bSelectNewNode);
	return NewNode;
}

//////////////////////////////////////////////////////////////////////////

void FMissionSchemaAction_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject(NodeTemplate);
}


UEdGraphNode* FMissionSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	// If there is a template, we actually use it
	if (NodeTemplate == nullptr) { return ResultNode; }
	
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	if (FromPin)
	{
		FromPin->Modify();
	}
	NodeTemplate->SetFlags(RF_Transactional);

	// set outer to be the graph so it doesn't go away
	NodeTemplate->Rename(nullptr, ParentGraph, REN_NonTransactional);
	ParentGraph->AddNode(NodeTemplate, true);
	NodeTemplate->CreateNewGuid();
	NodeTemplate->PostPlacedNewNode();
	// For input pins, new node will generally overlap node being dragged off
	// Work out if we want to visually push away from connected node
	int32 XLocation = static_cast<int32>(Location.X);
	if (FromPin && FromPin->Direction == EGPD_Input)
	{
		const UEdGraphNode* PinNode = FromPin->GetOwningNode();
		const FVector::FReal XDelta = FMath::Abs(PinNode->NodePosX - Location.X);
		if (XDelta < NodeDistance)
		{
			// Set location to edge of current node minus the max move distance
			// to force node to push off from connect node enough to give selection handle
			XLocation = PinNode->NodePosX - NodeDistance;
		}
	}
	
	NodeTemplate->NodePosX = XLocation;
	NodeTemplate->NodePosY = static_cast<int32>(Location.Y);
	NodeTemplate->SnapToGrid(GetDefault<UEditorStyleSettings>()->GridSnapSize);

	// setup pins after placing node in correct spot, since pin sorting will happen as soon as link connection change occurs
	NodeTemplate->AllocateDefaultPins();
	NodeTemplate->AutowireNewNode(FromPin);
	ResultNode = NodeTemplate;

	return ResultNode;
}

UEdGraphNode* FMissionSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode;
	if (FromPins.Num() > 0)
	{
		ResultNode = PerformAction(ParentGraph, FromPins[0], Location);

		// Try auto-wiring the rest of the pins
		for (int32 Index = 1; Index < FromPins.Num(); ++Index)
		{
			ResultNode->AutowireNewNode(FromPins[Index]);
		}
	}
	else
	{
		ResultNode = PerformAction(ParentGraph, nullptr, Location, bSelectNewNode);
	}

	return ResultNode;
}




UEdGraphNode* FMissionGraphSchemaAction_AutoArrange::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UPDMissionGraph* Graph = Cast<UPDMissionGraph>(ParentGraph);
	if (Graph != nullptr && Graph->Nodes.IsEmpty() == false)
	{
		MissionAutoArrangeHelpers::FNodeBoundsInfo BBoxTree;
		MissionAutoArrangeHelpers::GetNodeSizeInfo(static_cast<UPDMissionGraphNode*>(Graph->Nodes[0]), BBoxTree);
		MissionAutoArrangeHelpers::AutoArrangeNodes(static_cast<UPDMissionGraphNode*>(Graph->Nodes[0]), BBoxTree, 0, Graph->Nodes[0]->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
	}
	return nullptr;
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