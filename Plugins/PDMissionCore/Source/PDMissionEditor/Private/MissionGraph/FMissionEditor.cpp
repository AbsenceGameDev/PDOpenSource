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


#include "MissionGraph/FMissionEditor.h"


// Copyright Epic Games, Inc. All Rights Reserved.
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Editor/EditorEngine.h"
#include "EngineGlobals.h"
#include "MissionGraph/MissionGraph.h"
#include "MissionGraphTypes.h"
#include "MissionGraph/MissionGraphNode.h"
#include "ScopedTransaction.h"
#include "EdGraphUtilities.h"
#include "Framework/Commands/GenericCommands.h"
#include "GraphEditorActions.h"
#include "HAL/PlatformApplicationMisc.h"
#include "EdGraph/EdGraphSchema.h"

#define LOCTEXT_NAMESPACE "AIGraph"

FMissionGraphEditor::FMissionGraphEditor()
{
	MissionTable = nullptr;
	bCheckDirtyOnAssetSave = true;
	
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor != nullptr) { Editor->RegisterForUndo(this); }

	OnClassListUpdatedDelegateHandle =
		FMissionNodeClassHelper::OnPackageListUpdated.AddRaw(this, &FMissionGraphEditor::OnClassListUpdated);
}


FMissionGraphEditor::~FMissionGraphEditor()
{
	Debugger.Reset();
	UEditorEngine* Editor = static_cast<UEditorEngine*>(GEngine);
	if (Editor) { Editor->UnregisterForUndo(this); }

	FMissionNodeClassHelper::OnPackageListUpdated.Remove(OnClassListUpdatedDelegateHandle);
}

#define MGenericMapAction(Token) \
	MapAction(FGenericCommands::Get().##Token, \
		FExecuteAction::CreateRaw(this, &FMissionGraphEditor::##Token##Nodes), \
		FCanExecuteAction::CreateRaw(this, &FMissionGraphEditor::Can##Token##Nodes) \
		);

#define MGEMapAction(Token) \
	MapAction(FGraphEditorCommands::Get().##Token, \
		FExecuteAction::CreateRaw(this, &FMissionGraphEditor::##Token), \
		FCanExecuteAction::CreateRaw(this, &FMissionGraphEditor::Can##Token) \
		);	

void FMissionGraphEditor::CreateCommandList()
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

FGraphPanelSelectionSet FMissionGraphEditor::GetSelectedNodes() const
{
	const TSharedPtr<SGraphEditor> FocusedGraphEd = UpdateGraphEdPtr.Pin();
	if (FocusedGraphEd == nullptr) { return FGraphPanelSelectionSet{}; }

	return FocusedGraphEd->GetSelectedNodes();
}

void FMissionGraphEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	// @todo
}

void FMissionGraphEditor::PostUndo(bool bSuccess)
{
	if (bSuccess == false) { return; }

	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor == nullptr)
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}
	
	// Clear selection and avoid references to nodes that will be removed
	CurrentGraphEditor->ClearSelectionSet();
	CurrentGraphEditor->NotifyGraphChanged();
}

void FMissionGraphEditor::PostRedo(bool bSuccess)
{
	if (bSuccess == false) { return; }

	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor == nullptr)
	{
		FSlateApplication::Get().DismissAllMenus();
		return;
	}
	
	// Clear selection and avoid references to nodes that will be removed
	CurrentGraphEditor->ClearSelectionSet();
	CurrentGraphEditor->NotifyGraphChanged();
}

void FMissionGraphEditor::SelectAllNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }
	
	CurrentGraphEditor->SelectAllNodes();
}

bool FMissionGraphEditor::CanSelectAllNodes() const
{
	return true;
}

void FMissionGraphEditor::DeleteNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
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

bool FMissionGraphEditor::CanDeleteNodes() const
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

void FMissionGraphEditor::DeleteSelectedDuplicatableNodes()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
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

void FMissionGraphEditor::CutNodes()
{
	CopyNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FMissionGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FMissionGraphEditor::CopyNodes() const
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	TArray<UMissionGraphNode*> SubNodes;

	FString ExportedText;

	int32 CopySubNodeIndex = 0;
	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UMissionGraphNode* AINode = Cast<UMissionGraphNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();

		if (AINode == nullptr) { continue; }
		
		// append all subnodes for selection
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
		UMissionGraphNode* Node = Cast<UMissionGraphNode>(*SelectedIter);
		if (Node == nullptr) { continue; }
		
		Node->PostCopyNode();
	}
}

