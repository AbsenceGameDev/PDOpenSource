/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "MissionGraph/Slate/PDSearchMission.h"
#include "MissionGraph/FPDMissionEditor.h"
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