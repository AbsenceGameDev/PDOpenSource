/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "MissionGraph/PDMissionGraphSchema.h"
#include "MissionGraph/PDMissionGraphNode.h"
#include "PDMissionGraphTypes.h"

#include "EdGraph/EdGraph.h"
#include "GraphEditorActions.h"
#include "ToolMenu.h"
#include "ScopedTransaction.h"
#include "ToolMenuSection.h"


// Copyright Epic Games, Inc. All Rights Reserved.

#include "Textures/SlateIcon.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Settings/EditorStyleSettings.h"
#include "EdGraph/EdGraph.h"
#include "ScopedTransaction.h"
#include "Framework/Commands/GenericCommands.h"
#include "EdGraphNode_Comment.h"
#include "Engine/UserDefinedStruct.h"
#include "MissionGraph/PDMissionGraph.h"

#define LOCTEXT_NAMESPACE "MissionGraph"

namespace
{
	// Maximum distance a drag can be off a node edge to require 'push off' from node
	constexpr int32 NodeDistance = 60;
}

//////////////////////////////////////////////////////////////////////////

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

UEdGraphNode* FMissionSchemaAction_NewNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UEdGraphNode* ResultNode = nullptr;

	// If there is a template, we actually use it
	if (NodeTemplate != nullptr)
	{
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

		UE_LOG(LogTemp, Warning, TEXT("FMissionSchemaAction_NewNode::PerformAction (single pin) : Success"))
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FMissionSchemaAction_NewNode::PerformAction (single pin) : fail"))
	}

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
		UE_LOG(LogTemp, Warning, TEXT("FMissionSchemaAction_NewNode::PerformAction (multi pin) : Success"))
	}
	else
	{
		ResultNode = PerformAction(ParentGraph, nullptr, Location, bSelectNewNode);
		UE_LOG(LogTemp, Warning, TEXT("FMissionSchemaAction_NewNode::PerformAction (multi pin) : fail"))
	}

	return ResultNode;
}

void FMissionSchemaAction_NewNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject(NodeTemplate);
}

UEdGraphNode* FMissionSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	if (ParentNode == nullptr) { return nullptr; }
	
	ParentNode->AddSubNode(NodeTemplate, ParentGraph);
	return nullptr;
}

UEdGraphNode* FMissionSchemaAction_NewSubNode::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode)
{
	return PerformAction(ParentGraph, nullptr, Location, bSelectNewNode);
}

void FMissionSchemaAction_NewSubNode::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject(NodeTemplate);
	Collector.AddReferencedObject(ParentNode);
}

//////////////////////////////////////////////////////////////////////////

UPDMissionGraphSchema::UPDMissionGraphSchema(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

TSharedPtr<FMissionSchemaAction_NewNode> UPDMissionGraphSchema::AddNewNodeAction(FGraphActionListBuilderBase& ContextMenuBuilder, const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FMissionSchemaAction_NewNode> NewAction = MakeShared<FMissionSchemaAction_NewNode>(Category, MenuDesc, Tooltip, 0);
	ContextMenuBuilder.AddAction(NewAction);

	return NewAction;
}

TSharedPtr<FMissionSchemaAction_NewSubNode> UPDMissionGraphSchema::AddNewSubNodeAction(FGraphActionListBuilderBase& ContextMenuBuilder, const FText& Category, const FText& MenuDesc, const FText& Tooltip)
{
	TSharedPtr<FMissionSchemaAction_NewSubNode> NewAction = MakeShared<FMissionSchemaAction_NewSubNode>(Category, MenuDesc, Tooltip, 0);
	ContextMenuBuilder.AddAction(NewAction);
	return NewAction;
}

void UPDMissionGraphSchema::GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder, int32 SubNodeFlags) const
{
	UEdGraph* Graph = const_cast<UEdGraph*>(ContextMenuBuilder.CurrentGraph);
	UClass* GraphNodeClass = nullptr;
	TArray<FPDMissionNodeData> NodeClasses;
	GetSubNodeClasses(SubNodeFlags, NodeClasses, GraphNodeClass);

	if (GraphNodeClass)
	{
		for (const auto& NodeClass : NodeClasses)
		{
			const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodeClass.ToString(), false));

			UPDMissionGraphNode* OpNode = NewObject<UPDMissionGraphNode>(Graph, GraphNodeClass);
			OpNode->ClassData = NodeClass;

			const TSharedPtr<FMissionSchemaAction_NewSubNode> AddOpAction = UPDMissionGraphSchema::AddNewSubNodeAction(ContextMenuBuilder, NodeClass.GetCategory(), NodeTypeName, NodeClass.GetTooltip());
			AddOpAction->ParentNode = Cast<UPDMissionGraphNode>(ContextMenuBuilder.SelectedObjects[0]);
			AddOpAction->NodeTemplate = OpNode;
		}
	}
}

