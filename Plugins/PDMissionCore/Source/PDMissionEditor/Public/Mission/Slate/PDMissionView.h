/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once


#include <CoreMinimal.h>

#include <SGraphPin.h>
#include <SGraphNode.h>
#include <SGraphNodeKnot.h>
#include <SNodePanel.h>
#include <Widgets/SWidget.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/SBoxPanel.h>

#include <EdGraphUtilities.h>
#include <Layout/Visibility.h>
#include <Styling/SlateColor.h>

#include <GameplayTagContainer.h>
#include <BlueprintUtilities.h>
#include <SGraphNodeDefault.h>
#include <Widgets/SCompoundWidget.h>
#include <Misc/NotifyHook.h>
#include <Misc/Attribute.h>
#include <Templates/SharedPointer.h>
#include <Delegates/Delegate.h>

#include <Math/Color.h>
#include <Math/Vector2D.h>

#include <HAL/Platform.h>
#include <Input/DragAndDrop.h>
#include <Input/Reply.h>
#include <Editor/GraphEditor/Private/DragNode.h>
#include <Internationalization/Text.h>
#include <Containers/Array.h>
#include <Containers/Map.h>
#include <Containers/UnrealString.h>
#include <DetailsViewArgs.h>

#include "PDMissionCommon.h"


class SWidget;
struct FGeometry;

class SCommentBubble;
class UPDMissionTransitionNode;
class STextEntryPopup;
class ITableRow;
class STableViewBase;
class UEdGraphNode;

namespace ESelectInfo { enum Type : int; }
template <typename ItemType> class STreeView;

class FFPDMissionGraphEditor;
class SGameplayTagCombo;

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
	virtual void UpdateGraphNode() override;
	virtual bool UseLowDetailNodeTitles() const override;
	virtual TSharedRef<SWidget> CreateNodeContentArea();
	virtual void SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget) override;
	/** Override this to create a button to add pins on the output side of the node */
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	/** Adds a new UEdGraph output pin and adds a new branch entry in the missionrow to boot */
	virtual FReply OnAddPin() override;
	//~ End SGraphNode Interface


	const FPDMissionRow* GetMissionRowPtr() const;
	static FLinearColor StaticGetTransitionColor(bool bIsOpened, bool bIsHovered);
	bool IsExtraDataEnabled() const;
	FReply OnExtraDataClicked();
	FSlateColor GetExtraDataColor() const;
	const FSlateBrush* GetExtraDataIconImage() const;


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
	FText GetTitle() const;
	FText GetDescription() const;
	EVisibility GetDescriptionVisibility() const;

	virtual FText GetPreviewCornerText() const;
	virtual const FSlateBrush* GetNameIcon() const;	

	TSharedPtr<class IStructureDetailsView> StructureDetailsViewBaseExt;
	TSharedPtr<class IStructureDetailsView> StructureDetailsViewMetaData;
	TSharedPtr<class IStructureDetailsView> StructureDetailsViewStateData;
	TSharedPtr<SWidget> ExtraDataButton;
	TSharedPtr<class IMenu> ExtraDataPopup;
	
protected:
	bool bOpenedNode = false; 
	bool bDragMarkerVisible = false;
	TArray< TSharedPtr<SGraphNode> > SubNodes;
	TSharedPtr<class SPDLODBranchNode> TitleLODNode;
};

class SGraphNodeMissionCondition : public SGraphNodeKnot 
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeMissionCondition){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UPDMissionGraphNode_Knot* InNode);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	// End of SGraphNode interface

	UPDMissionGraphNode* GetSourceNode(UEdGraphPin*& OutSourcePin) const;
	bool IsEnabled() const;
	FReply OnClicked();

	TSharedPtr<class IMenu> CondPopup;
	
	static FLinearColor StaticGetTransitionColor(bool bIsOpened, bool bIsHovered);

private:
	FSlateColor GetTransitionColor() const;
	const FSlateBrush* GetTransitionIconImage() const;

	TSharedPtr<class IStructureDetailsView> StructureDetailsView;	

	bool bOpenedNode = false;
};

