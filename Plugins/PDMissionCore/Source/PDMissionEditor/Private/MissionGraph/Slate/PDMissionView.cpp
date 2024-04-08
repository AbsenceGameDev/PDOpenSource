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

#include "MissionGraph/Slate/PDMissionView.h"
#include "MissionGraph/FPDMissionEditor.h"
#include "MissionGraph/PDMissionGraph.h"
#include "MissionGraph/PDMissionBuilder.h"
#include "MissionGraph/PDMissionGraphNode.h"

#include "EdGraph/EdGraphPin.h"
#include "Framework/Views/TableViewMetadata.h"
#include "Widgets/Views/STreeView.h"

#define LOCTEXT_NAMESPACE "FMissionTreeNode"

//////////////////////////////////////////////////////////////////////////
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

//////////////////////////////////////////////////////////////////////////
// SMissionTreeEditor

void SMissionTreeEditor::Construct( const FArguments& InArgs, const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditor)
{
	MissionEditorPtr = InMissionEditor;
	InMissionEditor->FocusedGraphEditorChanged.AddSP(this, &SMissionTreeEditor::OnFocusedGraphChanged);
	FPDMissionNodeHandle& MissionData = InMissionEditor->GetMissionData();
	if (UPDMissionGraph* MyGraph = FPDMissionBuilder::GetGraphFromBank(MissionData, 0))
	{
		MyGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &SMissionTreeEditor::OnGraphChanged));
	}

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
		if (UPDMissionGraphNode_EntryPoint* RootNode = Cast<UPDMissionGraphNode_EntryPoint>(Node))
		{
			RootNodes.Add(MakeShared<FMissionTreeNode>(RootNode, TSharedPtr<FMissionTreeNode>()));
		}
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
	if (Item.IsValid())
	{
		Item->OnClick(MissionEditorPtr);
	}
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