void UPDMissionGraphSchema::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Node && Context->Pin == nullptr)
	{
		const UPDMissionGraphNode* MissionGraphNode = Cast<const UPDMissionGraphNode>(Context->Node);
		if (MissionGraphNode && MissionGraphNode->CanPlaceBreakpoints())
		{
			FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaBreakpoints", LOCTEXT("BreakpointsHeader", "Breakpoints"));
			Section.AddMenuEntry(FGraphEditorCommands::Get().ToggleBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().AddBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().RemoveBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().EnableBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().DisableBreakpoint);
		}
	}
	
	if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("BehaviorTreeGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

void UPDMissionGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

	Super::BreakNodeLinks(TargetNode);
}

void UPDMissionGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

void UPDMissionGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

FLinearColor UPDMissionGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

bool UPDMissionGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	check(Pin != NULL);
	return Pin->bDefaultValueIsIgnored;
}

class FConnectionDrawingPolicy* UPDMissionGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FPDMissionGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

TSharedPtr<FEdGraphSchemaAction> UPDMissionGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FMissionSchemaAction_AddComment));
}

int32 UPDMissionGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	if (Graph)
	{
		const TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(Graph);
		if (GraphEditorPtr.IsValid())
		{
			return GraphEditorPtr->GetNumberOfSelectedNodes();
		}
	}

	return 0;
}

#undef LOCTEXT_NAMESPACE



#include UE_INLINE_GENERATED_CPP_BY_NAME(PDMissionGraphSchema)

#define LOCTEXT_NAMESPACE "MissionEditor"

//////////////////////////////////////////////////////////////////////
//



struct FPDCompareNodeXLocation
{
	FORCEINLINE bool operator()(const UEdGraphPin& A, const UEdGraphPin& B) const
	{
		const UEdGraphNode* NodeA = A.GetOwningNode();
		const UEdGraphNode* NodeB = B.GetOwningNode();

		if (NodeA->NodePosX == NodeB->NodePosX)
		{
			return NodeA->NodePosY < NodeB->NodePosY;
		}

		return NodeA->NodePosX < NodeB->NodePosX;
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
			UPDMissionGraphNode* BTNode = Cast<UPDMissionGraphNode>(Graph->Nodes[Index]);
			if (BTNode)
			{
				BTNode->bIsSubNode = true;
			}
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
		if (Pin)
		{
			SGraphNode::FNodeSet NodeFilter;
			TArray<UEdGraphPin*> TempLinkedTo = Pin->LinkedTo;
			for (int32 Idx = 0; Idx < TempLinkedTo.Num(); Idx++)
			{
				UPDMissionGraphNode* GraphNode = Cast<UPDMissionGraphNode>(TempLinkedTo[Idx]->GetOwningNode());
				if (GraphNode && BBoxTree.Children.Num() > 0)
				{
					AutoArrangeNodes(GraphNode, BBoxTree.Children[BBoxIndex], PosX, PosY + GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().Y * 2.5f);
					GraphNode->DEPRECATED_NodeWidget.Pin()->MoveTo(FDeprecateSlateVector2D(BBoxTree.Children[BBoxIndex].SubGraphBBox.X / 2.f - GraphNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize().X / 2.f + PosX, PosY), NodeFilter);
					PosX += BBoxTree.Children[BBoxIndex].SubGraphBBox.X + 20.f;
					BBoxIndex++;
				}
			}
		}
	}

