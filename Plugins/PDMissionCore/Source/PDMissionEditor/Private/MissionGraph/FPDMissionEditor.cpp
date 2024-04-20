/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */


#include "MissionGraph/FPDMissionEditor.h"

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

const FName FFPDMissionGraphEditor::GraphViewMode("GraphViewMode");
const FName FFPDMissionGraphEditor::TreeViewMode("TreeViewMode");

const FName MissionEditorAppIdentifier("MissionEditorApp");

FFPDMissionGraphEditor::FFPDMissionGraphEditor()
{
	MissionData.DataTarget = FDataTableRowHandle{};
	bCheckDirtyOnAssetSave = true;
	
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor != nullptr) { Editor->RegisterForUndo(this); }
}


FFPDMissionGraphEditor::~FFPDMissionGraphEditor()
{
	// Debugger.Reset(); @todo : debugger
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor) { Editor->UnregisterForUndo(this); }
}

#define MGenericMapAction(Token) \
	MapAction(FGenericCommands::Get(). Token, \
		FExecuteAction::CreateRaw(this, &FFPDMissionGraphEditor:: Token##Nodes), \
		FCanExecuteAction::CreateRaw(this, &FFPDMissionGraphEditor::Can##Token##Nodes) \
		);

#define MGEMapAction(Token) \
	MapAction(FGraphEditorCommands::Get(). Token, \
		FExecuteAction::CreateRaw(this, &FFPDMissionGraphEditor:: Token ), \
		FCanExecuteAction::CreateRaw(this, &FFPDMissionGraphEditor::Can##Token ) \
		);	

void FFPDMissionGraphEditor::CreateCommandList()
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

FGraphPanelSelectionSet FFPDMissionGraphEditor::GetSelectedNodes() const
{
	const TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	if (FocusedGraphEd == nullptr) { return FGraphPanelSelectionSet{}; }

	return FocusedGraphEd->GetSelectedNodes();
}

void FFPDMissionGraphEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	// @todo
}

void FFPDMissionGraphEditor::PostUndo(bool bSuccess)
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

void FFPDMissionGraphEditor::PostRedo(bool bSuccess)
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

void FFPDMissionGraphEditor::SelectAllNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }
	
	CurrentGraphEditor->SelectAllNodes();
}

bool FFPDMissionGraphEditor::CanSelectAllNodes() const
{
	return true;
}

void FFPDMissionGraphEditor::DeleteNodes() const
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

bool FFPDMissionGraphEditor::CanDeleteNodes() const
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

void FFPDMissionGraphEditor::SearchMissionTree() const
{
	TabManager->TryInvokeTab(FPDMissionEditorTabs::SearchID);
	SearchResults->FocusOnSearch();
}

bool FFPDMissionGraphEditor::CanSearchMissionTree() const
{
	return true;
}

void FFPDMissionGraphEditor::DeleteSelectedDuplicatableNodes()
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

void FFPDMissionGraphEditor::CutNodes()
{
	CopyNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FFPDMissionGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FFPDMissionGraphEditor::CopyNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UPDMissionGraphNode*> SubNodes;

	FString ExportedText;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UPDMissionGraphNode* MissionGraphNode = Cast<UPDMissionGraphNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();

		if (MissionGraphNode == nullptr) { continue; }
		
		// append all sub-nodes for selection
		MissionGraphNode->CopySubNodeIndex = CopySubNodeIndex;
		for (int32 Idx = 0; Idx < MissionGraphNode->SubNodes.Num(); Idx++)
		{
			MissionGraphNode->SubNodes[Idx]->CopySubNodeIndex = CopySubNodeIndex;
			SubNodes.Add(MissionGraphNode->SubNodes[Idx]);
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

bool FFPDMissionGraphEditor::CanCopyNodes() const
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

void FFPDMissionGraphEditor::PasteNodes()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }

	PasteNodesAtLocation(CurrentGraphEditor->GetPasteLocation());
}

void FFPDMissionGraphEditor::PasteNodesAtLocation(const FVector2D& Location)
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

	// Skip sub-nodes
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* PasteNode = *It;
		UPDMissionGraphNode* PasteMissionNode = Cast<UPDMissionGraphNode>(PasteNode);
		
		const bool bIsSubNode = (PasteMissionNode != nullptr && PasteMissionNode->IsSubNode());
		if (PasteNode == nullptr || bIsSubNode) { return; }
		
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

	// Iterate sub-nodes
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UPDMissionGraphNode* PasteNode = Cast<UPDMissionGraphNode>(*It);
		if (PasteNode == nullptr || PasteNode->IsSubNode() == false) { continue; }

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
			
		if (bHasMultipleNodesSelected || bPastedParentNode || SelectedParent == nullptr) { continue; }
		PasteNode->ParentNode = SelectedParent;
		SelectedParent->AddSubNode(PasteNode, EdGraph);
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

