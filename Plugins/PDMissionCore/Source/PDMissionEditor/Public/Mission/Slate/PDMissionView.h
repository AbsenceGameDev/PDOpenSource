/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once


#include "CoreMinimal.h"

#include "SGraphPin.h"
#include "SGraphNode.h"
#include "SNodePanel.h"
#include "Widgets/SWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"

#include "EdGraphUtilities.h"
#include "Layout/Visibility.h"
#include "Styling/SlateColor.h"

#include "GameplayTagContainer.h"
#include "BlueprintUtilities.h"
#include "Widgets/SCompoundWidget.h"
#include "Misc/NotifyHook.h"
#include "Templates/SharedPointer.h"

#include "Math/Color.h"
#include "Math/Vector2D.h"

#include "HAL/Platform.h"
#include "Input/DragAndDrop.h"
#include "Input/Reply.h"
#include "Editor/GraphEditor/Private/DragNode.h"
#include "Internationalization/Text.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"

class UPDMissionTransitionNode;
class STextEntryPopup;
class ITableRow;
class STableViewBase;
class UEdGraphNode;

namespace ESelectInfo { enum Type : int; }
template <typename ItemType> class STreeView;

class FFPDMissionGraphEditor;

/** Item that matched the search results */
class FMissionTreeNode : public TSharedFromThis<FMissionTreeNode>
{
public:
	/** Create a BT node result */
	FMissionTreeNode(UEdGraphNode* InNode, const TSharedPtr<const FMissionTreeNode>& InParent);

	/** Called when user clicks on the search item */
	FReply OnClick(const TWeakPtr<FFPDMissionGraphEditor>& MissionEditorPtr) const;

	/** Create an icon to represent the result */
	static TSharedRef<SWidget>	CreateIcon();

	/** Gets the comment on this node if any */
	FString GetCommentText() const;

	/** Gets the node type */
	FText GetNodeTypeText() const;

	/** Gets the node title text */
	FText GetText() const;

	UEdGraphNode* GetGraphNode() const { return GraphNodePtr.Get(); }

	const TArray< TSharedPtr<FMissionTreeNode> >& GetChildren() const;

	/** Search result parent */
	TWeakPtr<const FMissionTreeNode> ParentPtr;

private:
	mutable bool bChildrenDirty = true;

	/** Any children listed under this mission node */
	mutable TArray< TSharedPtr<FMissionTreeNode> > Children;

	/** The graph node that this search result refers to */
	TWeakObjectPtr<UEdGraphNode> GraphNodePtr;
};

/** */
class SMissionTreeEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMissionTreeEditor){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedPtr<class FFPDMissionGraphEditor>& InMissionEditor);

private:
	typedef STreeView<TSharedPtr<FMissionTreeNode>> STreeViewType;

	/** Get the children of a row */
	void OnGetChildren(TSharedPtr<FMissionTreeNode> InItem, TArray<TSharedPtr<FMissionTreeNode>>& OutChildren);

	/** Called when user clicks on a new result */
	void OnTreeSelectionChanged(TSharedPtr<FMissionTreeNode> Item, ESelectInfo::Type SelectInfo);

	/** Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FMissionTreeNode> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	void OnFocusedGraphChanged();
	void OnGraphChanged(const FEdGraphEditAction& Action);

	/** Begins the search based on the SearchValue */
	void RefreshTree();
	void BuildTree();
	
private:
	/** Pointer back to the mission editor that owns us */
	TWeakPtr<class FFPDMissionGraphEditor> MissionEditorPtr;
	
	/** The tree view displays the results */
	TSharedPtr<STreeViewType> TreeView;
	
	/** This buffer stores the currently displayed results */
	TArray<TSharedPtr<FMissionTreeNode>> RootNodes;

	/** The string to search for */
	FString	SearchValue;
	
};


class SGraphPanel;
class SToolTip;
class UPDMissionGraphNode;

class FDragMissionGraphNode : public FDragNode
{
public:
	DRAG_DROP_OPERATOR_TYPE(FDragMissionGraphNode, FDragNode)