	void GetNodeSizeInfo(UPDMissionGraphNode* ParentNode, FNodeBoundsInfo& BBoxTree)
	{
		BBoxTree.SubGraphBBox = ParentNode->DEPRECATED_NodeWidget.Pin()->GetDesiredSize();
		float LevelWidth = 0;
		float LevelHeight = 0;

		UEdGraphPin* Pin = MissionGraphHelpers::FindGraphNodePin(ParentNode, EGPD_Output);
		if (Pin)
		{
			Pin->LinkedTo.Sort(FPDCompareNodeXLocation());
			for (int32 Idx = 0; Idx < Pin->LinkedTo.Num(); Idx++)
			{
				UPDMissionGraphNode* GraphNode = Cast<UPDMissionGraphNode>(Pin->LinkedTo[Idx]->GetOwningNode());
				if (GraphNode)
				{
					const int32 ChildIdx = BBoxTree.Children.Add(FNodeBoundsInfo());
					FNodeBoundsInfo& ChildBounds = BBoxTree.Children[ChildIdx];

					GetNodeSizeInfo(GraphNode, ChildBounds);

					LevelWidth += ChildBounds.SubGraphBBox.X + 20.f;
					if (ChildBounds.SubGraphBBox.Y > LevelHeight)
					{
						LevelHeight = ChildBounds.SubGraphBBox.Y;
					}
				}
			}

			if (LevelWidth > BBoxTree.SubGraphBBox.X)
			{
				BBoxTree.SubGraphBBox.X = LevelWidth;
			}

			BBoxTree.SubGraphBBox.Y += LevelHeight;
		}
	}
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

//////////////////////////////////////////////////////////////////////
// UPDMissionGraphSchema

int32 UPDMissionGraphSchema::CurrentCacheRefreshID = 0;


void UPDMissionGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	// @todo Add an entry point by default
	// 	FGraphNodeCreator<UPDMissionGraphNode_EntryPoint> NodeCreator(Graph);
	// 	UPDMissionGraphNode_EntryPoint* MyNode = NodeCreator.CreateNode();
	// 	NodeCreator.Finalize();
	// 	SetNodeMetaData(MyNode, FNodeMetadata::DefaultGraphNode);
}

void UPDMissionGraphSchema::GetSubNodeClasses(int32 SubNodeFlags, TArray<FPDMissionNodeData>& ClassData, UClass*& GraphNodeClass) const
{
	// @todo. Make actual subnode classes, currenlty these are actually the main nodes
	// TArray<FPDMissionNodeData> TempClassData;
	// switch (static_cast<EMissionGraphSubNodeType>(SubNodeFlags))
	// {
	// case EMissionGraphSubNodeType::MainQuest:
	// 	// @todo Gather row data, replace -> 
	// 	GraphNodeClass = UPDMissionGraphNode_MainQuest::StaticClass();
	// 	break;
	// case EMissionGraphSubNodeType::SideQuest:
	// 	// @todo Gather row data, replace -> 
	// 	GraphNodeClass = UPDMissionGraphNode_SideQuest::StaticClass();
	// 	break;
	// case EMissionGraphSubNodeType::EventQuest:
	// 	// @todo Gather row data, replace -> 
	// 	GraphNodeClass = UPDMissionGraphNode_EventQuest::StaticClass();
	// 	break;
	// default:
	// 	break;
	// }
	// ClassData.AddUnique(FPDMissionNodeData{FPDMissionRow::StaticStruct(), ""});
}