bool FMissionGraphEditor::CanCopyNodes() const
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

void FMissionGraphEditor::PasteNodes()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor == nullptr) { return; }

	PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
}

void FMissionGraphEditor::PasteNodesHere(const FVector2D& Location)
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	// Undo/Redo support
	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	UEdGraph* EdGraph = CurrentGraphEditor->GetCurrentGraph();
	UMissionGraph* MissionGraph = Cast<UMissionGraph>(EdGraph);

	EdGraph->Modify();
	if (MissionGraph != nullptr) { MissionGraph->LockUpdates(); }

	UMissionGraphNode* SelectedParent = nullptr;
	bool bHasMultipleNodesSelected = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UMissionGraphNode* Node = Cast<UMissionGraphNode>(*SelectedIter);
		Node = Node != nullptr && Node->IsSubNode() ? Node->ParentNode : Node;

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
		UMissionGraphNode* MissionNode = Cast<UMissionGraphNode>(EdNode);
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

	TMap<int32, UMissionGraphNode*> ParentMap;
	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* PasteNode = *It;
		UMissionGraphNode* PasteMissionNode = Cast<UMissionGraphNode>(PasteNode);

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
		UMissionGraphNode* PasteNode = Cast<UMissionGraphNode>(*It);
		if (PasteNode && PasteNode->IsSubNode())
		{
			PasteNode->NodePosX = 0;
			PasteNode->NodePosY = 0;

			// remove subnode from graph, it will be referenced from parent node
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

void FMissionGraphEditor::FixupPastedNodes(const TSet<UEdGraphNode*>& PastedGraphNodes, const TMap<FGuid/*New*/, FGuid/*Old*/>& NewToOldNodeMapping)
{
	// @todo
}

void FMissionGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FMissionGraphEditor::InitMissionEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost,
	UObject* InObject)
{
	// @todo
}

TSharedRef<FMissionGraphEditor> FMissionGraphEditor::CreateMissionEditor(const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* Object)
{
	return TSharedRef<FMissionGraphEditor>{}; // @todo
}

FName FMissionGraphEditor::GetToolkitFName() const
{
	// @todo
	return NAME_None;
}

FText FMissionGraphEditor::GetBaseToolkitName() const
{
	// @todo
	return FText::GetEmpty();
}

FString FMissionGraphEditor::GetWorldCentricTabPrefix() const
{
	// @todo
	return "";
}

FLinearColor FMissionGraphEditor::GetWorldCentricTabColorScale() const
{
	// @todo
	return FLinearColor::Black;
}

FText FMissionGraphEditor::GetToolkitName() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitName();
}

FText FMissionGraphEditor::GetToolkitToolTipText() const
{
	// @todo
	return FWorkflowCentricApplication::GetToolkitToolTipText();
}

void FMissionGraphEditor::FocusWindow(UObject* ObjectToFocusOn)
{
	// @todo
	FWorkflowCentricApplication::FocusWindow(ObjectToFocusOn);
}

void FMissionGraphEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// @todo
	FNotifyHook::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
}

void FMissionGraphEditor::OnNodeDoubleClicked(UEdGraphNode* Node)
{
	// @todo
}

void FMissionGraphEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	// @todo
}

void FMissionGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	// @todo
}

FGraphAppearanceInfo FMissionGraphEditor::GetGraphAppearance() const
{
	// @todo
	return FGraphAppearanceInfo{};
}

bool FMissionGraphEditor::InEditingMode(bool bGraphIsEditable) const
{
	// @todo
	return false;
}

EVisibility FMissionGraphEditor::GetDebuggerDetailsVisibility() const
{
	// @todo
	return EVisibility::Visible;
}

EVisibility FMissionGraphEditor::GetInjectedNodeVisibility() const
{
	// @todo
	return EVisibility::Visible;
}

TWeakPtr<SGraphEditor> FMissionGraphEditor::GetFocusedGraphPtr() const
{
	// @todo
	return nullptr;
}

