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


#include "MissionGraph/PDMissionEditor.h"

#include "MissionGraph/PDMissionGraph.h"
#include "MissionGraph/PDMissionGraphNode.h"
#include "MissionGraph/PDMissionEditorToolbar.h"
#include "MissionGraph/PDMissionTabFactories.h"
#include "PDMissionGraphTypes.h"
#include "PDMissionEditorCommands.h"

#include "Framework/Application/SlateApplication.h"
#include "Editor/EditorEngine.h"
#include "EngineGlobals.h"
#include "ScopedTransaction.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "PDMissionCommon.h"
#include "StructViewerFilter.h"
#include "StructViewerModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraph/EdGraphSchema.h"
// #include "MissionGraph/PDMissionBuilder.h"
#include "MissionGraph/PDMissionEditorModes.h"
#include "MissionGraph/Slate/PDMissionView.h"
#include "MissionGraph/Slate/PDSearchMission.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "MissionGraph"

const FName FPDMissionGraphEditor::GraphViewMode("GraphViewMode");
const FName FPDMissionGraphEditor::TreeViewMode("TreeViewMode");

const FName MissionEditorAppIdentifier("MissionEditorApp");

FPDMissionGraphEditor::FPDMissionGraphEditor()
{
	MissionData.DataTarget = FDataTableRowHandle{};
	bCheckDirtyOnAssetSave = true;
	
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor != nullptr) { Editor->RegisterForUndo(this); }

	OnClassListUpdatedDelegateHandle =
		FPDMissionDataNodeHelper::OnPackageListUpdated.AddRaw(this, &FPDMissionGraphEditor::OnClassListUpdated);
}


FPDMissionGraphEditor::~FPDMissionGraphEditor()
{
	// Debugger.Reset(); @todo : debugger
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor) { Editor->UnregisterForUndo(this); }

	FPDMissionDataNodeHelper::OnPackageListUpdated.Remove(OnClassListUpdatedDelegateHandle);
}

#define MGenericMapAction(Token) \
	MapAction(FGenericCommands::Get(). Token, \
		FExecuteAction::CreateRaw(this, &FPDMissionGraphEditor:: Token##Nodes), \
		FCanExecuteAction::CreateRaw(this, &FPDMissionGraphEditor::Can##Token##Nodes) \
		);

#define MGEMapAction(Token) \
	MapAction(FGraphEditorCommands::Get(). Token, \
		FExecuteAction::CreateRaw(this, &FPDMissionGraphEditor:: Token), \
		FCanExecuteAction::CreateRaw(this, &FPDMissionGraphEditor::Can##Token) \
		);	

void FPDMissionGraphEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid()) { return; }

	GraphEditorCommands = MakeShareable(new FUICommandList);

	GraphEditorCommands->MGenericMapAction(SelectAll);
	GraphEditorCommands->MGenericMapAction(Delete);
	GraphEditorCommands->MGenericMapAction(Copy);
	GraphEditorCommands->MGenericMapAction(Cut);
	GraphEditorCommands->MGenericMapAction(Paste);
	GraphEditorCommands->MGenericMapAction(Duplicate);
	GraphEditorCommands->MGEMapAction(CreateComment);
}

FGraphPanelSelectionSet FPDMissionGraphEditor::GetSelectedNodes() const
{
	const TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	if (FocusedGraphEd == nullptr) { return FGraphPanelSelectionSet{}; }

	return FocusedGraphEd->GetSelectedNodes();
}

void FPDMissionGraphEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	// @todo
}

void FPDMissionGraphEditor::PostUndo(bool bSuccess)
{
	if (bSuccess == false) { return; }

	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr)
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}
	
	// Clear selection and avoid references to nodes that will be removed
	CurrentGraphEditor->ClearSelectionSet();
	CurrentGraphEditor->NotifyGraphChanged();
}

void FPDMissionGraphEditor::PostRedo(bool bSuccess)
{
	if (bSuccess == false) { return; }

	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr)
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}
	
	// Clear selection and avoid references to nodes that will be removed
	CurrentGraphEditor->ClearSelectionSet();
	CurrentGraphEditor->NotifyGraphChanged();
}

