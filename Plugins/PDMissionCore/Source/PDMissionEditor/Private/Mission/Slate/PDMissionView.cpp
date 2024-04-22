/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Mission/Slate/PDMissionView.h"
#include "Mission/FPDMissionEditor.h"
#include "Mission/PDMissionBuilder.h"
#include "Mission/Graph/PDMissionGraph.h"
#include "Mission/Graph/PDMissionGraphNode.h"

#include <EdGraph/EdGraphPin.h>
#include <EdGraph/EdGraphSchema.h>
#include <Framework/Views/TableViewMetadata.h>
#include <Widgets/Input/STextComboBox.h>
#include <Widgets/Views/STreeView.h>

#include "PDMissionEditor.h"
#include "Subsystems/PDMissionSubsystem.h"


#include "Framework/Application/SlateApplication.h"
#include "Editor.h"
#include "SGraphPanel.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FMissionTreeNode"

//
// FMissionTreeNode
FMissionTreeNode::FMissionTreeNode(UEdGraphNode* InNode, const TSharedPtr<const FMissionTreeNode>& InParent)
	: ParentPtr(InParent) , GraphNodePtr(InNode) { }

TSharedRef<SWidget> FMissionTreeNode::CreateIcon()
{
	return SNew(SImage)
		.Image(FAppStyle::GetBrush(TEXT("GraphEditor.FIB_Event")))
		.ColorAndOpacity(FSlateColor::UseForeground());
}

FReply FMissionTreeNode::OnClick(const TWeakPtr<FFPDMissionGraphEditor>& MissionEditorPtr) const
{
	const UEdGraphNode* GraphNode = GetGraphNode();
	if (MissionEditorPtr.IsValid() == false || GraphNode == nullptr) { return  FReply::Handled(); }
	
	return MissionEditorPtr.Pin()->DebugHandler.JumpToNode(GraphNode);
}

FText FMissionTreeNode::GetNodeTypeText() const
{
	const UEdGraphNode* GraphNode = GetGraphNode();
	if (GraphNode == nullptr) { return FText::GetEmpty(); }

	return GraphNode->GetClass()->GetDisplayNameText();
}

FString FMissionTreeNode::GetCommentText() const
{
	const UEdGraphNode* GraphNode = GetGraphNode();
	if (GraphNode == nullptr) { return FString(); }

	return GraphNode->NodeComment;
}

FText FMissionTreeNode::GetText() const
{
	const UEdGraphNode* GraphNode = GetGraphNode();
	if (GraphNode == nullptr) { return FText::GetEmpty(); }
	
	return GraphNode->GetNodeTitle(ENodeTitleType::ListView);
}

const TArray< TSharedPtr<FMissionTreeNode> >& FMissionTreeNode::GetChildren() const
{
	const UPDMissionGraphNode* MissionNode = Cast<UPDMissionGraphNode>(GetGraphNode());
	if (bChildrenDirty == false || MissionNode == nullptr) { return Children; }
	bChildrenDirty = false;
	
	for (UPDMissionGraphNode* SubNode : MissionNode->SubNodes)
	{
		Children.Add(MakeShared<FMissionTreeNode>(SubNode, SharedThis(this)));
	}

	UEdGraphPin* OutputPin = MissionNode->GetOutputPin();
	if (OutputPin == nullptr) { return Children; }
	
	for (const UEdGraphPin* LinkedToPin : OutputPin->LinkedTo)
	{
		Children.Add(MakeShared<FMissionTreeNode>(LinkedToPin->GetOwningNode(), SharedThis(this)));
	}

	return Children;
}

//
// SMissionTreeEditor
void SMissionTreeEditor::Construct( const FArguments& InArgs, const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditor)
{
	MissionEditorPtr = InMissionEditor;
	InMissionEditor->FocusedGraphEditorChanged.AddSP(this, &SMissionTreeEditor::OnFocusedGraphChanged);

	// @todo Use MissionData handle and build a graph here
	FPDMissionNodeHandle& MissionData = InMissionEditor->GetMissionData();

	BuildTree();
	ChildSlot
	[
		SNew(SVerticalBox)

		+SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Menu.Background"))
			[
				SAssignNew(TreeView, STreeViewType)
				.ItemHeight(24)
				.TreeItemsSource(&RootNodes)
				.OnGenerateRow(this, &SMissionTreeEditor::OnGenerateRow)
				.OnGetChildren(this, &SMissionTreeEditor::OnGetChildren)
				.OnSelectionChanged(this, &SMissionTreeEditor::OnTreeSelectionChanged)
				.SelectionMode(ESelectionMode::Multi)
			]
		]
	];
}