void UPDMissionGraphSchema::AddMissionNodeOptions(const FString& CategoryName, FGraphContextMenuBuilder& ContextMenuBuilder, const FPDMissionNodeHandle& NodeData) const
{
	struct FPrivateNodeSelector // @todo move out of here later
	{
		static UClass* Op(UClass* TestClass)
		{
			if (TestClass->IsChildOf(UPDMissionGraphNode_MainQuest::StaticClass()))  { return UPDMissionGraphNode_MainQuest::StaticClass(); }
			if (TestClass->IsChildOf(UPDMissionGraphNode_SideQuest::StaticClass()))  { return UPDMissionGraphNode_SideQuest::StaticClass(); }
			if (TestClass->IsChildOf(UPDMissionGraphNode_EventQuest::StaticClass())) { return UPDMissionGraphNode_EventQuest::StaticClass(); }
			if (TestClass->IsChildOf(UPDMissionGraphNode_Knot::StaticClass()))       { return UPDMissionGraphNode_Knot::StaticClass(); }
			if (TestClass->IsChildOf(UPDMissionGraphNode_EntryPoint::StaticClass())) { return UPDMissionGraphNode_EntryPoint::StaticClass(); }
			return UPDMissionGraphNode_EntryPoint::StaticClass(); // fallback
		}
	};
	
	// @todo fix after initial test
	FCategorizedGraphActionListBuilder ListBuilder(CategoryName);
	
	TMap<UClass*, FPDMissionNodeData> NodeClasses;
	// gather all native classes
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* TestClass = *It;
		if (TestClass->HasAnyClassFlags(CLASS_Native) && TestClass->IsChildOf(UPDMissionGraphNode::StaticClass()))
		{
			FPDMissionNodeData NodeClass(TestClass, "");
			NodeClass.bHideParent = false;
			NodeClass.bIsHidden = false;
			NodeClasses.Emplace(FPrivateNodeSelector::Op(TestClass),NodeClass);
		}
	}

	for (const TPair<UClass*, FPDMissionNodeData>& NodePair : NodeClasses)
	{
		const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodePair.Value.ToString(), false));
		TSharedPtr<FMissionSchemaAction_NewNode> AddOpAction = UPDMissionGraphSchema::AddNewNodeAction(ListBuilder, NodePair.Value.GetCategory(), NodeTypeName, FText::GetEmpty());
		UPDMissionGraphNode* OpNode = NewObject<UPDMissionGraphNode>(ContextMenuBuilder.OwnerOfTemporaries, NodePair.Key);
		OpNode->ClassData = NodePair.Value;
		AddOpAction->NodeTemplate = OpNode;
	}

	ContextMenuBuilder.Append(ListBuilder);
}

void UPDMissionGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// @todo Use PinCategory
	const FName PinCategory = ContextMenuBuilder.FromPin ?
		ContextMenuBuilder.FromPin->PinType.PinCategory :
		FPDMissionGraphTypes::PinCategory_MultipleNodes;

	const bool bNoParent = (ContextMenuBuilder.FromPin == nullptr);

	if (bNoParent || (ContextMenuBuilder.FromPin && (ContextMenuBuilder.FromPin->Direction == EGPD_Input)))
	{
		const FPDMissionNodeHandle DummyData; // @todo replace dummy with actual data
		AddMissionNodeOptions(TEXT("Entry Point"), ContextMenuBuilder, DummyData);
	}

	// @todo Use PinCategory
	if (bNoParent)
	{
		const TSharedPtr<FMissionGraphSchemaAction_AutoArrange> Action1( new FMissionGraphSchemaAction_AutoArrange(FText::GetEmpty(), LOCTEXT("AutoArrange", "Auto Arrange"), FText::GetEmpty(), 0) );
		const TSharedPtr<FMissionSchemaAction_NewNode> Action2( new FMissionSchemaAction_NewNode(FText::GetEmpty(), LOCTEXT("NewNode", "New node"), FText::GetEmpty(), 0));
		const TSharedPtr<FMissionSchemaAction_NewSubNode> Action3( new FMissionSchemaAction_NewSubNode(FText::GetEmpty(), LOCTEXT("NewSubNode", "New sub node"), FText::GetEmpty(), 0) );
		const TSharedPtr<FMissionSchemaAction_AddComment> Action4( new FMissionSchemaAction_AddComment(LOCTEXT("AddComment", "Add new comment"), LOCTEXT("AddComment", "Add new comment")));
		ContextMenuBuilder.AddAction(Action1);
		ContextMenuBuilder.AddAction(Action2);
		ContextMenuBuilder.AddAction(Action3);
		ContextMenuBuilder.AddAction(Action4);
	}
}