void FPDMissionGraphEditor::SelectAllNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }
	
	CurrentGraphEditor->SelectAllNodes();
}

bool FPDMissionGraphEditor::CanSelectAllNodes() const
{
	return true;
}

void FPDMissionGraphEditor::DeleteNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt);
		if (Node == nullptr || Node->CanUserDeleteNode() == false) { continue; }
		
		Node->Modify();
		Node->DestroyNode();
	}
}

bool FPDMissionGraphEditor::CanDeleteNodes() const
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr || Node->CanUserDeleteNode() == false) { continue; }
		
		return true;
	}
	return false;
}

void FPDMissionGraphEditor::SearchMissionTree() const
{
	TabManager->TryInvokeTab(FPDMissionEditorTabs::SearchID);
	SearchResults->FocusOnSearch();
}

bool FPDMissionGraphEditor::CanSearchMissionTree() const
{
	return true;
}

void FPDMissionGraphEditor::DeleteSelectedDuplicatableNodes()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr || Node->CanDuplicateNode() == false) { continue; }

		CurrentGraphEditor->SetNodeSelection(Node, true);
	}

	// Delete the duplicatable nodes
	DeleteNodes();

	CurrentGraphEditor->ClearSelectionSet();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr) { continue; }
		
		CurrentGraphEditor->SetNodeSelection(Node, true);
	}
}

void FPDMissionGraphEditor::CutNodes()
{
	CopyNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FPDMissionGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FPDMissionGraphEditor::CopyNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UPDMissionGraphNode*> SubNodes;

	FString ExportedText;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UPDMissionGraphNode* AINode = Cast<UPDMissionGraphNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();

		if (AINode == nullptr) { continue; }
		
		// append all sub-nodes for selection
		AINode->CopySubNodeIndex = CopySubNodeIndex;
		for (int32 Idx = 0; Idx < AINode->SubNodes.Num(); Idx++)
		{
			AINode->SubNodes[Idx]->CopySubNodeIndex = CopySubNodeIndex;
			SubNodes.Add(AINode->SubNodes[Idx]);
		}
		CopySubNodeIndex++;
	}

	for (int32 Idx = 0; Idx < SubNodes.Num(); Idx++)
	{
		SelectedNodes.Add(SubNodes[Idx]);
		SubNodes[Idx]->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UPDMissionGraphNode* Node = Cast<UPDMissionGraphNode>(*SelectedIter);
		if (Node == nullptr) { continue; }
		
		Node->PostCopyNode();
	}
}

bool FPDMissionGraphEditor::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		const UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node == nullptr || Node->CanDuplicateNode() == false) { continue; }

		return true;
	}
	return false;
}

void FPDMissionGraphEditor::PasteNodes()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }

	PasteNodesAtLocation(CurrentGraphEditor->GetPasteLocation());
}