FText FMissionGraphEditor::GetLocalizedMode(FName InMode)
{
	// @todo
	return FText::GetEmpty();
}

UDataTable* FMissionGraphEditor::GetMissionData() const
{
	// @todo
	return MissionTable;
}

TSharedRef<SWidget> FMissionGraphEditor::SpawnProperties()
{
	// @todo
	return SNew(SGraphEditor);
}

TSharedRef<SWidget> FMissionGraphEditor::SpawnSearch()
{
	// @todo
	return SNew(SGraphEditor);
}

TSharedRef<SWidget> FMissionGraphEditor::SpawnMissionTree()
{
	// @todo
	return SNew(SGraphEditor);
}

void FMissionGraphEditor::RegisterToolbarTab(const TSharedRef<FTabManager>& InTabManager)
{
	// @todo
}

void FMissionGraphEditor::RegenerateMissionGraph()
{
	// @todo
}

void FMissionGraphEditor::SaveEditedObjectState()
{
	// @todo
}

bool FMissionGraphEditor::CanCreateNewNodeClasses() const
{
	// @todo
	return true;
}

class FMissionNodeClassFilter : public IClassViewerFilter
{
public:
	FMissionNodeClassFilter(UClass* InBaseClass)
		: BaseClass(InBaseClass)
	{
	}

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		if (InClass == nullptr) { return false; }
		
		return InClass->IsChildOf(BaseClass);
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return InUnloadedClassData->IsChildOf(BaseClass);
	}

private:
	UClass* BaseClass;
};

TSharedRef<SWidget> FMissionGraphEditor::HandleCreateNewClassMenu(UClass* BaseClass) const
{
	FClassViewerInitializationOptions Options;
	Options.bShowUnloadedBlueprints = true;
	Options.ClassFilters.Add(MakeShareable( new FMissionNodeClassFilter(BaseClass) ));

	const FOnClassPicked OnPicked( FOnClassPicked::CreateSP( this, &FMissionGraphEditor::HandleNewNodeClassPicked ) );
	return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer").CreateClassViewer(Options, OnPicked);
}

void FMissionGraphEditor::HandleNewNodeClassPicked(UClass* InClass) const
{
	// @todo
}

TSharedRef<SGraphEditor> FMissionGraphEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	// @todo
	return SNew(SGraphEditor);
}

void FMissionGraphEditor::CreateInternalWidgets()
{
	// @todo
}

void FMissionGraphEditor::ExtendMenu()
{
	// @todo
}

void FMissionGraphEditor::BindCommonCommands()
{
	// @todo
}

bool FMissionGraphEditor::CanPasteNodes() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return false; }

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FMissionGraphEditor::DuplicateNodes()
{
	CopyNodes();
	PasteNodes();
}

bool FMissionGraphEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

bool FMissionGraphEditor::CanCreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
 	return CurrentGraphEditor.IsValid() ? (CurrentGraphEditor->GetNumberOfSelectedNodes() != 0) : false;
}

void FMissionGraphEditor::CreateComment() const
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	UEdGraph* EdGraph = CurrentGraphEditor.IsValid() ? CurrentGraphEditor->GetCurrentGraph() : nullptr;
	if (EdGraph == nullptr) { return; }

	const TSharedPtr<FEdGraphSchemaAction> Action = EdGraph->GetSchema()->GetCreateCommentAction();
	if (Action.IsValid() == false) { return; }
	
	Action->PerformAction(EdGraph, nullptr, FVector2D());
}

void FMissionGraphEditor::OnClassListUpdated()
{
	const TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (CurrentGraphEditor.IsValid() == false) { return; }

	UMissionGraph* MissionGraph = Cast<UMissionGraph>(CurrentGraphEditor->GetCurrentGraph());
	if (MissionGraph == nullptr) { return; }
	
	const bool bUpdated = MissionGraph->UpdateUnknownNodeClasses();
	if (bUpdated == false) { return; }

	const FGraphPanelSelectionSet CurrentSelection = GetSelectedNodes();
	OnSelectedNodesChanged(CurrentSelection);
	MissionGraph->UpdateData();
}


#undef LOCTEXT_NAMESPACE
