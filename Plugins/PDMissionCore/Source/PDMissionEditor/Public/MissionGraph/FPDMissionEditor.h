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

#include "PDMissionGraphTypes.h"

#include "Misc/NotifyHook.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "GraphEditor.h"

class SGraphEditor;
struct FGraphAppearanceInfo;

class FPDMissionDebugger;
class FPDMissionEditorToolbar;
class FDocumentTabFactory;
class FDocumentTracker;
class IDetailsView;
class SSearchInMission;
class UEdGraph;
class UDataTable;
class SMissionTreeEditor;
struct Rect;

class UEdGraphNode;

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#define FOnCanConst(Token) \
	void Token () const;\
	bool Can##Token () const;
#define FOnCan(Token) \
	void Token ();\
	bool Can##Token () const;


class PDMISSIONEDITOR_API FFPDMissionGraphEditor : public FEditorUndoClient, public FWorkflowCentricApplication, public FNotifyHook
{
public:
	FFPDMissionGraphEditor();
	virtual ~FFPDMissionGraphEditor();

	FGraphPanelSelectionSet GetSelectedNodes() const;
	virtual void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	void CreateCommandList();

	/** @brief Delegates for graph editor commands */
	void DeleteSelectedDuplicatableNodes();
	void PasteNodesAtLocation(const FVector2D& Location);

	// Node operations
	FOnCanConst(SelectAllNodes);
	FOnCanConst(DeleteNodes);
	FOnCanConst(SearchMissionTree);
	FOnCan(CutNodes);
	FOnCanConst(CopyNodes);
	FOnCan(PasteNodes);
	FOnCan(DuplicateNodes);
	FOnCanConst(CreateComment);

	/** @brief Graph operations */
	virtual void OnClassListUpdated();

	/** @brief Register tab spawners for the mission editor */
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	/** @brief This function is called when the module starts */
	void InitMissionEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const FPDMissionNodeHandle& InData);
	/** @brief This function is called when the module starts */	
	void InitMissionEditor( const FPDMissionNodeHandle& InData);

	/** @brief This function is called when the module starts */	
	static TSharedRef<FFPDMissionGraphEditor> CreateMissionEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const FPDMissionNodeHandle& InData);
	/** @brief This function is called when the module starts */	
	static TSharedRef<FFPDMissionGraphEditor> CreateMissionEditor(const FPDMissionNodeHandle& InData);

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	//~ End IToolkit Interface

	virtual void FocusWindow(UObject* ObjectToFocusOn = nullptr);

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	// End of FNotifyHook

	// Delegates
	void OnNodeDoubleClicked(class UEdGraphNode* Node);
	void OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor);
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode(bool bGraphIsEditable) const;
	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

	EVisibility GetDebuggerDetailsVisibility() const;

	/** 
	 * Get the localized text to display for the specified mode 
	 * @param	InMode	The mode to display
	 * @return the localized text representation of the mode
	 */
	static FText GetLocalizedMode(FName InMode);

	/** Access the toolbar builder for this editor */
	TSharedPtr<class FPDMissionEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }

	/** Get the mission asset we are editing (if any) */
	FPDMissionNodeHandle& GetMissionData() const;

	/** Spawns the tab with the update graph inside */
	TSharedRef<SWidget> SpawnProperties() const;

	/** Spawns the search tab */
	TSharedRef<SWidget> SpawnSearch();

	/** Spawns the mission tree tab */
	TSharedRef<SWidget> SpawnMissionTree();

	// @todo This is a hack for now until we reconcile the default toolbar with application modes [duplicated from counterpart in Blueprint Editor]
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager);

	/** Regenerates the mission graph we were editing or creates a new one if there is none available */
	void RegenerateMissionGraph();

	/** Save the graph state for later editing */
	void SaveEditedObjectState();

	/** Are we allowed to create new node classes right now? */
	bool CanCreateNewMissionNodes() const;

	/** Create the menu used to make a new task node */
	TSharedRef<SWidget> HandleCreateNewStructMenu(UScriptStruct* BaseStruct) const;

	/** Handler for when a node class is picked */
	void HandleNewNodeStructPicked(const UScriptStruct* InStruct) const;

	/** @brief Get graph editor (tab) factory pointer */
	TWeakPtr<FDocumentTabFactory> GetGraphEditorTabFactoryPtr() { return GraphEditorTabFactoryPtr; };

	/** Create widget for graph editing */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);


protected:
	virtual void FixupPastedNodes(const TSet<UEdGraphNode*>& NewPastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping);

private:
	bool IsPropertyEditable() const;
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Add custom menu options */
	void ExtendMenu();

	/** Setup common commands */
	void BindCommands();


public:
	/** Modes in mode switcher */
	static const FName GraphViewMode;
	static const FName TreeViewMode;
	FPDMissionDebuggerHandler DebugHandler;
	FSimpleMulticastDelegate FocusedGraphEditorChanged;

protected:

	/** Currently focused graph */
	TWeakPtr<SGraphEditor> FocusGraphEditorPtr;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

private:
	TSharedPtr<FDocumentTracker> DocumentManager;
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/* The mission being edited */
	FPDMissionNodeHandle MissionData;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;

	// TSharedPtr<FPDMissionDebuggerHandler> Debugger; @todo

	/** Find results log as well as the search filter */
	TSharedPtr<SSearchInMission> SearchResults;
	TSharedPtr<SMissionTreeEditor> TreeEditor;

	uint32 bForceDisablePropertyEdit : 1;

	TSharedPtr<FPDMissionEditorToolbar> ToolbarBuilder;	

};
