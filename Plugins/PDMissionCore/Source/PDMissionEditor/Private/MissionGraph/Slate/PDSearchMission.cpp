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

#include "MissionGraph/Slate/PDSearchMission.h"
#include "..\..\..\Public\MissionGraph\FPDMissionEditor.h"
#include "MissionGraph/PDMissionGraphNode.h"

#include "EdGraph/EdGraph.h"
#include "Framework/Views/TableViewMetadata.h"
#include "Layout/WidgetPath.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/STreeView.h"

#define LOCTEXT_NAMESPACE "FSearchInMission"

FSearchInMissionResult::FSearchInMissionResult(const FString& InValue)
	: Value(InValue), GraphNode(nullptr)
{
}

FSearchInMissionResult::FSearchInMissionResult(const FString& InValue, const TSharedPtr<FSearchInMissionResult>& InParent, UEdGraphNode* InNode)
	: Value(InValue), GraphNode(InNode), Parent(InParent)
{
}

void FSearchInMissionResult::SetNodeHighlight(bool bHighlight)
{
}

TSharedRef<SWidget> FSearchInMissionResult::CreateIcon() const
{
	return SNew(SImage) // Icon and icon colour
		.Image(GraphNode.IsValid() ? FAppStyle::GetBrush(TEXT("GraphEditor.FIB_Event")) : nullptr)
		.ColorAndOpacity(FSlateColor::UseForeground()); 
}

FReply FSearchInMissionResult::OnClick(const TWeakPtr<class FFPDMissionGraphEditor>& MissionEditorPtr, const TSharedPtr<FSearchInMissionResult>& Root) const
{
	if (MissionEditorPtr.IsValid() == false || GraphNode.IsValid() == false)
	{
		return FReply::Handled();
	}
	
	Parent.IsValid() && Parent.HasSameObject(Root.Get()) ?
		MissionEditorPtr.Pin()->DebugHandler.JumpToNode(GraphNode.Get())
		: MissionEditorPtr.Pin()->DebugHandler.JumpToNode(Parent.Pin()->GraphNode.Get());
	
	return FReply::Handled();
}

FString FSearchInMissionResult::GetNodeTypeText() const
{
	if (GraphNode.IsValid() == false) { return FString(); }
	
	FString NodeClassName = GraphNode->GetClass()->GetName();
	const int32 StringPos = NodeClassName.Find("_");
	if (StringPos == INDEX_NONE) { return NodeClassName; }

	return NodeClassName.RightChop(StringPos + 1);
}

FString FSearchInMissionResult::GetCommentText() const
{
	if (GraphNode.IsValid() == false) { return FString(); }

	return GraphNode->NodeComment;
}

//////////////////////////////////////////////////////////////////////////
// SSearchInMission

void SSearchInMission::Construct( const FArguments& InArgs, const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditor)
{
	MissionEditorPtr = InMissionEditor;

	this->ChildSlot
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(SearchTextField, SSearchBox)
					.HintText(LOCTEXT("MissionSearchHint", "Enter text to find nodes..."))
					.OnTextChanged(this, &SSearchInMission::OnSearchTextChanged)
					.OnTextCommitted(this, &SSearchInMission::OnSearchTextCommitted)
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("Menu.Background"))
				[
					SAssignNew(TreeView, STreeViewType)
					.ItemHeight(24)
					.TreeItemsSource(&ItemsFound)
					.OnGenerateRow(this, &SSearchInMission::OnGenerateRow)
					.OnGetChildren(this, &SSearchInMission::OnGetChildren)
					.OnSelectionChanged(this, &SSearchInMission::OnTreeSelectionChanged)
					.SelectionMode(ESelectionMode::Multi)
				]
			]
		];
}

void SSearchInMission::FocusOnSearch() const
{
	// NOTE: Careful, GeneratePathToWidget can be reentrant in that it can call visibility delegates and such
	FWidgetPath FilterTextBoxWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchTextField.ToSharedRef(), FilterTextBoxWidgetPath);

	// Set keyboard focus directly
	FSlateApplication::Get().SetKeyboardFocus(FilterTextBoxWidgetPath, EFocusCause::SetDirectly);
}

void SSearchInMission::OnSearchTextChanged(const FText& Text)
{
	SearchValue = Text.ToString();
	InitiateSearch();
}

void SSearchInMission::OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	OnSearchTextChanged(Text);
}