void FPDMissionGraphEditor::PasteNodesAtLocation(const FVector2D& Location)
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	// Undo/Redo support
	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();
	UPDMissionGraph* MissionGraph = Cast<UPDMissionGraph>(EdGraph);

	EdGraph->Modify();
	if (MissionGraph != nullptr) { MissionGraph->LockUpdates(); }

	UPDMissionGraphNode* SelectedParent = nullptr;
	bool bHasMultipleNodesSelected = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UPDMissionGraphNode* Node = Cast<UPDMissionGraphNode>(*SelectedIter);
		Node = Node != nullptr && Node->IsSubNode() ? Node->ParentNode.Get() : Node;

		if (Node == nullptr) { continue; }
		
		if (SelectedParent == nullptr)
		{
			SelectedParent = Node;
			continue;
		}
		
		bHasMultipleNodesSelected = true;
		break;
	}

	// Clear the selection set (newly pasted stuff will be selected)
	CurrentGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(EdGraph, TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);
	
	// Number of nodes used to calculate AvgNodePosition
	int32 AvgCount = 0;

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* EdNode = *It;
		UPDMissionGraphNode* MissionNode = Cast<UPDMissionGraphNode>(EdNode);
		const bool bIsSubNode = (MissionNode != nullptr && MissionNode->IsSubNode());
		if (EdNode == nullptr || bIsSubNode) { continue; }
		
		AvgNodePosition.X += EdNode->NodePosX;
		AvgNodePosition.Y += EdNode->NodePosY;
		++AvgCount;
	}

	if (AvgCount > 0)
	{
		float InvNumNodes = 1.0f / static_cast<float>(AvgCount);
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	bool bPastedParentNode = false;

	TMap<FGuid/*New*/, FGuid/*Old*/> NewToOldNodeMapping;

	TMap<int32, UPDMissionGraphNode*> ParentMap;
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* PasteNode = *It;
		UPDMissionGraphNode* PasteMissionNode = Cast<UPDMissionGraphNode>(PasteNode);

		if (PasteNode && (PasteMissionNode == nullptr || PasteMissionNode->IsSubNode() == false))
		{
			bPastedParentNode = true;

			// Select the newly pasted stuff
			CurrentGraphEditor->SetNodeSelection(PasteNode, true);

			const FVector::FReal NodePosX = (PasteNode->NodePosX - AvgNodePosition.X) + Location.X;
			const FVector::FReal NodePosY = (PasteNode->NodePosY - AvgNodePosition.Y) + Location.Y;

			PasteNode->NodePosX = static_cast<int32>(NodePosX);
			PasteNode->NodePosY = static_cast<int32>(NodePosY);
			PasteNode->SnapToGrid(16);
			
			// Give new node a different Guid from the old one
			const FGuid OldGuid = PasteNode->NodeGuid;
			PasteNode->CreateNewGuid();

			const FGuid NewGuid = PasteNode->NodeGuid;
			NewToOldNodeMapping.Add(NewGuid, OldGuid);

			if (PasteMissionNode == nullptr) { continue; }
			
			PasteMissionNode->RemoveAllSubNodes();
			ParentMap.Add(PasteMissionNode->CopySubNodeIndex, PasteMissionNode);
		}
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UPDMissionGraphNode* PasteNode = Cast<UPDMissionGraphNode>(*It);
		if (PasteNode && PasteNode->IsSubNode())
		{
			PasteNode->NodePosX = 0;
			PasteNode->NodePosY = 0;

			// remove sub-node from graph, it will be referenced from parent node
			PasteNode->DestroyNode();

			PasteNode->ParentNode = ParentMap.FindRef(PasteNode->CopySubNodeIndex);
			if (PasteNode->ParentNode)
			{
				PasteNode->ParentNode->AddSubNode(PasteNode, EdGraph);
				continue;
			}
			
			if (bHasMultipleNodesSelected == false && bPastedParentNode == false && SelectedParent != nullptr)
			{
				PasteNode->ParentNode = SelectedParent;
				SelectedParent->AddSubNode(PasteNode, EdGraph);
			}
		}
	}

	FixupPastedNodes(PastedNodes, NewToOldNodeMapping);
	if (MissionGraph)
	{
		MissionGraph->UpdateClassData();
		MissionGraph->OnNodesPasted(TextToImport);
		MissionGraph->UnlockUpdates();
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();
	
	UObject* GraphOwner = EdGraph->GetOuter();
	if (GraphOwner == nullptr) { return; }
	
	GraphOwner->PostEditChange();
	GraphOwner->MarkPackageDirty();
}

void FPDMissionGraphEditor::FixupPastedNodes(const TSet<UEdGraphNode*>& PastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping)
{
	// @todo
}

void FPDMissionGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FPDMissionGraphEditor::InitMissionEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const FPDMissionGraph_NodeData& InData)
{
	MissionData = InData; // Root data to initialize the editor with


	const TSharedPtr<FPDMissionGraphEditor> ThisPtr(SharedThis(this));
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		// Register the document factories
		{
			const TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FPDMissionGraphEditorFactory(ThisPtr,
			                                                                                                          FPDMissionGraphEditorFactory::FOnCreateGraphEditorWidget::CreateSP(this, &FPDMissionGraphEditor::CreateGraphEditorWidget)
			));

			// Also store off a reference to the graph-editor factory so we can find all the tabs spawned by it later.
			GraphEditorTabFactoryPtr = GraphEditorFactory;
			DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
		}
	}

	TArray<UObject*> ObjectsToEdit;
	const FString CtxtStr = FString::Printf(TEXT("FPDMissionGraphEditor::InitMissionEditor - RequestedRootData: %s"), *MissionData.DataTarget.RowName.ToString());
	if(MissionData.DataTarget.GetRow<FPDMissionRow>(CtxtStr) != nullptr)
	{
		ObjectsToEdit.Add(const_cast<UDataTable*>(MissionData.DataTarget.DataTable.Get()));
	}

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FPDMissionEditorToolbar(SharedThis(this)));
	}

	FGraphEditorCommands::Register();
	FPDMissionEditorCommands::Register();
	// FPDMissionDebuggerCommands::Register(); @todo: debugger

	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor( Mode, InitToolkitHost, MissionEditorAppIdentifier, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsToEdit );

	BindCommands();
	ExtendMenu();
	CreateInternalWidgets();

	// // @todo: debugger
	// Debugger = MakeShareable(new FMissionDebugger);
	// Debugger->Setup(MissionData, SharedThis(this));
	// BindDebuggerToolbarCommands();

	AddApplicationMode(GraphViewMode, MakeShareable(new FMissionEditorApplicationMode_GraphView(SharedThis(this))));
	SetCurrentMode(GraphViewMode);

	OnClassListUpdated();
	RegenerateMenusAndToolbars();
}

TSharedRef<FPDMissionGraphEditor> FPDMissionGraphEditor::CreateMissionEditor(const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, const FPDMissionGraph_NodeData& InData)
{
	TSharedRef<FPDMissionGraphEditor> NewEditor(new FPDMissionGraphEditor());
	NewEditor->InitMissionEditor(Mode, InitToolkitHost, InData);
	return NewEditor;
}

FName FPDMissionGraphEditor::GetToolkitFName() const
{
	// @todo
	return NAME_None;
}

FText FPDMissionGraphEditor::GetBaseToolkitName() const
{
	// @todo
	return FText::GetEmpty();
}

FString FPDMissionGraphEditor::GetWorldCentricTabPrefix() const
{
	// @todo
	return "";
}

FLinearColor FPDMissionGraphEditor::GetWorldCentricTabColorScale() const
{
	// @todo
	return FLinearColor::Black;
}

FText FPDMissionGraphEditor::GetToolkitName() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitName();
}

FText FPDMissionGraphEditor::GetToolkitToolTipText() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitToolTipText();
}

void FPDMissionGraphEditor::FocusWindow(UObject* ObjectToFocusOn)
{
	FWorkflowCentricApplication::FocusWindow(ObjectToFocusOn);
}

void FPDMissionGraphEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// @todo
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
}

void FPDMissionGraphEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	if (Node == nullptr || Node->CanJumpToDefinition() == false) { return; }
	
	Node->JumpToDefinition();
}

void FPDMissionGraphEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	if (FocusGraphEditorPtr.Pin() != InGraphEditor)
	{
		FocusGraphEditorPtr = InGraphEditor;
		FocusedGraphEditorChanged.Broadcast();
	}

	FGraphPanelSelectionSet CurrentSelection;
	CurrentSelection = InGraphEditor->GetSelectedNodes();
	OnSelectedNodesChanged(CurrentSelection);	
}

void FPDMissionGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged == nullptr) { return; }
	
	const FScopedTransaction Transaction(LOCTEXT("MissionRenameNode", "Rename Node"));
	NodeBeingChanged->Modify();
	NodeBeingChanged->OnRenameNode(NewText.ToString());
}

FGraphAppearanceInfo FPDMissionGraphEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "MISSION");

	// @todo apply debugger appearance
	
	return AppearanceInfo;
}

bool FPDMissionGraphEditor::InEditingMode(bool bGraphIsEditable) const
{
	return bGraphIsEditable; // && FPDMissionDebugger::IsPIENotSimulating(); // @todo: debugger
}