const FPinConnectionResponse UPDMissionGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	if (PinA == nullptr || PinB == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinNull", "One or Both of the pins was null"));
	}

	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Both are on the same node"));
	}

	const bool bPinAIsSingleComposite = (PinA->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleComposite);
	const bool bPinAIsSingleTask = (PinA->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleTask);
	const bool bPinAIsSingleNode = (PinA->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleNode);

	const bool bPinBIsSingleComposite = (PinB->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleComposite);
	const bool bPinBIsSingleTask = (PinB->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleTask);
	const bool bPinBIsSingleNode = (PinB->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SingleNode);

	// Compare the directions
	if ((PinA->Direction == EGPD_Input) && (PinB->Direction == EGPD_Input))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorInput", "Can't connect input node to input node"));
	}
	else if ((PinB->Direction == EGPD_Output) && (PinA->Direction == EGPD_Output))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOutput", "Can't connect output node to output node"));
	}

	class FNodeVisitorCycleChecker
	{
	public:
		/** Check whether a loop in the graph would be caused by linking the passed-in nodes */
		bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
		{
			VisitedNodes.Add(EndNode);
			return TraverseInputNodesToRoot(StartNode);
		}

	private:
		/**
		 * Helper function for CheckForLoop()
		 * @param	Node	The node to start traversal at
		 * @return true if we reached a root node (i.e. a node with no input pins), false if we encounter a node we have already seen
		 */
		bool TraverseInputNodesToRoot(UEdGraphNode* Node)
		{
			VisitedNodes.Add(Node);

			// Follow every input pin until we cant any more ('root') or we reach a node we have seen (cycle)
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				UEdGraphPin* MyPin = Node->Pins[PinIndex];

				if (MyPin->Direction != EGPD_Input) { continue; }
				
				for (int32 LinkedPinIndex = 0; LinkedPinIndex < MyPin->LinkedTo.Num(); ++LinkedPinIndex)
				{
					UEdGraphPin* OtherPin = MyPin->LinkedTo[LinkedPinIndex];
					if (OtherPin == nullptr) { continue; }
					
					UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
					return VisitedNodes.Contains(OtherNode) ? false : TraverseInputNodesToRoot(OtherNode);
				}
			}

			return true;
		}

		TSet<UEdGraphNode*> VisitedNodes;
	};

	// check for cycles
	FNodeVisitorCycleChecker CycleChecker;
	if (!CycleChecker.CheckForLoop(PinA->GetOwningNode(), PinB->GetOwningNode()))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorcycle", "Can't create a graph cycle"));
	}

	const bool bPinASingleLink = bPinAIsSingleComposite || bPinAIsSingleTask || bPinAIsSingleNode;
	const bool bPinBSingleLink = bPinBIsSingleComposite || bPinBIsSingleTask || bPinBIsSingleNode;

	if (PinB->Direction == EGPD_Input && PinB->LinkedTo.Num() > 0)
	{
		if (bPinASingleLink)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("PinConnectReplace", "Replace connection"));
		}

		// @todo
		//return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("PinConnectReplace", "Replace connection"));
	}
	else if (PinA->Direction == EGPD_Input && PinA->LinkedTo.Num() > 0)
	{
		if (bPinBSingleLink)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("PinConnectReplace", "Replace connection"));
		}

		// @todo
		//return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectReplace", "Replace connection"));
	}

	if (bPinASingleLink && PinA->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectReplace", "Replace connection"));
	}
	else if (bPinBSingleLink && PinB->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("PinConnectReplace", "Replace connection"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
}

const FPinConnectionResponse UPDMissionGraphSchema::CanMergeNodes(const UEdGraphNode* NodeA, const UEdGraphNode* NodeB) const
{
	// Make sure the nodes are not the same 
	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));
	}

	const bool bIsSubnode_A = Cast<UPDMissionGraphNode>(NodeA) && Cast<UPDMissionGraphNode>(NodeA)->IsSubNode();
	const bool bIsSubnode_B = Cast<UPDMissionGraphNode>(NodeB) && Cast<UPDMissionGraphNode>(NodeB)->IsSubNode();
	// const bool bIsTask_B = NodeB->IsA(UPDMissionGraphNode_Task::StaticClass());

	if (bIsSubnode_A && (bIsSubnode_B)) // || bIsTask_B))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

void UPDMissionGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateRerouteNodeOnWire", "Create Reroute Node"));

	//@TODO: This constant is duplicated from inside of SGraphNodeKnot
	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	// Create a new knot
	UEdGraph* OwningGraph = PinA->GetOwningNode()->GetGraph();
	
	if (ensure(OwningGraph))
	{
		FGraphNodeCreator<UPDMissionGraphNode_Knot> NodeCreator(*OwningGraph);
		UPDMissionGraphNode_Knot* MyNode = NodeCreator.CreateNode();
		MyNode->NodePosX = KnotTopLeft.X;
		MyNode->NodePosY = KnotTopLeft.Y;
		//MyNode->SnapToGrid(SNAP_GRID);
	 	NodeCreator.Finalize();
	
		//UK2Node_Knot* NewKnot = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_Knot>(ParentGraph, KnotTopLeft, EK2NewNodeFlags::SelectNewNode);
	
		// Move the connections across (only notifying the knot, as the other two didn't really change)
		PinA->BreakLinkTo(PinB);
		PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetInputPin() : CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetOutputPin());
		PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetInputPin() : CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetOutputPin());
	}
}




bool UPDMissionGraphSchema::GetPropertyCategoryInfo(const FProperty* TestProperty, FName& OutCategory, FName& OutSubCategory, UObject*& OutSubCategoryObject, bool& bOutIsWeakPointer)
{
	if (const FStructProperty* StructProperty = CastField<const FStructProperty>(TestProperty))
	{
		OutCategory = FPDMissionGraphTypes::PinCategory_MissionRow;
		OutSubCategoryObject = StructProperty->Struct;
		// Match IsTypeCompatibleWithProperty and erase REINST_ structs here:
		if(UUserDefinedStruct* UDS = Cast<UUserDefinedStruct>(StructProperty->Struct))
		{
			UUserDefinedStruct* RealStruct = UDS->PrimaryStruct.Get();
			if(RealStruct != nullptr) { OutSubCategoryObject = RealStruct; }
		}
	}
	else if (TestProperty->IsA<FFloatProperty>())
	{
		// @todo
		// OutCategory = PC_Real;
		// OutSubCategory = PC_Float;
	}
	else if (TestProperty->IsA<FDoubleProperty>())
	{
		// @todo
		// OutCategory = PC_Real;
		// OutSubCategory = PC_Double;
	}
	else if (TestProperty->IsA<FInt64Property>())
	{
		// @todo
		// OutCategory = PC_Int64;
	}
	else if (TestProperty->IsA<FIntProperty>())
	{
		// @todo
		// OutCategory = PC_Int;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
	}
	else if (const FByteProperty* ByteProperty = CastField<const FByteProperty>(TestProperty))
	{
		// @todo
		
		// OutCategory = PC_Byte;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
		// else
		// {
		// 	OutSubCategoryObject = ByteProperty->Enum;
		// }
	}
	else if (const FEnumProperty* EnumProperty = CastField<const FEnumProperty>(TestProperty))
	{
		// @todo
		
		
		// // K2 only supports byte enums right now - any violations should have been caught by UHT or the editor
		// if (!EnumProperty->GetUnderlyingProperty()->IsA<FByteProperty>())
		// {
		// 	OutCategory = TEXT("unsupported_enum_type: enum size is larger than a byte");
		// 	return false;
		// }
		//
		// OutCategory = PC_Byte;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
		// else
		// {
		// 	OutSubCategoryObject = EnumProperty->GetEnum();
		// }
	}
	else if (TestProperty->IsA<FNameProperty>())
	{
		// @todo
		// OutCategory = PC_Name;
	}
	else if (TestProperty->IsA<FStrProperty>())
	{
		// @todo
		// OutCategory = PC_String;
	}
	else if (TestProperty->IsA<FTextProperty>())
	{
		// @todo
		// OutCategory = PC_Text;
	}
	else if (const FFieldPathProperty* FieldPathProperty = CastField<const FFieldPathProperty>(TestProperty))
	{
		// @todo
		// OutCategory = PC_FieldPath;
		//OutSubCategoryObject = SoftObjectProperty->PropertyClass; @todo: FProp
	}
	else
	{
		OutCategory = TEXT("bad_type");
		return false;
	}

	return true;
}



