/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
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