void SMissionTreeEditor::OnFocusedGraphChanged()
{
	RefreshTree();
}

void SMissionTreeEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	RefreshTree();
}

void SMissionTreeEditor::RefreshTree()
{
	BuildTree();

	TreeView->RequestTreeRefresh();

	for (auto It(RootNodes.CreateIterator()); It; ++It)
	{
		TreeView->SetItemExpansion(*It, true);
	}
}

void SMissionTreeEditor::BuildTree()
{
	RootNodes.Empty();

	const TWeakPtr<SGraphEditor> FocusedGraphEditor = MissionEditorPtr.Pin()->GetFocusedGraphPtr();
	UEdGraph* Graph = nullptr;
	if (FocusedGraphEditor.IsValid())
	{
		Graph = FocusedGraphEditor.Pin()->GetCurrentGraph();
	}

	if (Graph == nullptr)
	{
		return;
	}

	// @todo no entry points are created, coming back to in a couple of days
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		UPDMissionGraphNode_EntryPoint* RootNode = Cast<UPDMissionGraphNode_EntryPoint>(Node);
		if (RootNode == nullptr) { continue; }

		RootNodes.Add(MakeShared<FMissionTreeNode>(RootNode, TSharedPtr<FMissionTreeNode>()));
	}
}

TSharedRef<ITableRow> SMissionTreeEditor::OnGenerateRow(TSharedPtr<FMissionTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow< TSharedPtr<FMissionTreeNode> >, OwnerTable)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(450.0f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						InItem->CreateIcon()
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoWidth()
					.Padding(2, 0)
					[
						SNew(STextBlock)
						.Text(InItem->GetText())
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(InItem->GetNodeTypeText())
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetCommentText()))
				.ColorAndOpacity(FLinearColor::Yellow)
			]
		];
}

void SMissionTreeEditor::OnGetChildren(TSharedPtr<FMissionTreeNode> InItem, TArray< TSharedPtr<FMissionTreeNode> >& OutChildren)
{
	OutChildren.Append(InItem->GetChildren());
}

void SMissionTreeEditor::OnTreeSelectionChanged(TSharedPtr<FMissionTreeNode> Item, ESelectInfo::Type)
{
	if (Item.IsValid() == false) { return; }

	Item->OnClick(MissionEditorPtr);
}


//  NODES

TSharedRef<FDragMissionGraphNode> FDragMissionGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
	const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragMissionGraphNode> Operation = MakeShareable(new FDragMissionGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragMissionGraphNode> FDragMissionGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
	const TArray<TSharedRef<SGraphNode>>& InDraggedNodes)
{
	TSharedRef<FDragMissionGraphNode> Operation = MakeShareable(new FDragMissionGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UPDMissionGraphNode* FDragMissionGraphNode::GetDropTargetNode() const
{
	return Cast<UPDMissionGraphNode>(GetHoveredNode());
}

void SMissionGraphNode::Construct(const FArguments& InArgs, UPDMissionGraphNode* InNode)
{
	SetCursor(EMouseCursor::CardinalCross);

	GraphNode = InNode;
	UpdateGraphNode();

	bDragMarkerVisible = false;	
}

void SMissionGraphNode::AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);
}

FText SMissionGraphNode::GetTitle() const
{
	return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

FText SMissionGraphNode::GetDescription() const
{
	UPDMissionGraphNode* MyNode = CastChecked<UPDMissionGraphNode>(GraphNode);
	return MyNode ? MyNode->GetDescription() : FText::GetEmpty();
}

EVisibility SMissionGraphNode::GetDescriptionVisibility() const
{
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMissionGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(4.0f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else // Direction == EEdGraphPinDirection::EGPD_Output
	{
		RightNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(4.0f)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}

FReply SMissionGraphNode::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		//if we are holding mouse over a subnode
		UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (TestNode && TestNode->IsSubNode())
		{
			const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragMissionGraphNode::New(Panel, Node));
		}
	}

	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bDragMarkerVisible)
	{
		SetDragMarker(false);
	}

	return FReply::Unhandled();
}