EVisibility FPDMissionGraphEditor::GetDebuggerDetailsVisibility() const
{
	// @todo: debugger
	return /* Debugger.IsValid() && Debugger->IsDebuggerRunning() ? EVisibility::Visible : */ EVisibility::Collapsed;
}

TWeakPtr<SGraphEditor> FPDMissionGraphEditor::GetFocusedGraphPtr() const
{
	return FocusGraphEditorPtr;
}

FText FPDMissionGraphEditor::GetLocalizedMode(FName InMode)
{
	static TMap< FName, FText > LocModes;
	if (LocModes.Num() == 0)
	{
		LocModes.Add(GraphViewMode, LOCTEXT("GraphVieWMode", "Graph") );
		LocModes.Add(TreeViewMode, LOCTEXT("TreeViewMode", "Tree View") );
	}
	check( InMode != NAME_None );
	
	const FText* OutDesc = LocModes.Find( InMode );
	check( OutDesc );
	
	return *OutDesc;
}

FPDMissionGraph_NodeData &FPDMissionGraphEditor::GetMissionData() const
{
	// @todo
	return const_cast<FPDMissionGraph_NodeData&>(MissionData);
}

TSharedRef<SWidget> FPDMissionGraphEditor::SpawnProperties()
{
	return
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.HAlign(HAlign_Fill)
		.FillHeight(1.0f)
		[
			DetailsView.ToSharedRef()
		];
}

TSharedRef<SWidget> FPDMissionGraphEditor::SpawnSearch()
{
	// SearchResults = SNew(SSearchInMission, SharedThis(this));  // @todo SSearchInMission
	return SearchResults.ToSharedRef();
}

TSharedRef<SWidget> FPDMissionGraphEditor::SpawnMissionTree()
{
	// TreeEditor = SNew(SMissionTreeEditor, SharedThis(this));  // @todo SMissionTreeEditor
	return TreeEditor.ToSharedRef();
}

void FPDMissionGraphEditor::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FPDMissionGraphEditor::RegenerateMissionGraph()
{
	// @todo	
}

void FPDMissionGraphEditor::SaveEditedObjectState()
{
	// Clear currently edited documents
	MissionData.LastEditedDocuments.Empty();
	// Ask all open documents to save their state, which will update LastEditedDocuments
	DocumentManager->SaveAllState();
}

bool FPDMissionGraphEditor::CanCreateNewMissionNodes() const
{
	// @todo
	return true;
}

class FPDMissionNodeFilter : public IStructViewerFilter
{
public:
	FPDMissionNodeFilter( UScriptStruct* InBaseClass)
		: BaseClass(InBaseClass)
	{
	}

	virtual bool IsStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const UScriptStruct* InStruct, TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
	{
		if (InStruct == nullptr) { return false; }
		
		return InStruct->IsChildOf(BaseClass);
	}

	virtual bool IsUnloadedStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const FSoftObjectPath& InStructPath, TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
	{
		return true;
	}

private:
	 UScriptStruct* BaseClass;
};

TSharedRef<SWidget> FPDMissionGraphEditor::HandleCreateNewStructMenu(UScriptStruct* BaseStruct) const
{
	FStructViewerInitializationOptions Options;
	Options.bShowUnloadedStructs = true;
	Options.StructFilter = MakeShareable(new FPDMissionNodeFilter(BaseStruct));

	const FOnStructPicked OnPicked(FOnStructPicked::CreateSP(this, &FPDMissionGraphEditor::HandleNewNodeStructPicked));
	return FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer").CreateStructViewer(Options, OnPicked);
}

void FPDMissionGraphEditor::HandleNewNodeStructPicked(const UScriptStruct* BasesStruct) const
{
	// @todo
}

#define MMapDebugCommand(Token) \
GraphEditorCommands->MapAction( FGraphEditorCommands::Get(). Token, \
	FExecuteAction::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::On##Token##), \
	FCanExecuteAction::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::Can##Token## ), \
	FIsActionChecked(), \
	FIsActionButtonVisible::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::Can##Token## ) \
	);

