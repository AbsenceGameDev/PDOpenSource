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
#pragma once

// #include "SlateBasics.h"
#include "SGraphPin.h"
#include "BlueprintUtilities.h"
#include "EdGraphUtilities.h"
#include "Widgets/SCompoundWidget.h"

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


class SPDAttributePin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SPDAttributePin) {}
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj);

	void FillMissionList(bool bOverwrite);
	
	//this override is used to display slate widget used for customization.
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	void OnAttributeSelected(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo);
	virtual FSlateColor GetPinColor() const override;
private:
	TArray<TSharedPtr<FString>> MissionConcatList;
	TArray<TSharedPtr<FString>> MissionRowNameList;
	TMap<int32, FName> IndexToName;
};


class FPDAttributeGraphPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override;
};