FReply SMissionGraphNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedPtr<SGraphNode> SMissionGraphNode::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> ResultNode;

	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > SubWidgetsSet;
	for (int32 i = 0; i < SubNodes.Num(); i++)
	{
		SubWidgetsSet.Add(SubNodes[i].ToSharedRef());
	}

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(WidgetGeometry, SubWidgetsSet, Result);

	if (Result.Num() > 0)
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray(ArrangedChildren.GetInternalArray());
		
		const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
		if (HoveredIndex != INDEX_NONE)
		{
			ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredIndex].Widget);
		}
	}

	return ResultNode;
}

void SMissionGraphNode::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SMissionGraphNode::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMissionGraphNode::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));

		UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (DragConnectionOp->IsValidOperation() && TestNode && TestNode->IsSubNode())
		{
			SetDragMarker(true);
		}
	}

	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SMissionGraphNode::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));
	}
	return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SMissionGraphNode::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		DragConnectionOp->SetHoveredNode(TSharedPtr<SGraphNode>(NULL));
	}

	SetDragMarker(false);
	SGraphNode::OnDragLeave(DragDropEvent);
}

FReply SMissionGraphNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetDragMarker(false);

	TSharedPtr<FDragMissionGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragMissionGraphNode>();
	if (DragNodeOp.IsValid())
	{
		if (!DragNodeOp->IsValidOperation())
		{
			return FReply::Handled();
		}

		const float DragTime = float(FPlatformTime::Seconds() - DragNodeOp->StartTime);
		if (DragTime < 0.5f)
		{
			return FReply::Handled();
		}

		UPDMissionGraphNode* MyNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (MyNode == nullptr || MyNode->IsSubNode())
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_DragDropNode", "Drag&Drop Node"));
		bool bReorderOperation = true;

		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UPDMissionGraphNode* DraggedNode = Cast<UPDMissionGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			if (DraggedNode && DraggedNode->ParentNode)
			{
				if (DraggedNode->ParentNode != GraphNode)
				{
					bReorderOperation = false;
				}

				DraggedNode->ParentNode->RemoveSubNode(DraggedNode);
			}
		}

		UPDMissionGraphNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		const int32 InsertIndex = MyNode->FindSubNodeDropIndex(DropTargetNode);

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UPDMissionGraphNode* DraggedTestNode = Cast<UPDMissionGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			DraggedTestNode->Modify();
			DraggedTestNode->ParentNode = MyNode;

			MyNode->Modify();
			MyNode->InsertSubNodeAt(DraggedTestNode, InsertIndex);
		}

		if (bReorderOperation)
		{
			UpdateGraphNode();
		}
		else
		{
			UPDMissionGraph* MyGraph = Cast<UPDMissionGraph>(MyNode->GetGraph());
			if (MyGraph)
			{
				MyGraph->OnSubNodeDropped();
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}

TSharedPtr<SToolTip> SMissionGraphNode::GetComplexTooltip()
{
	return NULL;
}

FText SMissionGraphNode::GetPreviewCornerText() const
{
	return FText::GetEmpty();
}

const FSlateBrush* SMissionGraphNode::GetNameIcon() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

void SMissionGraphNode::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
{
	SGraphNode::SetOwner(OwnerPanel);

	for (auto& ChildWidget : SubNodes)
	{
		if (ChildWidget.IsValid())
		{
			ChildWidget->SetOwner(OwnerPanel);
			OwnerPanel->AttachGraphEvents(ChildWidget);
		}
	}
}

//
// Mission Transition Node(s)

void SGraphNodeMissionTransition::Construct(const FArguments& InArgs, UPDMissionTransitionNode* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

void SGraphNodeMissionTransition::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
}

void SGraphNodeMissionTransition::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	// Ignored; position is set by the location of the attached state nodes
}

bool SGraphNodeMissionTransition::RequiresSecondPassLayout() const
{
	return true;
}