bool UPDMissionGraphSchema::ConvertPropertyToPinType(const FProperty* Property, /*out*/ FEdGraphPinType& TypeOut) const
{
	if (Property == nullptr)
	{
		TypeOut.PinCategory = TEXT("bad_type");
		return false;
	}

	TypeOut.PinSubCategory = NAME_None;
	
	// Handle whether or not this is an array property
	const FMapProperty* MapProperty = CastField<const FMapProperty>(Property);
	const FSetProperty* SetProperty = CastField<const FSetProperty>(Property);
	const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(Property);
	const FProperty* TestProperty = Property;
	if (MapProperty)
	{
		TestProperty = MapProperty->KeyProp;

		// set up value property:
		UObject* SubCategoryObject = nullptr;
		bool bIsWeakPtr = false;
		bool bResult = const_cast<UPDMissionGraphSchema*>(this)->GetPropertyCategoryInfo(MapProperty->ValueProp, TypeOut.PinValueType.TerminalCategory,
		                                                                                 TypeOut.PinValueType.TerminalSubCategory, SubCategoryObject, bIsWeakPtr);
		TypeOut.PinValueType.TerminalSubCategoryObject = SubCategoryObject;

		if (bIsWeakPtr)
		{
			return false;
		}

		if (!bResult)
		{
			return false;
		}

		// Ensure that the value term will be identified as a wrapper type if the source property has that flag set.
		if(MapProperty->ValueProp->HasAllPropertyFlags(CPF_UObjectWrapper))
		{
			TypeOut.PinValueType.bTerminalIsUObjectWrapper = true;
		}
	}
	else if (SetProperty)
	{
		TestProperty = SetProperty->ElementProp;
	}
	else if (ArrayProperty)
	{
		TestProperty = ArrayProperty->Inner;
	}
	TypeOut.ContainerType = FEdGraphPinType::ToPinContainerType(ArrayProperty != nullptr, SetProperty != nullptr, MapProperty != nullptr);
	TypeOut.bIsReference = Property->HasAllPropertyFlags(CPF_OutParm|CPF_ReferenceParm);
	TypeOut.bIsConst     = Property->HasAllPropertyFlags(CPF_ConstParm);

	// This flag will be set on the key/inner property for container types, so check the test property.
	TypeOut.bIsUObjectWrapper = TestProperty->HasAllPropertyFlags(CPF_UObjectWrapper);

	// @todo
	// // Check to see if this is the wildcard property for the target container type
	//
	// if(IsWildcardProperty(Property))
	// {
	// 	TypeOut.PinCategory = PC_Wildcard;
	// 	if(MapProperty)
	// 	{
	// 		TypeOut.PinValueType.TerminalCategory = PC_Wildcard;
	// 	}
	// }
	// else
	// {
	// 	UObject* SubCategoryObject = nullptr;
	// 	bool bIsWeakPointer = false;
	// 	bool bResult = GetPropertyCategoryInfo(TestProperty, TypeOut.PinCategory, TypeOut.PinSubCategory, SubCategoryObject, bIsWeakPointer);
	// 	TypeOut.bIsWeakPointer = bIsWeakPointer;
	// 	TypeOut.PinSubCategoryObject = SubCategoryObject;
	// 	if (!bResult)
	// 	{
	// 		return false;
	// 	}
	// }
	//
	// if (TypeOut.PinSubCategory == PSC_Bitmask)
	// {
	// 	const FString& BitmaskEnumName = TestProperty->GetMetaData(TEXT("BitmaskEnum"));
	// 	if(!BitmaskEnumName.IsEmpty())
	// 	{
	// 		// @TODO: Potentially replace this with a serialized UEnum reference on the FProperty (e.g. FByteProperty::Enum)
	// 		TypeOut.PinSubCategoryObject = UClass::TryFindTypeSlow<UEnum>(BitmaskEnumName);
	// 	}
	// }

	return true;
}



bool UPDMissionGraphSchema::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UPDMissionGraphSchema::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UPDMissionGraphSchema::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
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