void FFPDMissionGraphEditor::FixupPastedNodes(const TSet<UEdGraphNode*>& PastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping)
{
	// @todo
}

void FFPDMissionGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FFPDMissionGraphEditor::InitMissionEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const FPDMissionNodeHandle& InData)
{
	MissionData = InData; // Root data to initialize the editor with

	const TSharedPtr<FFPDMissionGraphEditor> ThisPtr(SharedThis(this));
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		// Register the document factories
		{
			const TSharedRef<FDocumentTabFactory> GraphEditorFactory =
				MakeShareable(new FPDMissionGraphEditorFactory(
					ThisPtr,
					FPDMissionGraphEditorFactory::FOnCreateGraphEditorWidget
						::CreateSP(this, &FFPDMissionGraphEditor::CreateGraphEditorWidget)
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

	if (ObjectsToEdit.IsEmpty())
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

void FFPDMissionGraphEditor::InitMissionEditor(const FPDMissionNodeHandle& InData)
{
	MissionData = InData; // Root data to initialize the editor with

	const TSharedPtr<FFPDMissionGraphEditor> ThisPtr(SharedThis(this));
	if (DocumentManager.IsValid() == false) 
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		// Register the document factories
		{
			const TSharedRef<FDocumentTabFactory> GraphEditorFactory =
				MakeShareable(new FPDMissionGraphEditorFactory(
					ThisPtr,
					FPDMissionGraphEditorFactory::FOnCreateGraphEditorWidget
						::CreateSP(this, &FFPDMissionGraphEditor::CreateGraphEditorWidget)
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

	if (ObjectsToEdit.IsEmpty())
	{
		ObjectsToEdit.Add(const_cast<UDataTable*>(MissionData.DataTarget.DataTable.Get()));
	}

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FPDMissionEditorToolbar(SharedThis(this)));
	}

	FGraphEditorCommands::Register();
	FPDMissionEditorCommands::Register();
	
	const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
	constexpr bool bCreateDefaultStandaloneMenu = true;
	constexpr bool bCreateDefaultToolbar = true;
	InitAssetEditor(EToolkitMode::Type::Standalone, TSharedPtr<IToolkitHost>(), MissionEditorAppIdentifier, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsToEdit );

	BindCommands();
	ExtendMenu();
	CreateInternalWidgets();

	AddApplicationMode(GraphViewMode, MakeShareable(new FMissionEditorApplicationMode_GraphView(SharedThis(this))));
	SetCurrentMode(GraphViewMode);

	OnClassListUpdated();
	RegenerateMenusAndToolbars();	
}

TSharedRef<FFPDMissionGraphEditor> FFPDMissionGraphEditor::CreateMissionEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, const FPDMissionNodeHandle& InData)
{
	TSharedRef<FFPDMissionGraphEditor> NewEditor(new FFPDMissionGraphEditor());
	NewEditor->InitMissionEditor(Mode, InitToolkitHost, InData);
	return NewEditor;
}

TSharedRef<FFPDMissionGraphEditor> FFPDMissionGraphEditor::CreateMissionEditor(const FPDMissionNodeHandle& InData)
{
	TSharedRef<FFPDMissionGraphEditor> NewEditor(new FFPDMissionGraphEditor());
	NewEditor->InitMissionEditor(InData);
	return NewEditor;	
}

FName FFPDMissionGraphEditor::GetToolkitFName() const
{
	// @todo
	return NAME_None;
}

FText FFPDMissionGraphEditor::GetBaseToolkitName() const
{
	// @todo
	return FText::GetEmpty();
}

FString FFPDMissionGraphEditor::GetWorldCentricTabPrefix() const
{
	// @todo
	return "";
}

FLinearColor FFPDMissionGraphEditor::GetWorldCentricTabColorScale() const
{
	// @todo
	return FLinearColor::Black;
}

FText FFPDMissionGraphEditor::GetToolkitName() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitName();
}

FText FFPDMissionGraphEditor::GetToolkitToolTipText() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitToolTipText();
}

void FFPDMissionGraphEditor::FocusWindow(UObject* ObjectToFocusOn)
{
	FWorkflowCentricApplication::FocusWindow(ObjectToFocusOn);
}

void FFPDMissionGraphEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// @todo
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
}

void FFPDMissionGraphEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	if (Node == nullptr || Node->CanJumpToDefinition() == false) { return; }
	
	Node->JumpToDefinition();
}

void FFPDMissionGraphEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	if (FocusGraphEditorPtr.Pin() != InGraphEditor)
	{
		FocusGraphEditorPtr = InGraphEditor;
		FocusedGraphEditorChanged.Broadcast();
	}

	const FGraphPanelSelectionSet CurrentSelection = InGraphEditor->GetSelectedNodes();
	OnSelectedNodesChanged(CurrentSelection);	
}

void FFPDMissionGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged == nullptr) { return; }
	
	const FScopedTransaction Transaction(LOCTEXT("MissionRenameNode", "Rename Node"));
	NodeBeingChanged->Modify();
	NodeBeingChanged->OnRenameNode(NewText.ToString());
}

FGraphAppearanceInfo FFPDMissionGraphEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "MISSION");

	// @todo apply debugger appearance
	
	return AppearanceInfo;
}

bool FFPDMissionGraphEditor::InEditingMode(bool bGraphIsEditable) const
{
	return bGraphIsEditable; // && FPDMissionDebugger::IsPIENotSimulating(); // @todo: debugger
}

EVisibility FFPDMissionGraphEditor::GetDebuggerDetailsVisibility() const
{
	// @todo: debugger
	return /* Debugger.IsValid() && Debugger->IsDebuggerRunning() ? EVisibility::Visible : */ EVisibility::Collapsed;
}

TWeakPtr<SGraphEditor> FFPDMissionGraphEditor::GetFocusedGraphPtr() const
{
	return FocusGraphEditorPtr;
}

FText FFPDMissionGraphEditor::GetLocalizedMode(FName InMode)
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

FPDMissionNodeHandle &FFPDMissionGraphEditor::GetMissionData() const
{
	// @todo
	return const_cast<FPDMissionNodeHandle&>(MissionData);
}

TSharedRef<SWidget> FFPDMissionGraphEditor::SpawnProperties() const
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

TSharedRef<SWidget> FFPDMissionGraphEditor::SpawnSearch()
{
	SearchResults = SNew(SSearchInMission, SharedThis(this)); 
	return SearchResults.ToSharedRef();
}

TSharedRef<SWidget> FFPDMissionGraphEditor::SpawnMissionTree()
{
	TreeEditor = SNew(SMissionTreeEditor, SharedThis(this)); 
	return TreeEditor.ToSharedRef();
}

void FFPDMissionGraphEditor::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FFPDMissionGraphEditor::RegenerateMissionGraph()
{
	// @todo	
}

void FFPDMissionGraphEditor::SaveEditedObjectState()
{
	// Clear currently edited documents
	MissionData.LastEditedDocuments.Empty();
	// Ask all open documents to save their state, which will update LastEditedDocuments
	DocumentManager->SaveAllState();
}

bool FFPDMissionGraphEditor::CanCreateNewMissionNodes() const
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

	virtual bool IsStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const UScriptStruct* InStruct, TSharedRef<FStructViewerFilterFuncs> InFilterFunctions) override
	{
		if (InStruct == nullptr) { return false; }
		
		return InStruct->IsChildOf(BaseClass);
	}

	virtual bool IsUnloadedStructAllowed(const FStructViewerInitializationOptions& InInitOptions, const FSoftObjectPath& InStructPath, TSharedRef<FStructViewerFilterFuncs> InFilterFunctions) override
	{
		return true;
	}

private:
	 UScriptStruct* BaseClass;
};

TSharedRef<SWidget> FFPDMissionGraphEditor::HandleCreateNewStructMenu(UScriptStruct* BaseStruct) const
{
	FStructViewerInitializationOptions Options;
	Options.bShowUnloadedStructs = true;
	Options.StructFilter = MakeShareable(new FPDMissionNodeFilter(BaseStruct));

	const FOnStructPicked OnPicked(FOnStructPicked::CreateSP(this, &FFPDMissionGraphEditor::HandleNewNodeStructPicked));
	return FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer").CreateStructViewer(Options, OnPicked);
}