class SGraphNodeMissionTransition : public SGraphNode // Wrote this ages ago, I might just trash and rewrite, 
{
public:
	SLATE_BEGIN_ARGS(SGraphNodeMissionTransition){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UPDMissionTransitionNode* InNode);

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



/** The visual representation of a control point meant to adjust how connections are routed, also known as a Reroute node.
 * The input knot node should have properly implemented ShouldDrawNodeAsControlPointOnly to return true with valid indices for its pins.
 */
class PDMISSIONEDITOR_API SMissionGraphNodeKnot : public SGraphNodeDefault
{
public:
	SLATE_BEGIN_ARGS(SMissionGraphNodeKnot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, class UEdGraphNode* InKnot);

	// SGraphNode interface
	virtual void UpdateGraphNode() override;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const override;
	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	virtual void RequestRenameOnSpawn() override { }
	// End of SGraphNode interface

protected:
	/** Returns Offset to center comment on the node's only pin */
	FVector2D GetCommentOffset() const;
protected:

	/** Toggles the hovered visibility state */
	virtual void OnCommentBubbleToggled(bool bInCommentBubbleVisible) override;

	/** If bHoveredCommentVisibility is true, hides the comment bubble after a change is committed */
	virtual void OnCommentTextCommitted(const FText& NewComment, ETextCommit::Type CommitInfo) override;

	/** The hovered visibility state. If false, comment bubble will only appear on hover. */
	bool bAlwaysShowCommentBubble = false;

	/** SharedPtr to comment bubble */
	TSharedPtr<SCommentBubble> CommentBubble;

	const FSlateBrush* ShadowBrush = nullptr;
	const FSlateBrush* ShadowBrushSelected = nullptr;
};

class PDMISSIONEDITOR_API SMissionGraphPinKnot : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SMissionGraphPinKnot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

	// SWidget interface
	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	// End of SWidget interface

protected:
	// Begin SGraphPin interface
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	virtual TSharedRef<FDragDropOperation> SpawnPinDragEvent(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins) override;
	virtual FReply OnPinMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent) override;
	virtual FSlateColor GetPinColor() const override;
	// End SGraphPin interface
};

// TODO:  Write slate code so Make Mission entry displays some simple creation wizard 
// -- 1. The user selects the tag, ensure we hide tags in existing missions from the tag list
// -- 2. A button is displayed that says "Create Mission Node". When pressed create new entry in the table
// -- 3. Update the nodes selected mission, and then refresh graph
class PDMISSIONEDITOR_API SPDNewMissionWizard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPDNewMissionWizard) : _OwningTable(nullptr) {}
	SLATE_ARGUMENT(UDataTable*, OwningTable)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);


	FReply OnClicked();
	bool IsButtonEnabled() const;

	FGameplayTag GetSelectedTag() const { return SelectedTag; }


	TSharedPtr<SGameplayTagCombo> TagCombo;
	TSharedPtr<SButton> NewMissionButton;
	
	UDataTable* OwnerTable = nullptr;

protected:
	FGameplayTag SelectedTag;

};


DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FOnGetActiveDetailSlotContent, bool);

class PDMISSIONEDITOR_API SPDLODBranchNode : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SPDLODBranchNode) : _UseLowDetailSlot(false) {}

	SLATE_ATTRIBUTE(bool, UseLowDetailSlot) // Should the low detail or high detail slot be shown?
	SLATE_NAMED_SLOT(FArguments, LowDetail) // The low-detail slot
	SLATE_NAMED_SLOT(FArguments, HighDetail) // The high-detail slot

	SLATE_EVENT(FOnGetActiveDetailSlotContent, OnGetActiveDetailSlotContent)
SLATE_END_ARGS()

public:
	SPDLODBranchNode();

	void Construct(const FArguments& InArgs);

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	// End of SWidget interface

	// Determine whether we need to show the low-detail slot or high-detail slot
	void RefreshLODSlotContent();

protected:
	int LastCachedValue; // What kind of slot was shown last frame
	TAttribute<bool> ShowLowDetailAttr; // The attribute indicating the kind of slot to show
	TSharedRef<SWidget> ChildSlotLowDetail; // The low-detail child slot
	TSharedRef<SWidget> ChildSlotHighDetail; // The high-detail child slot

	FOnGetActiveDetailSlotContent OnGetActiveDetailSlotContent;

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

