/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "MissionGraph/Slate/PDMissionView.h"
#include "MissionGraph/FPDMissionEditor.h"
#include "MissionGraph/PDMissionGraph.h"
#include "MissionGraph/PDMissionBuilder.h"
#include "MissionGraph/PDMissionGraphNode.h"
#include "MissionGraph/PDMissionGraphSchema.h"

#include <EdGraph/EdGraphPin.h>
#include <EdGraph/EdGraphSchema.h>
#include <Framework/Views/TableViewMetadata.h>
#include <Widgets/Input/STextComboBox.h>
#include <Widgets/Views/STreeView.h>

#include "PDMissionEditor.h"
#include "Subsystems/PDMissionSubsystem.h"

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
	if (Item.IsValid() == false) { return; }

	Item->OnClick(MissionEditorPtr);
}


//
// Row Selector attribute

void SPDAttributePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// FillMissionList(true);
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
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionRow && InPin->PinType.PinSubCategoryObject == FPDMissionRow::StaticStruct()) 
	{
		return SNew(SPDAttributePin, InPin); 
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionDataRef && InPin->PinType.PinSubCategoryObject == FPDMissionRow::StaticStruct())
	{
		return SNew(SPDDataAttributePin, InPin); 
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder && InPin->PinType.PinSubCategoryObject == FPDMissionRow::StaticStruct())
	{
		return SNew(SPDNewKeyDataAttributePin, InPin); 
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