void FFPDMissionGraphEditor::HandleNewNodeStructPicked(const UScriptStruct* BasesStruct) const
{
	// @todo
}

#define MMapDebugCommand(Token) \
GraphEditorCommands->MapAction( FGraphEditorCommands::Get(). Token, \
	FExecuteAction::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::On##Token ), \
	FCanExecuteAction::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::Can##Token ), \
	FIsActionChecked(), \
	FIsActionButtonVisible::CreateSP(&DebugHandler, &FPDMissionDebuggerHandler::Can##Token ) \
	);

TSharedRef<SGraphEditor> FFPDMissionGraphEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph != NULL);
	
	if (GraphEditorCommands.IsValid() == false)
	{
		CreateCommandList();

		// // Debug actions, FPDMissionDebuggerHandler failing to create a valid ThisPtr @todo fix when it becomes relevant again
		// MMapDebugCommand(AddBreakpoint);
		// MMapDebugCommand(RemoveBreakpoint);
		// MMapDebugCommand(EnableBreakpoint);
		// MMapDebugCommand(DisableBreakpoint);
		// MMapDebugCommand(ToggleBreakpoint);
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FFPDMissionGraphEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FFPDMissionGraphEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FFPDMissionGraphEditor::OnNodeTitleCommitted);

	// Make title bar
	const TSharedRef<SWidget> TitleBarWidget = 
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
				.Text(LOCTEXT("MissionGraphLabel", "Mission Editor"))
				.TextStyle( FAppStyle::Get(), TEXT("GraphBreadcrumbButtonText") )
			]
		];

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FFPDMissionGraphEditor::InEditingMode, bGraphIsEditable)
		.Appearance(this, &FFPDMissionGraphEditor::GetGraphAppearance)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);
}

bool FFPDMissionGraphEditor::IsPropertyEditable() const
{
	if (bForceDisablePropertyEdit /* || FPDMissionDebugger::IsPIESimulating() */ )
	{
		return false;
	}

	const TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	return FocusedGraphEd.IsValid() && FocusedGraphEd->GetCurrentGraph() && FocusedGraphEd->GetCurrentGraph()->bEditable;
}

void FFPDMissionGraphEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	const TSharedPtr<SGraphEditor> FocusedGraphEd = FocusGraphEditorPtr.Pin();
	if (FocusedGraphEd.IsValid() && FocusedGraphEd->GetCurrentGraph())
	{
		FocusedGraphEd->GetCurrentGraph()->GetSchema()->ForceVisualizationCacheClear();
	}

	// @todo: Review/remove/move
	// FPDMissionBuilder::RebuildData(MissionData);
}

void FFPDMissionGraphEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>( "PropertyEditor" );
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	
	DetailsView = PropertyEditorModule.CreateDetailView( DetailsViewArgs );
	DetailsView->SetObject( nullptr );
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FFPDMissionGraphEditor::IsPropertyEditable));
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FFPDMissionGraphEditor::OnFinishedChangingProperties);
}

void FFPDMissionGraphEditor::ExtendMenu()
{
	// @todo
}

void FFPDMissionGraphEditor::BindCommands()
{
	ToolkitCommands->MapAction(FPDMissionEditorCommands::Get().SearchMissionTree,
			FExecuteAction::CreateSP(this, &FFPDMissionGraphEditor::SearchMissionTree),
			FCanExecuteAction::CreateSP(this, &FFPDMissionGraphEditor::CanSearchMissionTree)
			);
}

bool FFPDMissionGraphEditor::CanPasteNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return false; }

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FFPDMissionGraphEditor::DuplicateNodes()
{
	CopyNodes();
	PasteNodes();
}

bool FFPDMissionGraphEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

bool FFPDMissionGraphEditor::CanCreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
 	return CurrentGraphEditor.IsValid() ? (CurrentGraphEditor->GetNumberOfSelectedNodes() != 0) : false;
}

void FFPDMissionGraphEditor::CreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = FocusGraphEditorPtr.Pin();
	UEdGraph* EdGraph = CurrentGraphEditor.IsValid() ? CurrentGraphEditor->GetCurrentGraph() : nullptr;
	if (EdGraph == nullptr) { return; }

	const TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction();
	if (Action.IsValid() == false) { return; }
	
	Action->PerformAction(EdGraph, nullptr, FVector2D());
}

void FFPDMissionGraphEditor::OnClassListUpdated()
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