class SPDTagSelector : public SPDMissionGraphPin
{
	SLATE_BEGIN_ARGS(SPDTagSelector) {}
	SLATE_END_ARGS()

	/// Wrap a tag selector
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;

	FGameplayTag GetGameplayTag() const {return Tag;}


	FGameplayTag Tag;
};

UENUM()
enum class EGenericInputSelector : uint8
{
	EMissionState,
	EMissionBranchBehaviour,
	EMissionBranchDelayTime,
	EMissionTickInterval,

	EMissionTickPaused,
	EMissionRepeatable,

	ENextMissionBranch,
	EMissionMetadata,
	EAllMissionRowData,

	EMissionID,
	EMissionTickDeltaValue,
	MAX,
};

class SPDGenericInputWrapper : public SPDMissionGraphPin
{
	SLATE_BEGIN_ARGS(SPDGenericInputWrapper) : _InputType(EGenericInputSelector::EMissionState) {}
	SLATE_ATTRIBUTE(EGenericInputSelector, InputType)
	SLATE_ARGUMENT(FName, MissionRowName)
	SLATE_END_ARGS()

	/// Wrap a tag selector
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;


	TSharedRef<SWidget> GenerateBaseSettingsContent();
	TSharedRef<SWidget> GenerateTickSettingsContent();
	TSharedRef<SWidget> GenerateProgressRulesContent();
	TSharedRef<SWidget> GenerateMetadataContent();
	TSharedRef<SWidget> GenerateBranchesContent();

	TSharedRef<SWidget> GenerateTagCompoundContent();
	TSharedRef<SWidget> GenerateStateDataContent();

private:
	FPDMissionRow* GetCurrentMissionRow();
	TPair<FDetailsViewArgs, FStructureDetailsViewArgs> GetDetailsArgs()
	{
		FDetailsViewArgs RetDetailsViewArgs;
		RetDetailsViewArgs.bAllowSearch = false;
		RetDetailsViewArgs.bHideSelectionTip = true;
		RetDetailsViewArgs.bLockable = false;
		RetDetailsViewArgs.bSearchInitialKeyFocus = false;
		RetDetailsViewArgs.bUpdatesFromSelection = false;
		RetDetailsViewArgs.bShowOptions = false;
		FStructureDetailsViewArgs RetStructureDetailsViewArgs;
		RetStructureDetailsViewArgs.bShowObjects = true;
		return TPair<FDetailsViewArgs, FStructureDetailsViewArgs>{RetDetailsViewArgs, RetStructureDetailsViewArgs};
	}

	TAttribute<EGenericInputSelector> InputTypeAttr;
	FName MissionRowName;

	TSharedPtr<class IStructureDetailsView> StructureDetailsView;
	
	union 
	{
		EPDMissionState MissionState; 
		EPDMissionBranchBehaviour BranchBehaviour;
		ECheckBoxState IsPaused, IsRepeatable;
		float Interval, BranchDelayTime;
		int32 mID, DeltaValue;
	};
	TArray<TSharedPtr<FString>> Options;
public:
	ECheckBoxState GetPauseState() const {return GetValue<EGenericInputSelector::EMissionTickPaused>();}
	ECheckBoxState GetRepeatableState() const {return GetValue<EGenericInputSelector::EMissionRepeatable>();}
	int32 GetMissionID() const {return GetValue<EGenericInputSelector::EMissionID>();}
	FText GetMissionIDAsText() const {return FText::AsCultureInvariant(FString::FromInt(GetMissionID()));}
	TSharedPtr<FString> GetMissionStateAsString() const;
	TSharedPtr<FString> GetMissionBranchBehaviourAsString() const;
	float GetMissionTickInterval() const;
	float GetMissionBranchDelay() const;
	FText GetMissionTickIntervalAsText() const;
	FText GetMissionBranchDelayAsText() const;

	
	void OnPauseStateChanged(ECheckBoxState NewState);
	void OnRepeatableStateChanged(ECheckBoxState NewState);
	void OnMissionIDChanged(int32 NewID);
	void OnMissionIDChangedViaTag(FGameplayTag MissionTag);
	void OnMissionStateChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnMissionBranchBehaviourChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnMissionTickIntervalChanged(float NewVal);
	void OnMissionBranchDelayChanged(float NewVal);