void SSearchInMission::InitiateSearch()
{
	TArray<FString> Tokens;
	SearchValue.ParseIntoArray(Tokens, TEXT(" "), true);

	for (auto It(ItemsFound.CreateIterator()); It; ++It)
	{
		It->Get()->SetNodeHighlight(false); // need to reset highlight
		TreeView->SetItemExpansion(*It, false);
	}
	
	ItemsFound.Empty();
	if (Tokens.Num() > 0)
	{
		HighlightText = FText::FromString(SearchValue);
		MatchTokens(Tokens);
	}

	// Insert a fake result to inform user if none found
	if (ItemsFound.Num() == 0)
	{
		ItemsFound.Add(MakeShared<FSearchInMissionResult>(LOCTEXT("MissionSearchNoResults", "No Results found").ToString()));
	}

	TreeView->RequestTreeRefresh();

	for (auto It(ItemsFound.CreateIterator()); It; ++It)
	{
		TreeView->SetItemExpansion(*It, true);
	}
}

void SSearchInMission::MatchTokens(const TArray<FString>& Tokens)
{
	RootSearchResult.Reset();

	const TWeakPtr<SGraphEditor> FocusedGraphEditor = MissionEditorPtr.Pin()->GetFocusedGraphPtr();
	const UEdGraph* Graph = FocusedGraphEditor.IsValid() ? FocusedGraphEditor.Pin()->GetCurrentGraph() : nullptr;
	if (Graph == nullptr) { return; }

	RootSearchResult = MakeShared<FSearchInMissionResult>(FString("MissionRoot"));

	for (auto It(Graph->Nodes.CreateConstIterator()); It; ++It)
	{
		UEdGraphNode* Node = *It;
			
		const FString NodeName = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
		FSearchResult NodeResult(new FSearchInMissionResult(NodeName, RootSearchResult, Node));

		FString NodeSearchString = NodeName + Node->GetClass()->GetName() + Node->NodeComment;
		NodeSearchString = NodeSearchString.Replace(TEXT(" "), TEXT(""));

		const bool bNodeMatchesSearch = StringMatchesSearchTokens(Tokens, NodeSearchString);
		if (UPDMissionGraphNode* MissionGraphNode = Cast<UPDMissionGraphNode>(Node))
		{
			for (UPDMissionGraphNode* SubNode : MissionGraphNode->SubNodes)
			{
				MatchTokensInChild(Tokens, SubNode, NodeResult);
			}
		}

		if (NodeResult->Children.IsEmpty() && bNodeMatchesSearch == false) { continue; }
		NodeResult->SetNodeHighlight(true);
		ItemsFound.Add(NodeResult);
	}
}

void SSearchInMission::MatchTokensInChild(const TArray<FString>& Tokens, UEdGraphNode* Child, const FSearchResult& ParentNode)
{
	if (Child == nullptr) { return; }

	const FString ChildName = Child->GetNodeTitle(ENodeTitleType::ListView).ToString();
	FString ChildSearchString = ChildName + Child->GetClass()->GetName() + Child->NodeComment;
	
	ChildSearchString = ChildSearchString.Replace(TEXT(" "), TEXT(""));
	if (StringMatchesSearchTokens(Tokens, ChildSearchString) == false) { return; }
	
	const FSearchResult DecoratorResult(new FSearchInMissionResult(ChildName, ParentNode, Child));
	ParentNode->Children.Add(DecoratorResult);
}

TSharedRef<ITableRow> SSearchInMission::OnGenerateRow( FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable ) const
{
	return SNew(STableRow< TSharedPtr<FSearchInMissionResult> >, OwnerTable)
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
						.Text(FText::FromString(InItem->Value))
						.HighlightText(HighlightText)
					]
				]
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetNodeTypeText()))
				.HighlightText(HighlightText)
			]
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(InItem->GetCommentText()))
				.ColorAndOpacity(FLinearColor::Yellow)
				.HighlightText(HighlightText)
			]
		];
}

void SSearchInMission::OnGetChildren(FSearchResult InItem, TArray< FSearchResult >& OutChildren)
{
	OutChildren += InItem->Children;
}

void SSearchInMission::OnTreeSelectionChanged(FSearchResult Item , ESelectInfo::Type) const
{
	if (Item.IsValid() == false) { return; }

	Item->OnClick(MissionEditorPtr, RootSearchResult).Handled();
}

bool SSearchInMission::StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString)
{
	bool bFoundAllTokens = true;

	//search the entry for each token, it must have all of them to pass
	for (auto TokIT(Tokens.CreateConstIterator()); TokIT; ++TokIT)
	{
		const FString& Token = *TokIT;
		if (ComparisonString.Contains(Token)) { continue; }
		
		bFoundAllTokens = false;
		break;
	}
	return bFoundAllTokens;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