TSharedRef<SGraphEditor> FPDMissionGraphEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph != NULL);
	
	if (!GraphEditorCommands.IsValid())
	{
		CreateCommandList();

		// Debug actions
		MMapDebugCommand(AddBreakpoint);
		MMapDebugCommand(RemoveBreakpoint);
		MMapDebugCommand(EnableBreakpoint);
		MMapDebugCommand(DisableBreakpoint);
		MMapDebugCommand(ToggleBreakpoint);
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FPDMissionGraphEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FPDMissionGraphEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FPDMissionGraphEditor::OnNodeTitleCommitted);

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget = 
		SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush( TEXT("Graph.TitleBackground") ) )
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ConversationGraphLabel", "Conversation Editor"))
				.TextStyle( FAppStyle::Get(), TEXT("GraphBreadcrumbButtonText") )
			]
		];

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FPDMissionGraphEditor::InEditingMode, bGraphIsEditable)
		.Appearance(this, &FPDMissionGraphEditor::GetGraphAppearance)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);
}

bool FPDMissionGraphEditor::IsPropertyEditable() const
{
	if (bForceDisablePropertyEdit /* || FPDMissionDebugger::IsPIESimulating() */ )
	{
		return false;
	}

	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	return FocusedGraphEd.IsValid() && FocusedGraphEd->GetCurrentGraph() && FocusedGraphEd->GetCurrentGraph()->bEditable;
}

void FPDMissionGraphEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	if (FocusedGraphEd.IsValid() && FocusedGraphEd->GetCurrentGraph())
	{
		FocusedGraphEd->GetCurrentGraph()->GetSchema()->ForceVisualizationCacheClear();
	}

	// @todo: Review/remove/move
	// FPDMissionBuilder::RebuildBank(MissionData);
}

void FPDMissionGraphEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	
	DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );
	DetailsView->SetObject( nullptr );
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FPDMissionGraphEditor::IsPropertyEditable));
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FPDMissionGraphEditor::OnFinishedChangingProperties);

	//DetailsView->RegisterInstancedCustomPropertyTypeLayout(TEXT("ConversationNodeHandle"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FConversationNodeReferenceCustomization::MakeInstance, TWeakPtr<FConversationEditor>(SharedThis(this))), nullptr);
}

void FPDMissionGraphEditor::ExtendMenu()
{
	// @todo
}

void FPDMissionGraphEditor::BindCommands()
{
	ToolkitCommands->MapAction(FPDMissionEditorCommands::Get().SearchMissionTree,
			FExecuteAction::CreateSP(this, &FPDMissionGraphEditor::SearchMissionTree),
			FCanExecuteAction::CreateSP(this, &FPDMissionGraphEditor::CanSearchMissionTree)
			);
}

bool FPDMissionGraphEditor::CanPasteNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return false; }

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FPDMissionGraphEditor::DuplicateNodes()
{
	CopyNodes();
	PasteNodes();
}

bool FPDMissionGraphEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

bool FPDMissionGraphEditor::CanCreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
 	return CurrentGraphEditor.IsValid() ? (CurrentGraphEditor->GetNumberOfSelectedNodes() != 0) : false;
}

void FPDMissionGraphEditor::CreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	UEdGraph* EdGraph = CurrentGraphEditor.IsValid() ? CurrentGraphEditor->GetCurrentGraph() : nullptr;
	if (EdGraph == nullptr) { return; }

	const TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction();
	if (Action.IsValid() == false) { return; }
	
	Action->PerformAction(EdGraph, nullptr, FVector2D());
}

void FPDMissionGraphEditor::OnClassListUpdated()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	UPDMissionGraph* MissionGraph = Cast<UPDMissionGraph>(CurrentGraphEditor->GetCurrentGraph());
	if (MissionGraph == nullptr) { return; }
	
	const bool bUpdated = MissionGraph->UpdateUnknownNodeClasses();
	if (bUpdated == false) { return; }

	const FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
	OnSelectedNodesChanged(CurrentSelection);
	MissionGraph->UpdateData();
}


#undef LOCTEXT_NAMESPACE