	template<EGenericInputSelector TSelector>
	auto& GetValueMutable() 
	{
		using enum EGenericInputSelector;
		if constexpr (TSelector == EMissionState){return MissionState;}
		else if constexpr (TSelector == EMissionBranchBehaviour){return BranchBehaviour;}
		else if constexpr (TSelector == EMissionBranchDelayTime){return BranchDelayTime;}
		else if constexpr (TSelector == EMissionTickInterval){return Interval;}

		else if constexpr (TSelector == EMissionTickPaused){return IsPaused;}
		else if constexpr (TSelector == EMissionRepeatable){return IsRepeatable;}

		else if constexpr (TSelector == EMissionID){return mID;}
		else if constexpr (TSelector == EMissionTickDeltaValue){return DeltaValue;}
		else {return INDEX_NONE;}
	}

	template<EGenericInputSelector TSelector>
	auto GetValue() const
	{
		SPDGenericInputWrapper* MutableSelf = const_cast<SPDGenericInputWrapper*>(this);
		return  MutableSelf->GetValueMutable<TSelector>();
	}	
};

//
// Empty Label fake pin
class SPDLabelAsPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDLabelAsPin) {}
	SLATE_ARGUMENT(FName, MissionRowName)	
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;

// SGraphPin interface
	/** @return Actual icon object */
	virtual const FSlateBrush* GetPinIcon() const override;

	/** @return The color that we should use to draw this pin */
	virtual FSlateColor GetPinColor() const override { return FSlateColor{FColor::Green}; };

	/** @return The color that we should use to draw the highlight for this pin */
	virtual FSlateColor GetHighlightColor() const override { return FSlateColor{FColor::Silver}; };

	virtual FSlateColor GetPinDiffColor() const override { return FSlateColor{FColor::Transparent};};
	/** @return The color that we should use to draw this pin's text */
	virtual FSlateColor GetPinTextColor() const override { return FSlateColor{FColor::Emerald};};

	/** We never allow connections with a label pin, these are only cosmetical */
	virtual bool TryHandlePinConnection(SGraphPin& OtherSPin) {return false;};
// End of SGraphPin interface

	FName MissionRowName;
	TSharedPtr<class IStructureDetailsView> StructureDetailsView;
	TWeakObjectPtr<UObject> SubCategoryObject = nullptr;

	friend class FPDAttributeGraphPinFactory;
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
Licensed Work:        PDOpenSource v.0.1.0 (Source available on github)
                      The Licensed Work is (c) 2026 Ario Amin (@ Permafrost Development)
Additional Use Grant: You may make commercial use of the Licensed Work provided these three additional conditions as met; 
                      1. Must give attributions to the original author of the Licensed Work, in 'Credits' if that is applicable.
                      2. The Licensed Work must be 'Compiled' before being redistributed.
                      3. The Licensed Work 'Source' may be linked but may not be packaged into the product or service being sold
                      4. The Licenced Work 'Source' Must not be resold or repackaged or redistributed as another product, is only allowed to be used within a commercial or non-commercial game project.
                      5. Teams whose 'Total Finances' exceed $100,000 USD for the most recent 12-month period must contact the owner for a custom license or buy the framework from a marketplace it has been made available on.

                      "Credits" indicate a scrolling screen with attributions. This is usually in a products end-state

                      "Total Finances" means the largest of your aggregate gross revenues, entire budget, or funding (no matter the source).
                      "Package" means the collection of files distributed by the Licensor, and derivatives of that collection
                      and/or of those files..   

                      "Source" form means the source code, documentation source, and configuration files for the Package, usually in human-readable format.

                      "Compiled" form means the compiled bytecode, object code, binary, or any other
                      form resulting from mechanical transformation or translation of the Source form.


Change Date:          2030-05-14

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