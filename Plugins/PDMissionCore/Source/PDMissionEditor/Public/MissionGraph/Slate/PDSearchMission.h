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

#include "Widgets/SCompoundWidget.h"

class ITableRow;
class STableViewBase;
class UEdGraphNode;
namespace ESelectInfo { enum Type : int; }
namespace ETextCommit { enum Type : int; }
template <typename ItemType> class STreeView;

/** Item that matched the search results */
class FSearchInMissionResult
{
public: 
	/** Create a root (or only text) result */
	FSearchInMissionResult(const FString& InValue);
	
	/** Create a BT node result */
	FSearchInMissionResult(const FString& InValue, const TSharedPtr<FSearchInMissionResult>& InParent, UEdGraphNode* InNode);

	/** Called when user clicks on the search item */
	FReply OnClick(const TWeakPtr<class FFPDMissionGraphEditor>& MissionEditor, const TSharedPtr<FSearchInMissionResult>& Root) const;

	/** Create an icon to represent the result */
	TSharedRef<SWidget>	CreateIcon() const;

	/** Gets the comment on this node if any */
	FString GetCommentText() const;

	/** Gets the node type */
	FString GetNodeTypeText() const;

	/** Highlights mission graph nodes */
	void SetNodeHighlight(bool bHighlight);

	/** Any children listed under this mission node */
	TArray< TSharedPtr<FSearchInMissionResult> > Children;

	/** The string value for this result */
	FString Value;

	/** The graph node that this search result refers to */
	TWeakObjectPtr<UEdGraphNode> GraphNode;

	/** Search result parent */
	TWeakPtr<FSearchInMissionResult> Parent;
};

/** Widget for searching for mission data in a mission graph */
class SSearchInMission : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSearchInMission){}
	SLATE_END_ARGS()

	/** Focuses this widget's search box */
	void FocusOnSearch() const;
	
	
	void Construct(const FArguments& InArgs, const TSharedPtr<class FFPDMissionGraphEditor>& InMissionEditor);

private:
	typedef TSharedPtr<FSearchInMissionResult> FSearchResult;
	typedef STreeView<FSearchResult> STreeViewType;

	/** Called when user changes the text they are searching for */
	void OnSearchTextChanged(const FText& Text);

	/** Called when user commits text */
	void OnSearchTextCommitted(const FText& Text, ETextCommit::Type CommitType);

	/** Get the children of a row */
	void OnGetChildren(FSearchResult InItem, TArray<FSearchResult>& OutChildren);

	/** Called when user clicks on a new result */
	void OnTreeSelectionChanged(FSearchResult Item, ESelectInfo::Type SelectInfo) const;

	/** Called when a new row is being generated */
	TSharedRef<ITableRow> OnGenerateRow(FSearchResult InItem, const TSharedRef<STableViewBase>& OwnerTable) const;

	/** Begins the search based on the SearchValue */
	void InitiateSearch();
	
	/** Find any results that contain all of the tokens */
	void MatchTokens(const TArray<FString>& Tokens);

	/** Find if child contains all of the tokens and add a result accordingly */
	static void MatchTokensInChild(const TArray<FString>& Tokens, UEdGraphNode* Child, const FSearchResult& ParentNode);
	
	/** Determines if a string matches the search tokens */
	static bool StringMatchesSearchTokens(const TArray<FString>& Tokens, const FString& ComparisonString);

private:
	/** Pointer back to the mission editor that owns us */
	TWeakPtr<class FFPDMissionGraphEditor> MissionEditorPtr;
	
	/** The tree view displays the results */
	TSharedPtr<STreeViewType> TreeView;

	/** The search text box */
	TSharedPtr<class SSearchBox> SearchTextField;
	
	/** This buffer stores the currently displayed results */
	TArray<FSearchResult> ItemsFound;

	/** we need to keep a handle on the root result, because it won't show up in the tree */
	FSearchResult RootSearchResult;

	/** The string to highlight in the results */
	FText HighlightText;

	/** The string to search for */
	FString	SearchValue;	
	
};