	static TSharedRef<FDragMissionGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphNode>& InDraggedNode);
	static TSharedRef<FDragMissionGraphNode> New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphNode> >& InDraggedNodes);

	UPDMissionGraphNode* GetDropTargetNode() const;

	double StartTime;

protected:
	typedef FDragNode Super;
};

class PDMISSIONEDITOR_API SMissionGraphNode final : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SMissionGraphNode){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPDMissionGraphNode* InNode);

	//~ Begin SGraphNode Interface
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
	virtual FReply OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	virtual void SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel) override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	//~ End SGraphNode Interface

	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

	/** adds subnode widget inside current node */
	virtual void AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget);

	/** gets decorator or service node if one is found under mouse cursor */
	TSharedPtr<SGraphNode> GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent);

	/** gets drag over marker visibility */
	EVisibility GetDragOverMarkerVisibility() const;

	/** sets drag marker visible or collapsed on this node */
	void SetDragMarker(bool bEnabled);

protected:
	TArray< TSharedPtr<SGraphNode> > SubNodes;

	uint32 bDragMarkerVisible : 1;
	
	
	virtual FText GetTitle() const;
	virtual FText GetDescription() const;
	virtual EVisibility GetDescriptionVisibility() const;

	virtual FText GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;
};

class SGraphNodeMissionTransition : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeMissionTransition){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UPDMissionTransitionNode* InNode);

	// SNodePanel::SNode interface
	virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const override;
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	virtual bool RequiresSecondPassLayout() const override;
	virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const override;
	// End of SNodePanel::SNode interface

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	// End of SGraphNode interface

	// SWidget interface
	void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	// End of SWidget interface

	// Calculate position for multiple nodes to be placed between a start and end point, by providing this nodes index and max expected nodes 
	void PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const;

	static FLinearColor StaticGetTransitionColor(UPDMissionTransitionNode* TransNode, bool bIsHovered);

private:
	TSharedPtr<STextEntryPopup> TextEntryWidget;

	/** Cache of the widget representing the previous state node */
	mutable TWeakPtr<SNode> PrevStateNodeWidgetPtr;

private:
	FText GetPreviewCornerText(bool reverse) const;
	FSlateColor GetTransitionColor() const;
	const FSlateBrush* GetTransitionIconImage() const;

	TSharedRef<SWidget> GenerateRichTooltip();
	// TSharedRef<SWidget> GenerateInlineDisplayOrEditingWidget(bool bShowGraphPreview);
};


//
// Pins

class SPDMissionGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDMissionGraphPin) {}
	SLATE_END_ARGS()
	
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObject);
	
	virtual TSharedRef<SWidget> GetDefaultValueWidget() override;
	virtual FSlateColor GetPinColor() const override;
	const FSlateBrush* GetPinBorder() const;
};

class SPDAttributePin : public SPDMissionGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDAttributePin) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	// void FillMissionList(bool bOverwrite);
	
	//this override is used to display slate widget used for customization.
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	void OnAttributeSelected(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
	virtual FSlateColor GetPinColor() const override;
private:

};

// @todo This should be the split struct, needs to reflect the actual values in the given entry in teh datatable,
// @todo cont: Note should be taken that child mission and mission transition rules should be represented in the graph visually, much like the state machine in the animinstance 
class SPDDataAttributePin : public SPDMissionGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDDataAttributePin) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	
	//this override is used to display slate widget used for customization.
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	void OnAttributeSelected(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
	virtual FSlateColor GetPinColor() const override;
private:

	// FPDMissionRow
};

class SPDNewKeyDataAttributePin : public SPDMissionGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDNewKeyDataAttributePin) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	
	//this override is used to display slate widget used for customization.
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	virtual FSlateColor GetPinColor() const override;
private:

	FName MissionName;
	FGameplayTag MissionTag;
};

//
// Struct pin widget
class SPDStructBuilder : public SWidget
{
	SLATE_BEGIN_ARGS(SPDStructBuilder) {}
	SLATE_END_ARGS()
};


//
// Pin factory

class FPDAttributeGraphPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};

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