void SGraphNodeMissionTransition::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	// Find the geometry of the state nodes we're connecting
	FGeometry StartGeom;
	FGeometry EndGeom;

	int32 TransIndex = 0;
	int32 NumOfTrans = 1;

	UPDMissionGraphNode* PrevState = TransNode->GetOwningMission();
	UPDMissionGraphNode* NextState = TransNode->GetTargetMission();
	if ((PrevState != NULL) && (NextState != NULL))
	{
		const TSharedRef<SNode>* pPrevNodeWidget = NodeToWidgetLookup.Find(PrevState);
		const TSharedRef<SNode>* pNextNodeWidget = NodeToWidgetLookup.Find(NextState);
		if ((pPrevNodeWidget != NULL) && (pNextNodeWidget != NULL))
		{
			const TSharedRef<SNode>& PrevNodeWidget = *pPrevNodeWidget;
			const TSharedRef<SNode>& NextNodeWidget = *pNextNodeWidget;

			StartGeom = FGeometry(FVector2D(PrevState->NodePosX, PrevState->NodePosY), FVector2D::ZeroVector, PrevNodeWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(NextState->NodePosX, NextState->NodePosY), FVector2D::ZeroVector, NextNodeWidget->GetDesiredSize(), 1.0f);

			TArray<UPDMissionTransitionNode*> Transitions;
			PrevState->GetMissionTransitions(Transitions);

			Transitions = Transitions.FilterByPredicate([NextState](const UPDMissionTransitionNode* InTransition) -> bool
			{
				return InTransition->GetTargetMission() == NextState;
			});

			TransIndex = Transitions.IndexOfByKey(TransNode);
			NumOfTrans = Transitions.Num();

			PrevStateNodeWidgetPtr = PrevNodeWidget;
		}
	}

	//Position Node
	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, TransIndex, NumOfTrans);
}

TSharedRef<SWidget> SGraphNodeMissionTransition::GenerateRichTooltip()
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	//
	// Generate tooltip test in a vertical widget box, for the transition wdget
	UEdGraphPin* CanExecPin = NULL;
	TSharedRef<SVerticalBox> Widget = SNew(SVerticalBox);
	const FText TooltipDesc = GetPreviewCornerText(false);
	
	// Transition rule linearized
	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew(STextBlock)
			.TextStyle( FAppStyle::Get(), TEXT("Graph.TransitionNode.TooltipName") )
			.Text(TooltipDesc)
		];
	
	Widget->AddSlot()
	.AutoHeight()
	.Padding( 2.0f )
	[
		SNew(STextBlock)
		.TextStyle( FAppStyle::Get(), TEXT("Graph.TransitionNode.TooltipRule") )
		.Text(LOCTEXT("AnimGraphNodeTransitionRule_ToolTip", "Transition Rule (in words)"))
	];
	
	// Widget->AddSlot()
	// 	.AutoHeight()
	// 	.Padding( 2.0f )
	// 	[
	// 		SNew(SKismetLinearExpression, CanExecPin)
	// 	];

	return Widget;
}

TSharedPtr<SToolTip> SGraphNodeMissionTransition::GetComplexTooltip()
{
	return SNew(SToolTip)
		[
			GenerateRichTooltip()
		];
}

void SGraphNodeMissionTransition::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill") )
				.ColorAndOpacity( this, &SGraphNodeMissionTransition::GetTransitionColor )
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( this, &SGraphNodeMissionTransition::GetTransitionIconImage )
			]
		];
}

FText SGraphNodeMissionTransition::GetPreviewCornerText(bool bReverse) const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	UPDMissionGraphNode* PrevState = (bReverse ? TransNode->GetTargetMission() : TransNode->GetOwningMission());
	UPDMissionGraphNode* NextState = (bReverse ? TransNode->GetOwningMission() : TransNode->GetTargetMission());

	FText Result = LOCTEXT("BadTransition", "Bad transition (missing source or target)");

	// Show the priority if there is any ambiguity
	if (PrevState != NULL)
	{
		if (NextState != NULL)
		{
			TArray<UPDMissionTransitionNode*> TransitionFromSource;
			PrevState->GetMissionTransitions(/*out*/ TransitionFromSource);

			bool bMultiplePriorities = false;
			if (TransitionFromSource.Num() > 1)
			{
				// See if the priorities differ
				for (int32 Index = 0; (Index < TransitionFromSource.Num()) && !bMultiplePriorities; ++Index)
				{
					const bool bDifferentPriority = (TransitionFromSource[Index]->PriorityOrder != TransNode->PriorityOrder);
					bMultiplePriorities |= bDifferentPriority;
				}
			}

			if (bMultiplePriorities)
			{
				Result = FText::Format(LOCTEXT("TransitionXToYWithPriority", "{0} to {1} (Priority {2})"), FText::FromString(PrevState->GetMissionName()), FText::FromString(NextState->GetMissionName()), FText::AsNumber(TransNode->PriorityOrder));
			}
			else
			{
				Result = FText::Format(LOCTEXT("TransitionXToY", "{0} to {1}"), FText::FromString(PrevState->GetMissionName()), FText::FromString(NextState->GetMissionName()));
			}
		}
	}

	return Result;
}

FLinearColor SGraphNodeMissionTransition::StaticGetTransitionColor(UPDMissionTransitionNode* TransNode, bool bIsHovered)
{
	//@TODO: Make configurable by styling
	const FLinearColor ActiveColor(1.0f, 0.4f, 0.3f, 1.0f);
	const FLinearColor HoverColor(0.724f, 0.256f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.9f, 0.9f, 0.9f, 1.0f);
	

	//@TODO: ANIMATION: Sort out how to display this
	// 			if (TransNode->SharedCrossfadeIdx != INDEX_NONE)
	// 			{
	// 				WireColor.R = (TransNode->SharedCrossfadeIdx & 1 ? 1.0f : 0.15f);
	// 				WireColor.G = (TransNode->SharedCrossfadeIdx & 2 ? 1.0f : 0.15f);
	// 				WireColor.B = (TransNode->SharedCrossfadeIdx & 4 ? 1.0f : 0.15f);
	// 			}
	

	return bIsHovered ? HoverColor : BaseColor;
}

FSlateColor SGraphNodeMissionTransition::GetTransitionColor() const
{	
	// Highlight the transition node when the node is hovered or when the previous state is hovered
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	return StaticGetTransitionColor(TransNode, (IsHovered() || (PrevStateNodeWidgetPtr.IsValid() && PrevStateNodeWidgetPtr.Pin()->IsHovered())));
}

const FSlateBrush* SGraphNodeMissionTransition::GetTransitionIconImage() const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
}

// TSharedRef<SWidget> SGraphNodeMissionTransition::GenerateInlineDisplayOrEditingWidget(bool bShowGraphPreview)
// {
// }

void SGraphNodeMissionTransition::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->AddPinToHoverSet(Pin);
	}

	SGraphNode::OnMouseEnter(MyGeometry, MouseEvent);
}

void SGraphNodeMissionTransition::OnMouseLeave(const FPointerEvent& MouseEvent)
{	
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->RemovePinFromHoverSet(Pin);
	}

	SGraphNode::OnMouseLeave(MouseEvent);
}

void SGraphNodeMissionTransition::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = static_cast<int32>(NewCorner.X);
	GraphNode->NodePosY = static_cast<int32>(NewCorner.Y);
}



//
// PINS

void SPDMissionGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObject)
{
	this->SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InGraphPinObject;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SPDMissionGraphPin::GetPinBorder)
		.BorderBackgroundColor(this, &SPDMissionGraphPin::GetPinColor)
		.OnMouseButtonDown(this, &SPDMissionGraphPin::OnPinMouseDown)
		.Cursor(this, &SPDMissionGraphPin::GetPinCursor)
		.Padding(FMargin(10.0f))
		);		
}


TSharedRef<SWidget>	SPDMissionGraphPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SPDMissionGraphPin::GetPinBorder() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

FSlateColor SPDMissionGraphPin::GetPinColor() const
{
	return FSlateColor(IsHovered() ? FLinearColor::Yellow : FLinearColor::Black);
}


//
// Row Selector attribute

void SPDAttributePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SPDAttributePin::GetDefaultValueWidget()
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return SNew(STextComboBox); }
	
	return	SNew(STextComboBox) //note you can display any widget here
		.OptionsSource(&MissionSubsystem->Utility.MissionRowNameList) 
		.OnSelectionChanged(this, &SPDAttributePin::OnAttributeSelected);
}
void SPDAttributePin::OnAttributeSelected(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
	const UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	const FPDMissionUtility* Utility = MissionSubsystem != nullptr ? &MissionSubsystem->Utility : nullptr;
	if (Utility == nullptr) { return; }	
	
	const FName SelectedMissionRowName = Utility->IndexToName.FindRef(Utility->MissionRowNameList.Find(ItemSelected));

	switch (SelectInfo)
	{
	case ESelectInfo::OnNavigation:
	case ESelectInfo::Direct:
		return;
	case ESelectInfo::OnKeyPress:
	case ESelectInfo::OnMouseClick:
		break;
	}

	
	// We need to refresh the DataRefPins here, this is the struct view pin that will reflect our selected entry key
	UPDMissionGraphNode* AsMissionGraphNode = OwnerNodePtr.Pin() != nullptr ?
		Cast<UPDMissionGraphNode>(OwnerNodePtr.Pin()->GetNodeObj()) : nullptr;
	if (AsMissionGraphNode != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Calling RefreshDataRefPins(SelectedMissionRowName), SelectedMissionName: %s"), *SelectedMissionRowName.ToString())
		AsMissionGraphNode->RefreshDataRefPins(SelectedMissionRowName);
	}
	

	// Wants to create new mission,
	if (SelectedMissionRowName == "--New Mission Row--")
	{
		// AsMissionGraphNode->ConstructKeyEntryPin_ForNewRowEntry();
		
		// @todo 1. Add save button to the graph and the call the functions I wrote yesterday for saving/loading to and from the editing table
		// @todo 2. We need a hidden pin which becomes visible when this option is set, one that lets us select a rowname and mission tag for hte mission entry we are creating
		return;
	}
	
	UDataTable* EditingTable = nullptr;
	if (FModuleManager::Get().IsModuleLoaded("PDMissionEditor"))
	{
		FPDMissionEditorModule& PDMissionEditorModule = FModuleManager::GetModuleChecked<FPDMissionEditorModule>("PDMissionEditor");
		EditingTable = PDMissionEditorModule.GetIntermediaryEditingTable(true);
	}

	if (EditingTable == nullptr)
	{
		return;
	}
	
	FPDAssociativeMissionEditingRow* AssociativeRow = EditingTable->FindRow<FPDAssociativeMissionEditingRow>(SelectedMissionRowName, "");
	if(AssociativeRow != nullptr)
	{
		// @todo represent the value of the found row as node input/output 
	}
	
}

FSlateColor SPDAttributePin::GetPinColor() const
{
	FSlateColor OutColour{FLinearColor::Green};
	return OutColour;
}

//
// Widget reflection / data reference to the entry in the intermediate datatable
void SPDDataAttributePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// @todo
}

TSharedRef<SWidget> SPDDataAttributePin::GetDefaultValueWidget()
{
	return SGraphPin::GetDefaultValueWidget();
}

void SPDDataAttributePin::OnAttributeSelected(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
}

FSlateColor SPDDataAttributePin::GetPinColor() const
{
	FSlateColor OutColour{FLinearColor::Yellow};
	return OutColour;
}

//
// Key / tag selector slate widget
void SPDNewKeyDataAttributePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
}

TSharedRef<SWidget> SPDNewKeyDataAttributePin::GetDefaultValueWidget()
{
	return SGraphPin::GetDefaultValueWidget();
}

FSlateColor SPDNewKeyDataAttributePin::GetPinColor() const
{
	FSlateColor OutColour{FLinearColor::Blue};
	return OutColour;
}


//
// PinFactory
TSharedPtr<SGraphPin> FPDAttributeGraphPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	/* Compare pin-category and subcategory to make sure the pin is of the correct type  */
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionName) 
	{
		return SNew(SPDAttributePin, InPin); 
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder)
	{
		return SNew(SPDNewKeyDataAttributePin, InPin); 
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionDataRef && InPin->PinType.PinSubCategoryObject == FPDMissionRow::StaticStruct())
	{
		return SNew(SPDDataAttributePin, InPin); 
	}
	
	return FGraphPanelPinFactory::CreatePin(InPin);
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