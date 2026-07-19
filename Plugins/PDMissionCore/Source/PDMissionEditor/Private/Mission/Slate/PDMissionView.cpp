/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Mission/Slate/PDMissionView.h"
#include "Mission/FPDMissionEditor.h"
#include "Mission/PDMissionBuilder.h"
#include "Mission/Graph/PDMissionGraph.h"
#include "Mission/Graph/PDMissionGraphNodes.h"

#include "PDMissionEditor.h"
#include "Subsystems/PDMissionSubsystem.h"
#include "Mission/Slate/PDMissionView.h"

#include <Editor.h>
#include <ScopedTransaction.h>
#include <EdGraph/EdGraphPin.h>
#include <EdGraph/EdGraphSchema.h>

#if WITH_EDITOR
#include <IStructureDetailsView.h>
#endif

#include <Framework/Application/SlateApplication.h>
#include <Framework/Views/TableViewMetadata.h>

#include <SGraphPanel.h>
#include <SGraphNode.h>
#include <Widgets/Input/STextComboBox.h>
#include <Widgets/Views/STreeView.h>
#include <Widgets/Text/SInlineEditableTextBlock.h>
#include <Widgets/Input/SSlider.h>
#include <Widgets/Images/SImage.h>

#include <Editor/GraphEditor/Private/DragConnection.h>
#include <Framework/Commands/GenericCommands.h>
#include <TutorialMetaData.h>
#include <SCommentBubble.h>

#include <SGameplayTagWidget.h>
#include <SGameplayTagCombo.h>

#define LOCTEXT_NAMESPACE "FMissionTreeNode"

class FText;
class SWidget;
struct FGeometry;
struct FSlateBrush;


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

	// @todo Use MissionData handle and build a graph here
	FPDMissionNodeHandle& MissionData = InMissionEditor->GetMissionData();

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
		UPDMissionGraphNode_EntryPoint* RootNode = Cast<UPDMissionGraphNode_EntryPoint>(Node);
		if (RootNode == nullptr) { continue; }

		RootNodes.Add(MakeShared<FMissionTreeNode>(RootNode, TSharedPtr<FMissionTreeNode>()));
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


//  NODES

TSharedRef<FDragMissionGraphNode> FDragMissionGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
	const TSharedRef<SGraphNode>& InDraggedNode)
{
	TSharedRef<FDragMissionGraphNode> Operation = MakeShareable(new FDragMissionGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes.Add(InDraggedNode);
	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragMissionGraphNode> FDragMissionGraphNode::New(const TSharedRef<SGraphPanel>& InGraphPanel,
	const TArray<TSharedRef<SGraphNode>>& InDraggedNodes)
{
	TSharedRef<FDragMissionGraphNode> Operation = MakeShareable(new FDragMissionGraphNode);
	Operation->StartTime = FPlatformTime::Seconds();
	Operation->GraphPanel = InGraphPanel;
	Operation->DraggedNodes = InDraggedNodes;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Construct();
	return Operation;
}

UPDMissionGraphNode* FDragMissionGraphNode::GetDropTargetNode() const
{
	return Cast<UPDMissionGraphNode>(GetHoveredNode());
}

void SMissionGraphNode::Construct(const FArguments& InArgs, UPDMissionGraphNode* InNode)
{
	SetCursor(EMouseCursor::CardinalCross);

	GraphNode = InNode;

	UpdateGraphNode();

	bDragMarkerVisible = false;	
}

void SMissionGraphNode::AddSubNode(TSharedPtr<SGraphNode> SubNodeWidget)
{
	SubNodes.Add(SubNodeWidget);
}

FText SMissionGraphNode::GetTitle() const
{
	return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
}

FText SMissionGraphNode::GetDescription() const
{
	UPDMissionGraphNode* MyNode = CastChecked<UPDMissionGraphNode>(GraphNode);
	return MyNode ? MyNode->GetDescription() : FText::GetEmpty();
}

EVisibility SMissionGraphNode::GetDescriptionVisibility() const
{
	// LOD this out once things get too small
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;
}

void AddPinInner(TSharedPtr<SVerticalBox> NodeBox, const TSharedRef<SGraphPin>& PinToAdd)
{
	NodeBox->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	.AutoHeight()
	//.FillHeight(4.0f)
	[
		SNew(SBox)
		.MinDesiredHeight(24.0f)
		[
			PinToAdd
		]
	];
}

void SMissionGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	const bool bAdvancedParameter = PinObj && PinObj->bAdvancedView;
	if (bAdvancedParameter)
	{
		PinToAdd->SetVisibility(TAttribute<EVisibility>(PinToAdd, &SGraphPin::IsPinVisibleAsAdvanced));
	}

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		AddPinInner(LeftNodeBox,PinToAdd);
		InputPins.Add(PinToAdd);
	}
	else
	{
		AddPinInner(RightNodeBox,PinToAdd);
		OutputPins.Add(PinToAdd);
	}
}


FPDMissionRow* GetCurrentMission(FName MissionRowName)
{

	UE_LOG(LogTemp, Verbose, TEXT("MISSIONTEST: NEXT MISSION BRANCH SLATE WIDGET"));
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	FPDMissionUtility* Utility = MissionSubsystem ? &MissionSubsystem->Utility : nullptr;
	if (Utility)
	{
		const FDataTableRowHandle& MissionHandle = Utility->MissionLookupViaRowName.FindRef(MissionRowName);
		if (MissionHandle.DataTable)
		{
			return MissionHandle.GetRow<FPDMissionRow>("");
		}
	}
	return nullptr;
}



TPair<FDetailsViewArgs, FStructureDetailsViewArgs> GetDetailsArgs()
{
	FDetailsViewArgs RetDetailsViewArgs;
	RetDetailsViewArgs.bAllowSearch = false;
	RetDetailsViewArgs.bHideSelectionTip = true;
	RetDetailsViewArgs.bLockable = false;
	RetDetailsViewArgs.bSearchInitialKeyFocus = false;
	RetDetailsViewArgs.bUpdatesFromSelection = false;
	RetDetailsViewArgs.bShowOptions = false;
	RetDetailsViewArgs.bShowObjectLabel = false;

	FStructureDetailsViewArgs RetStructureDetailsViewArgs;
	RetStructureDetailsViewArgs.bShowObjects = false;
	return TPair<FDetailsViewArgs, FStructureDetailsViewArgs>{RetDetailsViewArgs, RetStructureDetailsViewArgs};
}

template<typename TStructType>
TSharedRef<SWidget> GenerateSettingsContent(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	return DetailsView.ToSharedRef();
}

template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionStateData>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		auto[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();

		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionStateData::StaticStruct(), (uint8*)&MissionRow->StateData);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}

template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionMetadata>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionMetadata::StaticStruct(), (uint8*)&MissionRow->Metadata);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}

template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionBaseExtension>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionBaseExtension::StaticStruct(), (uint8*)&MissionRow->Base.Ext);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}
	return DetailsView.ToSharedRef();
}


void SMissionGraphNode::SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget)
{
	DefaultTitleAreaWidget->ClearChildren();
	TSharedPtr<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);	
	
	// Write out 10 times by hand tomorrow to fully commit of this into my longterm memory
	DefaultTitleAreaWidget->AddSlot()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Center)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("NoBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						CreateTitleWidget(NodeTitle)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						NodeTitle.ToSharedRef()
					]
				]
			]
		]
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(0, 0, 5, 0)
		.AutoWidth()
		[
			CreateTitleRightWidget()
		]
	];
	DefaultTitleAreaWidget->AddSlot()
	.VAlign(VAlign_Top)
	[
		SNew(SBorder)
		.Visibility(EVisibility::HitTestInvisible)
		.BorderImage( FAppStyle::GetBrush( "Graph.Node.TitleHighlight" ) )
		.BorderBackgroundColor( this, &SGraphNode::GetNodeTitleIconColor )
		[
			SNew(SSpacer)
			.Size(FVector2D(20,20))
		]
	];

}

const FPDMissionRow* SMissionGraphNode::GetMissionRowPtr() const
{
	UPDMissionGraphNode* MissionNode = Cast<UPDMissionGraphNode>(GetNodeObj());
	if (nullptr == MissionNode)
	{
		return nullptr;
	}

	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	check(MissionSubsystem)

	FDataTableRowHandle* RowHandlePtr = MissionSubsystem->Utility.MissionLookupViaRowName.Find(FName(*MissionNode->GetMissionName()));
	if (nullptr == RowHandlePtr)
	{
		return nullptr;
	}

	return  RowHandlePtr->GetRow<FPDMissionRow>(TEXT("SMissionGraphNode::GetMissionRowPtr"));
}

bool SMissionGraphNode::IsExtraDataEnabled() const
{
	return nullptr != GetMissionRowPtr();
}

FReply SMissionGraphNode::OnExtraDataClicked()
{
	const FPDMissionRow* MissionRow = GetMissionRowPtr();
	if (nullptr == MissionRow)
	{
		return FReply::Unhandled();
	}

	TSharedRef<SWidget> BaseExtWidget = GenerateSettingsContent<FPDMissionBaseExtension>(MissionRow, StructureDetailsViewBaseExt);
	TSharedRef<SWidget> MetaDataWidget = GenerateSettingsContent<FPDMissionMetadata>(MissionRow, StructureDetailsViewMetaData);
	TSharedRef<SWidget> StateDataWidget = GenerateSettingsContent<FPDMissionStateData>(MissionRow, StructureDetailsViewStateData);
	TSharedRef<SWidget> BorderWidget = 
		SNew(SBorder)
		.Content()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				BaseExtWidget
			]
			+ SHorizontalBox::Slot()
			[
				MetaDataWidget
			]
			+ SHorizontalBox::Slot()
			[
				StateDataWidget
			]
		];

	FSlateApplication& SlateApp = FSlateApplication::Get();
	FWidgetPath ButtonPath;
	SlateApp.GeneratePathToWidgetUnchecked(ExtraDataButton.ToSharedRef(), ButtonPath);

	ExtraDataPopup = SlateApp.PushMenu(
		SlateApp.GetActiveTopLevelWindow().ToSharedRef(),
		ButtonPath,
		BorderWidget,
		SlateApp.GetCursorPos(),
		FPopupTransitionEffect(FPopupTransitionEffect::None),
		true,
		FVector2D(0.f, 0.f),
		EPopupMethod::CreateNewWindow,
		false);

	bOpenedNode = true;
	
	ExtraDataPopup->GetOnMenuDismissed().AddLambda(
		[this](TSharedRef<IMenu> DismissedMenu)
		{
			bOpenedNode = false;
			ExtraDataPopup.Reset();
		}
	);

	return FReply::Handled();
}


//@TODO: Make configurable by styling, given that this is the 3rd or 4th time I copy and paste this function and applying it to a different types with some different configs, 
// TODO Cont.: I need to move to a table and use as source
FLinearColor SMissionGraphNode::StaticGetTransitionColor(bool bIsOpened, bool bIsHovered)
{
	const FLinearColor ActiveColor(10.0f, 4.0f, 3.0f, 1.0f); // If opened
	const FLinearColor HoverColor(7.24f, 2.56f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.0f, 10.9f, 10.9f, 1.0f);
	return bIsOpened 
		? ActiveColor 
		: bIsHovered 
			? HoverColor 
			: BaseColor;
}

FSlateColor SMissionGraphNode::GetExtraDataColor() const
{
	return StaticGetTransitionColor(bOpenedNode, IsHovered());	
}
const FSlateBrush* SMissionGraphNode::GetExtraDataIconImage() const
{
	return FAppStyle::GetBrush("EditorViewportToolBar.MenuDropdown");
}

TSharedRef<SWidget> SMissionGraphNode::CreateNodeContentArea()
{
	TSharedRef<SBorder> ToplevelWidget =  SNew(SBorder)
		.BorderImage( FAppStyle::GetBrush("NoBorder") )
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Top)
		.Padding(FMargin(24,24))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.FillWidth(1.0f)
				[
					SAssignNew(LeftNodeBox, SVerticalBox)
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Right)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
			]
		];

	ExtraDataButton =
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
			.ColorAndOpacity(this, &SMissionGraphNode::GetExtraDataColor)
		]
		+SOverlay::Slot()
		[
			SNew(SButton)
			.IsEnabled_Raw(this, &SMissionGraphNode::IsExtraDataEnabled)
			.OnClicked_Raw(this, &SMissionGraphNode::OnExtraDataClicked)
			.ButtonColorAndOpacity_Raw(this, &SMissionGraphNode::GetExtraDataColor)
			.Content()
			[
				SNew(SImage)
				.Image(this, &SMissionGraphNode::GetExtraDataIconImage)
			]
		];

	LeftNodeBox->AddSlot()
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Top)
	.AutoHeight()
	//.FillHeight(4.0f)
	[
		SNew(SBox)
		.MinDesiredHeight(24.0f)
		[
			ExtraDataButton.ToSharedRef()
		]
	];


	return ToplevelWidget;
}

void SMissionGraphNode::CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox)
{
	TSharedRef<SWidget> AddPinButton = AddPinButtonContent(
		NSLOCTEXT("CompareNode", "CompareNodeAddPinButton", "Add Case"),
		NSLOCTEXT("CompareNode", "CompareNodeAddPinButton_Tooltip", "Add new case pins"));

	FMargin AddPinPadding = Settings->GetOutputPinPadding();
	AddPinPadding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(AddPinPadding)
		[
			AddPinButton
		];
}

FReply SMissionGraphNode::OnAddPin()
{
	UPDMissionGraphNode* MissionNode = CastChecked<UPDMissionGraphNode>(GraphNode);
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();

	FDataTableRowHandle* RowHandlePtr = nullptr;
	if (false == UPDMissionEditorStatics::RowOp::IsMissionValid(MissionNode->SelectedMissionRowName, RowHandlePtr))
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction( NSLOCTEXT("Kismet", "AddExecutionPin", "Add Execution Pin") );
	MissionNode->Modify();

	const TArray<UEdGraphPin*> OutPins = MissionNode->Pins.FilterByPredicate([](const UEdGraphPin* PinElem) -> bool {return PinElem->Direction != EEdGraphPinDirection::EGPD_Input;});
	const int32 BranchPrio = OutPins.Num();
	
	const FString PinName = UPDMissionEditorStatics::NodeOp::BuildPinName(BranchPrio, NAME_None);
	MissionNode->CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, *PinName);

	UPDMissionEditorStatics::RowOp::AddBranch(RowHandlePtr);

	UpdateGraphNode();
	GraphNode->GetGraph()->NotifyNodeChanged(GraphNode);
	return FReply::Handled();
}



BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SMissionGraphNode::UpdateGraphNode()
{
	SGraphNode::UpdateGraphNode(); // @todo replace with custom styling, this virtual is the function the SGraphNode uses to set up it's slate style
}

bool SMissionGraphNode::UseLowDetailNodeTitles() const
{
	return SGraphNode::UseLowDetailNodeTitles();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SMissionGraphNode::OnMouseMove(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !(GEditor->bIsSimulatingInEditor || GEditor->PlayWorld))
	{
		//if we are holding mouse over a subnode
		UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (TestNode && TestNode->IsSubNode())
		{
			const TSharedRef<SGraphPanel>& Panel = GetOwnerPanel().ToSharedRef();
			const TSharedRef<SGraphNode>& Node = SharedThis(this);
			return FReply::Handled().BeginDragDrop(FDragMissionGraphNode::New(Panel, Node));
		}
	}

	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && bDragMarkerVisible)
	{
		SetDragMarker(false);
	}

	return FReply::Unhandled();
}

FReply SMissionGraphNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
	if (TestNode && TestNode->IsSubNode())
	{
		GetOwnerPanel()->SelectionManager.ClickedOnNode(GraphNode, MouseEvent);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

TSharedPtr<SGraphNode> SMissionGraphNode::GetSubNodeUnderCursor(const FGeometry& WidgetGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SGraphNode> ResultNode;

	// We just need to find the one WidgetToFind among our descendants.
	TSet< TSharedRef<SWidget> > SubWidgetsSet;
	for (int32 i = 0; i < SubNodes.Num(); i++)
	{
		SubWidgetsSet.Add(SubNodes[i].ToSharedRef());
	}

	TMap<TSharedRef<SWidget>, FArrangedWidget> Result;
	FindChildGeometries(WidgetGeometry, SubWidgetsSet, Result);

	if (Result.Num() > 0)
	{
		FArrangedChildren ArrangedChildren(EVisibility::Visible);
		Result.GenerateValueArray(ArrangedChildren.GetInternalArray());
		
		const int32 HoveredIndex = SWidget::FindChildUnderMouse(ArrangedChildren, MouseEvent);
		if (HoveredIndex != INDEX_NONE)
		{
			ResultNode = StaticCastSharedRef<SGraphNode>(ArrangedChildren[HoveredIndex].Widget);
		}
	}

	return ResultNode;
}

void SMissionGraphNode::SetDragMarker(bool bEnabled)
{
	bDragMarkerVisible = bEnabled;
}

EVisibility SMissionGraphNode::GetDragOverMarkerVisibility() const
{
	return bDragMarkerVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMissionGraphNode::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));

		UPDMissionGraphNode* TestNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (DragConnectionOp->IsValidOperation() && TestNode && TestNode->IsSubNode())
		{
			SetDragMarker(true);
		}
	}

	SGraphNode::OnDragEnter(MyGeometry, DragDropEvent);
}

FReply SMissionGraphNode::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// Is someone dragging a node?
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are hovering over this node.
		TSharedPtr<SGraphNode> SubNode = GetSubNodeUnderCursor(MyGeometry, DragDropEvent);
		DragConnectionOp->SetHoveredNode(SubNode.IsValid() ? SubNode : SharedThis(this));
	}
	return SGraphNode::OnDragOver(MyGeometry, DragDropEvent);
}

void SMissionGraphNode::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragNode> DragConnectionOp = DragDropEvent.GetOperationAs<FDragNode>();
	if (DragConnectionOp.IsValid())
	{
		// Inform the Drag and Drop operation that we are not hovering any pins
		DragConnectionOp->SetHoveredNode(TSharedPtr<SGraphNode>(NULL));
	}

	SetDragMarker(false);
	SGraphNode::OnDragLeave(DragDropEvent);
}

FReply SMissionGraphNode::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	SetDragMarker(false);

	TSharedPtr<FDragMissionGraphNode> DragNodeOp = DragDropEvent.GetOperationAs<FDragMissionGraphNode>();
	if (DragNodeOp.IsValid())
	{
		if (!DragNodeOp->IsValidOperation())
		{
			return FReply::Handled();
		}

		const float DragTime = float(FPlatformTime::Seconds() - DragNodeOp->StartTime);
		if (DragTime < 0.5f)
		{
			return FReply::Handled();
		}

		UPDMissionGraphNode* MyNode = Cast<UPDMissionGraphNode>(GraphNode);
		if (MyNode == nullptr || MyNode->IsSubNode())
		{
			return FReply::Unhandled();
		}

		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_DragDropNode", "Drag&Drop Node"));
		bool bReorderOperation = true;

		const TArray< TSharedRef<SGraphNode> >& DraggedNodes = DragNodeOp->GetNodes();
		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UPDMissionGraphNode* DraggedNode = Cast<UPDMissionGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			if (DraggedNode && DraggedNode->ParentNode)
			{
				if (DraggedNode->ParentNode != GraphNode)
				{
					bReorderOperation = false;
				}

				DraggedNode->ParentNode->RemoveSubNode(DraggedNode);
			}
		}

		UPDMissionGraphNode* DropTargetNode = DragNodeOp->GetDropTargetNode();
		const int32 InsertIndex = MyNode->FindSubNodeDropIndex(DropTargetNode);

		for (int32 Idx = 0; Idx < DraggedNodes.Num(); Idx++)
		{
			UPDMissionGraphNode* DraggedTestNode = Cast<UPDMissionGraphNode>(DraggedNodes[Idx]->GetNodeObj());
			DraggedTestNode->Modify();
			DraggedTestNode->ParentNode = MyNode;

			MyNode->Modify();
			MyNode->InsertSubNodeAt(DraggedTestNode, InsertIndex);
		}

		if (bReorderOperation)
		{
			UpdateGraphNode();
		}
		else
		{
			UPDMissionGraph* MyGraph = Cast<UPDMissionGraph>(MyNode->GetGraph());
			if (MyGraph)
			{
				MyGraph->OnSubNodeDropped();
			}
		}
	}

	return SGraphNode::OnDrop(MyGeometry, DragDropEvent);
}

TSharedPtr<SToolTip> SMissionGraphNode::GetComplexTooltip()
{
	return NULL;
}

FText SMissionGraphNode::GetPreviewCornerText() const
{
	return FText::GetEmpty();
}

const FSlateBrush* SMissionGraphNode::GetNameIcon() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

void SMissionGraphNode::SetOwner(const TSharedRef<SGraphPanel>& OwnerPanel)
{
	SGraphNode::SetOwner(OwnerPanel);

	for (auto& ChildWidget : SubNodes)
	{
		if (ChildWidget.IsValid())
		{
			ChildWidget->SetOwner(OwnerPanel);
			OwnerPanel->AttachGraphEvents(ChildWidget);
		}
	}
}


TSharedRef<SWidget> GenerateBranchElementContent(FPDMissionRow* MissionRow, int32 BranchIdx, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow && MissionRow->ProgressRules.NextMissionBranch.Branches.IsValidIndex(BranchIdx))
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionBranchElement::StaticStruct(), (uint8*)&MissionRow->ProgressRules.NextMissionBranch.Branches[BranchIdx]);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}
	return DetailsView.ToSharedRef();
}


//
// Mission transition condition node
void SGraphNodeMissionCondition::Construct(const FArguments& InArgs, UPDMissionGraphNode_Knot* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

UPDMissionGraphNode* SGraphNodeMissionCondition::GetSourceNode(UEdGraphPin*& OutSourcePin) const
{
	if (InputPins.IsEmpty())
	{
		// TODO: create an actual log category for these slate widgets
		UE_LOG(LogTemp, Error, TEXT("SGraphNodeMissionCondition::GetSourceNode - Nothing connected"))
		return nullptr;
	}

	if (InputPins.IsEmpty())
	{
		// TODO: create an actual log category for these slate widgets
		UE_LOG(LogTemp, Error, TEXT("SGraphNodeMissionCondition::GetSourceNode - Nothing connected"))
		return nullptr;
	}

	UEdGraphPin* SourcePin = InputPins[0]->GetPinObj();
	auto[SourceNode, ResolvedSourcePin] = UPDMissionEditorStatics::NodeOp::ResolveMissionNodeFromKnot(SourcePin, EEdGraphPinDirection::EGPD_Input);
	SourceNode = SourceNode ? SourceNode : Cast<UPDMissionGraphNode>(SourcePin->GetOwningNode());
	if (false == UPDMissionEditorStatics::NodeOp::IsRowBasedMissionNode(SourceNode) || SourcePin->LinkedTo.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("SGraphNodeMissionCondition::GetSourceNode - No valid connected source mission"))
		return nullptr;
	}

	OutSourcePin = SourcePin;
	return SourceNode;	
}
bool SGraphNodeMissionCondition::IsEnabled() const
{
	UEdGraphPin* OutSourcePinDummy = nullptr;
	return GetSourceNode(OutSourcePinDummy) != nullptr;
}
FReply SGraphNodeMissionCondition::OnClicked()
{
	// TODO: Pseudo code, thoughts to explore later
	// UEdGraphPin* FinalSourcePin = nullptr;
	// UPDMissionGraphNode* SourceNode = GetSourceNode(FinalSourcePin);
	// if (false)
	// {
	// 	FPDWidgetCreationDelegate Delegate = FPDWidgetCreationDelegate::CreateLambda(
	// 	[this](FPDCreationDelegateParams Params) -> TSharedRef<SWidget>
	// 	{
	// 		return GenerateBranchElementContent(Params.MissionRow, Params.OptionalBranchIndex, Params.StructureDetailsView);
	// 	}); 
	// 	FPDCreationDelegateParams MutableParams;
	// 	MutableParams.CondPopup = CondPopup;
	// 	MutableParams.StructureDetailsView = StructureDetailsView;
	// 	return UPDMissionEditorStatics::NodeOp::GenericOnClicked(bOpenedNode, SourceNode, FinalSourcePin, this->GetSlot(ENodeZone::BottomCenter)->GetWidget(), true, Delegate, MutableParams);
	// }

	UEdGraphPin* FinalSourcePin = nullptr;
	UPDMissionGraphNode* SourceNode = GetSourceNode(FinalSourcePin);
	if (nullptr == SourceNode)
	{
		return FReply::Unhandled();
	}

	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	FDataTableRowHandle* RowHandlePtr = MissionSubsystem->Utility.MissionLookupViaRowName.Find(FName(*SourceNode->GetMissionName()));
	
	FPDMissionRow* MissionRow = nullptr == RowHandlePtr ? nullptr : RowHandlePtr->GetRow<FPDMissionRow>(TEXT("SGraphNodeMissionCondition::OnClicked"));
	if (nullptr == MissionRow)
	{
		UE_LOG(LogTemp, Error, TEXT("SGraphNodeMissionCondition::IsEnabled - No valid source mission row(%s)"), *SourceNode->GetMissionName())
		return FReply::Unhandled();
	}
	
	UEdGraphPin* SourceBranchPin = FinalSourcePin->LinkedTo[0];
	const int32 BranchIdx = UPDMissionEditorStatics::NodeOp::GetBranchIdxFromPinName(SourceBranchPin);
	if (false == MissionRow->ProgressRules.NextMissionBranch.Branches.IsValidIndex(BranchIdx))
	{
		UE_LOG(LogTemp, Error, TEXT("SGraphNodeMissionCondition::IsEnabled - No valid branch index(%i) at mission(%s)"), BranchIdx, *SourceNode->GetMissionName())
		return FReply::Unhandled();
	}

	TSharedRef<SWidget> BranchElementAsPopup = GenerateBranchElementContent(MissionRow, BranchIdx, StructureDetailsView);
	const FVector2D MousePosition = FSlateApplication::Get().GetCursorPos();

	FWidgetPath CondWidgetPath;
	FSlateApplication::Get().GeneratePathToWidgetUnchecked(this->GetSlot(ENodeZone::BottomCenter)->GetWidget(), CondWidgetPath);

	CondPopup = FSlateApplication::Get().PushMenu(
		FSlateApplication::Get().GetActiveTopLevelWindow().ToSharedRef(),
		CondWidgetPath,
		BranchElementAsPopup,
		MousePosition,
		FPopupTransitionEffect(FPopupTransitionEffect::None),
		true,
		FVector2D(0.f,0.f),
		EPopupMethod::CreateNewWindow,
		false);
    bOpenedNode = true;
	CondPopup->GetOnMenuDismissed().AddLambda([this](TSharedRef<IMenu> DismissedMenu) 
	{
	    bOpenedNode = false;
		CondPopup.Reset();
	});


	return FReply::Handled();
}
void SGraphNodeMissionCondition::UpdateGraphNode()
{
	SGraphNodeKnot::UpdateGraphNode();
	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::BottomCenter)
		.SlotOffset(FVector2D{8.0, 16.0})
		.SlotSize(FVector2D{75.0f, 50.0f})
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Top)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill") )
				.ColorAndOpacity( this, &SGraphNodeMissionCondition::GetTransitionColor )
			]
			+SOverlay::Slot()
			[
				SNew(SButton)
				.IsEnabled_Raw(this, &SGraphNodeMissionCondition::IsEnabled)
				.OnClicked_Raw(this, &SGraphNodeMissionCondition::OnClicked)
				.ButtonColorAndOpacity_Raw(this, &SGraphNodeMissionCondition::GetTransitionColor)
				.Content()
				[
					SNew(SImage)
					.Image( this, &SGraphNodeMissionCondition::GetTransitionIconImage )
				]
			]
		];
}

FLinearColor SGraphNodeMissionCondition::StaticGetTransitionColor(bool bIsOpened, bool bIsHovered)
{
	//@TODO: Make configurable by styling
	const FLinearColor ActiveColor(10.0f, 4.0f, 3.0f, 1.0f); // If opened
	const FLinearColor HoverColor(7.24f, 2.56f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.0f, 10.9f, 10.9f, 1.0f);
	return bIsOpened 
		? ActiveColor 
		: bIsHovered 
			? HoverColor 
			: BaseColor;
}

FSlateColor SGraphNodeMissionCondition::GetTransitionColor() const
{	
	// Highlight the transition node when the node is hovered or when the previous state is hovered
	UPDMissionGraphNode_ConditionNode* TransNode = CastChecked<UPDMissionGraphNode_ConditionNode>(GraphNode); // Reserved for later potential use, kept as a reminder
	return StaticGetTransitionColor(bOpenedNode, IsHovered());
}

const FSlateBrush* SGraphNodeMissionCondition::GetTransitionIconImage() const
{
	UPDMissionGraphNode_ConditionNode* TransNode = CastChecked<UPDMissionGraphNode_ConditionNode>(GraphNode); // Reserved for later potential use, kept as a reminder
	return FAppStyle::GetBrush("Graph.TransitionNode.Icon_Inertialization");
}


//
// Mission Transition Node(s)

void SGraphNodeMissionTransition::Construct(const FArguments& InArgs, UPDMissionTransitionNode* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

void SGraphNodeMissionTransition::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
}

void SGraphNodeMissionTransition::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	// Ignored; position is set by the location of the attached state nodes
}

bool SGraphNodeMissionTransition::RequiresSecondPassLayout() const
{
	return true;
}

void SGraphNodeMissionTransition::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	// Find the geometry of the state nodes we're connecting
	FGeometry StartGeom;
	FGeometry EndGeom;

	int32 TransIndex = 0;
	int32 NumOfTrans = 1;

	UPDMissionGraphNode* PrevState = TransNode->GetOwningMission();
	UPDMissionGraphNode* NextState = TransNode->GetTargetMission();
	if ((PrevState != NULL) && (NextState != NULL))
	{
		const TSharedRef<SNode>* pPrevNodeWidget = NodeToWidgetLookup.Find(PrevState);
		const TSharedRef<SNode>* pNextNodeWidget = NodeToWidgetLookup.Find(NextState);
		if ((pPrevNodeWidget != NULL) && (pNextNodeWidget != NULL))
		{
			const TSharedRef<SNode>& PrevNodeWidget = *pPrevNodeWidget;
			const TSharedRef<SNode>& NextNodeWidget = *pNextNodeWidget;

			StartGeom = FGeometry(FVector2D(PrevState->NodePosX, PrevState->NodePosY), FVector2D::ZeroVector, PrevNodeWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(NextState->NodePosX, NextState->NodePosY), FVector2D::ZeroVector, NextNodeWidget->GetDesiredSize(), 1.0f);

			TArray<UPDMissionTransitionNode*> Transitions;
			PrevState->GetMissionTransitions(Transitions);

			Transitions = Transitions.FilterByPredicate([NextState](const UPDMissionTransitionNode* InTransition) -> bool
			{
				return InTransition->GetTargetMission() == NextState;
			});

			TransIndex = Transitions.IndexOfByKey(TransNode);
			NumOfTrans = Transitions.Num();

			PrevStateNodeWidgetPtr = PrevNodeWidget;
		}
	}

	//Position Node
	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, TransIndex, NumOfTrans);
}

TSharedRef<SWidget> SGraphNodeMissionTransition::GenerateRichTooltip()
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	//
	// Generate tooltip test in a vertical widget box, for the transition wdget
	UEdGraphPin* CanExecPin = NULL;
	TSharedRef<SVerticalBox> Widget = SNew(SVerticalBox);
	const FText TooltipDesc = GetPreviewCornerText(false);
	
	// Transition rule linearized
	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew(STextBlock)
			.TextStyle( FAppStyle::Get(), TEXT("Graph.TransitionNode.TooltipName") )
			.Text(TooltipDesc)
		];
	
	Widget->AddSlot()
	.AutoHeight()
	.Padding( 2.0f )
	[
		SNew(STextBlock)
		.TextStyle( FAppStyle::Get(), TEXT("Graph.TransitionNode.TooltipRule") )
		.Text(LOCTEXT("AnimGraphNodeTransitionRule_ToolTip", "Transition Rule (in words)"))
	];
	
	// Widget->AddSlot()
	// 	.AutoHeight()
	// 	.Padding( 2.0f )
	// 	[
	// 		SNew(SKismetLinearExpression, CanExecPin)
	// 	];

	return Widget;
}

TSharedPtr<SToolTip> SGraphNodeMissionTransition::GetComplexTooltip()
{
	return SNew(SToolTip)
		[
			GenerateRichTooltip()
		];
}

void SGraphNodeMissionTransition::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);
	this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FAppStyle::GetBrush("Graph.TransitionNode.ColorSpill"))
				.ColorAndOpacity( this, &SGraphNodeMissionTransition::GetTransitionColor)
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(this, &SGraphNodeMissionTransition::GetTransitionIconImage)
			]
		];
}

FText SGraphNodeMissionTransition::GetPreviewCornerText(bool bReverse) const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);

	UPDMissionGraphNode* PrevState = (bReverse ? TransNode->GetTargetMission() : TransNode->GetOwningMission());
	UPDMissionGraphNode* NextState = (bReverse ? TransNode->GetOwningMission() : TransNode->GetTargetMission());

	FText Result = LOCTEXT("BadTransition", "Bad transition (missing source or target)");

	// Show the priority if there is any ambiguity
	if (PrevState != NULL)
	{
		if (NextState != NULL)
		{
			TArray<UPDMissionTransitionNode*> TransitionFromSource;
			PrevState->GetMissionTransitions(/*out*/ TransitionFromSource);

			bool bMultiplePriorities = false;
			if (TransitionFromSource.Num() > 1)
			{
				// See if the priorities differ
				for (int32 Index = 0; (Index < TransitionFromSource.Num()) && !bMultiplePriorities; ++Index)
				{
					const bool bDifferentPriority = (TransitionFromSource[Index]->PriorityOrder != TransNode->PriorityOrder);
					bMultiplePriorities |= bDifferentPriority;
				}
			}

			if (bMultiplePriorities)
			{
				Result = FText::Format(LOCTEXT("TransitionXToYWithPriority", "{0} to {1} (Priority {2})"), FText::FromString(PrevState->GetMissionName()), FText::FromString(NextState->GetMissionName()), FText::AsNumber(TransNode->PriorityOrder));
			}
			else
			{
				Result = FText::Format(LOCTEXT("TransitionXToY", "{0} to {1}"), FText::FromString(PrevState->GetMissionName()), FText::FromString(NextState->GetMissionName()));
			}
		}
	}

	return Result;
}

FLinearColor SGraphNodeMissionTransition::StaticGetTransitionColor(UPDMissionTransitionNode* TransNode, bool bIsHovered)
{
	//@TODO: Make configurable by styling
	const FLinearColor ActiveColor(1.0f, 0.4f, 0.3f, 1.0f);
	const FLinearColor HoverColor(0.724f, 0.256f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.9f, 0.9f, 0.9f, 1.0f);
	

	//@TODO: ANIMATION: Sort out how to display this
	// 			if (TransNode->SharedCrossfadeIdx != INDEX_NONE)
	// 			{
	// 				WireColor.R = (TransNode->SharedCrossfadeIdx & 1 ? 1.0f : 0.15f);
	// 				WireColor.G = (TransNode->SharedCrossfadeIdx & 2 ? 1.0f : 0.15f);
	// 				WireColor.B = (TransNode->SharedCrossfadeIdx & 4 ? 1.0f : 0.15f);
	// 			}
	

	return bIsHovered ? HoverColor : BaseColor;
}

FSlateColor SGraphNodeMissionTransition::GetTransitionColor() const
{	
	// Highlight the transition node when the node is hovered or when the previous state is hovered
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	return StaticGetTransitionColor(TransNode, (IsHovered() || (PrevStateNodeWidgetPtr.IsValid() && PrevStateNodeWidgetPtr.Pin()->IsHovered())));
}

const FSlateBrush* SGraphNodeMissionTransition::GetTransitionIconImage() const
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	return FAppStyle::GetBrush("Graph.TransitionNode.Icon");
}

// TSharedRef<SWidget> SGraphNodeMissionTransition::GenerateInlineDisplayOrEditingWidget(bool bShowGraphPreview)
// {
// }

void SGraphNodeMissionTransition::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->AddPinToHoverSet(Pin);
	}

	SGraphNode::OnMouseEnter(MyGeometry, MouseEvent);
}

void SGraphNodeMissionTransition::OnMouseLeave(const FPointerEvent& MouseEvent)
{	
	UPDMissionTransitionNode* TransNode = CastChecked<UPDMissionTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->RemovePinFromHoverSet(Pin);
	}

	SGraphNode::OnMouseLeave(MouseEvent);
}

void SGraphNodeMissionTransition::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = static_cast<int32>(NewCorner.X);
	GraphNode->NodePosY = static_cast<int32>(NewCorner.Y);
}

SPDLODBranchNode::SPDLODBranchNode()
	: LastCachedValue(INDEX_NONE)
	, ChildSlotLowDetail(SNullWidget::NullWidget)
	, ChildSlotHighDetail(SNullWidget::NullWidget)
{
}

void SPDLODBranchNode::Construct(const FArguments& InArgs)
{
	OnGetActiveDetailSlotContent = InArgs._OnGetActiveDetailSlotContent;
	ShowLowDetailAttr = InArgs._UseLowDetailSlot;
	ChildSlotLowDetail = InArgs._LowDetail.Widget;
	ChildSlotHighDetail = InArgs._HighDetail.Widget;

	const int32 CurrentValue = ShowLowDetailAttr.Get() ? 1 : 0;
	if (OnGetActiveDetailSlotContent.IsBound())
	{
		ChildSlot[OnGetActiveDetailSlotContent.Execute((CurrentValue != 0) ? false : true)];
	}
	else
	{
		ChildSlot[(CurrentValue != 0) ? ChildSlotLowDetail : ChildSlotHighDetail];
	}

}

void SPDLODBranchNode::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	RefreshLODSlotContent();
}

void SPDLODBranchNode::RefreshLODSlotContent()
{
	const int32 CurrentValue = ShowLowDetailAttr.Get() ? 1 : 0;
	if (CurrentValue != LastCachedValue)
	{
		LastCachedValue = CurrentValue;

		if (OnGetActiveDetailSlotContent.IsBound())
		{
			ChildSlot
				[
					OnGetActiveDetailSlotContent.Execute((CurrentValue != 0) ? false : true)
				];
		}
		else
		{
			ChildSlot
				[
					(CurrentValue != 0) ? ChildSlotLowDetail : ChildSlotHighDetail
				];
		}
	}
}

//
// PINS

void SPDMissionGraphPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObject)
{
	this->SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InGraphPinObject;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SPDMissionGraphPin::GetPinBorder)
		.BorderBackgroundColor(this, &SPDMissionGraphPin::GetPinColor)
		.OnMouseButtonDown(this, &SPDMissionGraphPin::OnPinMouseDown)
		.Cursor(this, &SPDMissionGraphPin::GetPinCursor)
		.Padding(FMargin(10.0f))
		);		
}


TSharedRef<SWidget>	SPDMissionGraphPin::GetDefaultValueWidget()
{
	return SNew(STextBlock);
}

const FSlateBrush* SPDMissionGraphPin::GetPinBorder() const
{
	return FAppStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

FSlateColor SPDMissionGraphPin::GetPinColor() const
{
	return FSlateColor(IsHovered() ? FLinearColor::Yellow : FLinearColor::Black);
}


//
// Row Selector attribute

void SPDAttributePin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	// @todo will try to figure out a good way to indent the pins a bit wihtout having to redo the whole construct
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SPDAttributePin::GetDefaultValueWidget()
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr) { return SNew(STextComboBox); }
	
	return 
		SNew(STextComboBox) //note you can display any widget here
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

	// - Write slate code so Make Mission entry displays some simple creation wizard 
	// -- 1. The user selects the tag, ensure we hide tags in existing missions from the tag list
	// -- 2. A button is displayed that says "Create Mission Node". When pressed create new entry in the table
	// -- 3. Update the nodes selected mission, and then refresh graph
	const bool bWantsToCreateNewMission = SelectedMissionRowName.ToString() == TAG_MakeNewMission.GetTag().ToString();
	if (bWantsToCreateNewMission)
	{		
		// @todo 1. Add save button to the graph and the call the functions I wrote yesterday for saving/loading to and from the editing table
		// @todo 2. We need a hidden pin which becomes visible when this option is set, one that lets us select a rowname and mission tag for hte mission entry we are creating
		return;
	}	

	
	// We need to refresh the DataRefPins here, this is the struct view pin that will reflect our selected entry key
	UPDMissionGraphNode* AsMissionGraphNode = OwnerNodePtr.Pin() != nullptr ?
		Cast<UPDMissionGraphNode>(OwnerNodePtr.Pin()->GetNodeObj()) : nullptr;
	if (AsMissionGraphNode != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Calling RefreshDataRefPins(SelectedMissionRowName), SelectedMissionName: %s"), *SelectedMissionRowName.ToString())
		AsMissionGraphNode->RefreshDataRefPins(SelectedMissionRowName);
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
// Tag selector wrapper
void SPDTagSelector::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SPDTagSelector::GetDefaultValueWidget()
{
	TSharedRef<SWidget> TagWidgetWrapper = 
		SNew(SGameplayTagCombo)
		.Visibility(EVisibility::Visible)
		.Tag(this, &SPDTagSelector::GetGameplayTag)
		.OnTagChanged_Lambda(
			[&](const FGameplayTag NewTag)
			{
				Tag = NewTag;
			});
	return TagWidgetWrapper;
}

//
// Checkbox Wrapper
void SPDGenericInputWrapper::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	InputTypeAttr = InArgs._InputType;
	MissionRowName = InArgs._MissionRowName;
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionBase>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		auto[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		StructureDetailsViewArgs.bShowObjects = StructureDetailsViewArgs.bShowAssets = StructureDetailsViewArgs.bShowClasses = StructureDetailsViewArgs.bShowInterfaces = false;
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionBase::StaticStruct(), (uint8*)&MissionRow->Base);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, nullptr);
		StructureDetailsView->GetDetailsView()->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateLambda(
			[](const FPropertyAndParent& PropAndParent) -> bool
			{
				return PropAndParent.Property.GetName() != GET_MEMBER_NAME_CHECKED(FPDMissionBase, Ext);
			}));
		StructureDetailsView->SetStructureData(StructData);

		DetailsView = StructureDetailsView->GetWidget();
	}
	return DetailsView.ToSharedRef();
}
template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionTickBehaviour>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionTickBehaviour::StaticStruct(), (uint8*)&MissionRow->TickSettings);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}
template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionTagCompound>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionTagCompound::StaticStruct(), (uint8*)&MissionRow->ProgressRules.MissionConditionHandler);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}
template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionRules>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionRules::StaticStruct(), (uint8*)&MissionRow->ProgressRules);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}
template<>
TSharedRef<SWidget> GenerateSettingsContent<FPDMissionBranch>(const FPDMissionRow* MissionRow, TSharedPtr<class IStructureDetailsView> StructureDetailsView)
{
	TSharedPtr<SWidget> DetailsView = SNew(STextBlock).Text(FText::AsCultureInvariant(TEXT("N/A")));
	if(MissionRow)
	{
		const auto&[DetailsViewArgs, StructureDetailsViewArgs] = GetDetailsArgs();
		FPropertyEditorModule& PropertyEditor = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		TSharedPtr<FStructOnScope> StructData = MakeShared<FStructOnScope>(FPDMissionBranch::StaticStruct(), (uint8*)&MissionRow->ProgressRules.NextMissionBranch);
		StructureDetailsView = PropertyEditor.CreateStructureDetailView(DetailsViewArgs, StructureDetailsViewArgs, StructData);
		DetailsView = StructureDetailsView->GetWidget();
	}	
	return DetailsView.ToSharedRef();
}


FPDMissionRow* SPDGenericInputWrapper::GetCurrentMissionRow(){return GetCurrentMission(MissionRowName);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateBaseSettingsContent(){return GenerateSettingsContent<FPDMissionBase>(GetCurrentMissionRow(), StructureDetailsView);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateTickSettingsContent(){return GenerateSettingsContent<FPDMissionTickBehaviour>(GetCurrentMissionRow(), StructureDetailsView);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateProgressRulesContent(){return GenerateSettingsContent<FPDMissionRules>(GetCurrentMissionRow(), StructureDetailsView);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateMetadataContent(){return GenerateSettingsContent<FPDMissionMetadata>(GetCurrentMissionRow(), StructureDetailsView);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateBranchesContent(){return GenerateSettingsContent<FPDMissionBranch>(GetCurrentMissionRow(), StructureDetailsView);}

TSharedRef<SWidget> SPDGenericInputWrapper::GenerateTagCompoundContent(){return GenerateSettingsContent<FPDMissionTagCompound>(GetCurrentMissionRow(), StructureDetailsView);}
TSharedRef<SWidget> SPDGenericInputWrapper::GenerateStateDataContent(){return GenerateSettingsContent<FPDMissionStateData>(GetCurrentMissionRow(), StructureDetailsView);}

TSharedRef<SWidget> SPDGenericInputWrapper::GetDefaultValueWidget()
{	
	switch (InputTypeAttr.Get())
	{
	case EGenericInputSelector::EMissionTickPaused:
		{
			TSharedRef<SWidget> PauseStateCheckboxWrapper = 
				SNew(SCheckBox)
				.Visibility(EVisibility::Visible)
				.OnCheckStateChanged(this, &SPDGenericInputWrapper::OnPauseStateChanged)
				.IsChecked(this, &SPDGenericInputWrapper::GetPauseState);
			return PauseStateCheckboxWrapper;
		}

	case EGenericInputSelector::EMissionRepeatable:
		{
			TSharedRef<SWidget> RepeatableStateCheckboxWrapper = 
				SNew(SCheckBox)
				.Visibility(EVisibility::Visible)
				.OnCheckStateChanged(this, &SPDGenericInputWrapper::OnRepeatableStateChanged)
				.IsChecked(this, &SPDGenericInputWrapper::GetRepeatableState);
			return RepeatableStateCheckboxWrapper;
		}
	case EGenericInputSelector::EMissionMetadata:
		{
			return SNew(SComboButton)
				.ContentPadding(FMargin(2.0f, 2.0f))
				.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton")
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Metadata"))

				]
				.OnGetMenuContent(this, &SPDGenericInputWrapper::GenerateMetadataContent);
		}
	case EGenericInputSelector::ENextMissionBranch:
		{
			return SNew(SComboButton)
				.ContentPadding(FMargin(2.0f, 2.0f))
				.ComboButtonStyle(FAppStyle::Get(), "SimpleComboButton") 
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(FText::AsCultureInvariant("Tiered branch logic"))
				]
				.OnGetMenuContent(this, &SPDGenericInputWrapper::GenerateBranchesContent);
		}
	case EGenericInputSelector::EMissionID:
		{
			FGameplayTag TODO__FINISH_ME_TAG = FGameplayTag::EmptyTag;
			OnMissionIDChangedViaTag(TODO__FINISH_ME_TAG);
			FText TODO__FINISH_ME_TEXT= FText::AsCultureInvariant(FString(TEXT("TODO:FINISH ME")));
			TSharedRef<SWidget> MissionIDValue = SNew(STextBlock).Text(TODO__FINISH_ME_TEXT); //(this, &SPDGenericInputWrapper::GetMissionIDAsText);
			
			return MissionIDValue;
		}
	case EGenericInputSelector::EMissionBranchBehaviour:
	{
		Options.Empty();
		for(int32 Step = static_cast<EPDMissionBranchBehaviour>(0); Step < EPDMissionBranchBehaviour::EMAX_BRANCH; Step++)
		{
			Options.Emplace(MakeShared<FString>(UEnum::GetValueAsName(static_cast<EPDMissionBranchBehaviour>(Step)).ToString()));
		}

		TSharedRef<SWidget> MissionBranchOptions = 
			SNew(STextComboBox)
			.OnSelectionChanged(this, &SPDGenericInputWrapper::OnMissionBranchBehaviourChanged)
			.InitiallySelectedItem(GetMissionStateAsString())
			.OptionsSource(&Options);

		return MissionBranchOptions;

	}
	case EGenericInputSelector::EMissionState:
	{
		Options.Empty();
		for(int32 Step = static_cast<EPDMissionState>(0); Step < EPDMissionState::EMAX_MISSIONSTATE; Step++)
		{
			Options.Emplace(MakeShared<FString>(UEnum::GetValueAsName(static_cast<EPDMissionState>(Step)).ToString()));
		}

		TSharedRef<SWidget> MissionStateOptions = 
			SNew(STextComboBox)
			.OnSelectionChanged(this, &SPDGenericInputWrapper::OnMissionStateChanged)
			.InitiallySelectedItem(GetMissionStateAsString())
			.OptionsSource(&Options);

		return MissionStateOptions;
	}

	// This luckliy forces the whole tree to the same element height
	case EGenericInputSelector::EMissionTickInterval:
	{
		TSharedRef<SWidget> NumberBox = 
			SNew(SBox)
			.MinDesiredWidth(250)
			.MaxDesiredWidth(400)
			.MinDesiredHeight(200)
			.HAlign(EHorizontalAlignment::HAlign_Left)
			.VAlign(EVerticalAlignment::VAlign_Top)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SSlider)
					.Visibility(EVisibility::Visible)
					.MinValue(0.f)
					.MaxValue(120.f) // Seconds
					.Value(this, &SPDGenericInputWrapper::GetMissionTickInterval)
					.OnValueChanged(this, &SPDGenericInputWrapper::OnMissionTickIntervalChanged)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(this, &SPDGenericInputWrapper::GetMissionTickIntervalAsText)
				]
			];
		return NumberBox;
	}
	case EGenericInputSelector::EMissionBranchDelayTime:
	{
		TSharedRef<SWidget> NumberBox = 
			SNew(SBox)
			.MinDesiredWidth(250)
			.MaxDesiredWidth(400)
			.MinDesiredHeight(200)
			.HAlign(EHorizontalAlignment::HAlign_Left)
			.VAlign(EVerticalAlignment::VAlign_Top)
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				[
					SNew(SSlider)
					.Visibility(EVisibility::Visible)
					.MinValue(0.f)
					.MaxValue(120.f) // Seconds				
					.Value(this, &SPDGenericInputWrapper::GetMissionBranchDelay)
					.OnValueChanged(this, &SPDGenericInputWrapper::OnMissionBranchDelayChanged)
				]
				+ SHorizontalBox::Slot()
				[
					SNew(STextBlock)
					.Text(this, &SPDGenericInputWrapper::GetMissionBranchDelayAsText)
				]
			];
		return NumberBox;
	}

	default:
		break;
	}

	return SGraphPin::GetDefaultValueWidget();
}

TSharedPtr<FString> SPDGenericInputWrapper::GetMissionStateAsString() const 
{
	return MakeShared<FString>(UEnum::GetValueAsName(GetValue<EGenericInputSelector::EMissionState>()).ToString());
}
TSharedPtr<FString> SPDGenericInputWrapper::GetMissionBranchBehaviourAsString() const 
{
	return MakeShared<FString>(UEnum::GetValueAsName(GetValue<EGenericInputSelector::EMissionBranchBehaviour>()).ToString());
}
float SPDGenericInputWrapper::GetMissionTickInterval() const 
{
	return GetValue<EGenericInputSelector::EMissionTickInterval>();
}
float SPDGenericInputWrapper::GetMissionBranchDelay() const 
{
	return GetValue<EGenericInputSelector::EMissionBranchDelayTime>();
}
FText SPDGenericInputWrapper::GetMissionTickIntervalAsText() const 
{
	return FText::AsCultureInvariant(FString::Printf(TEXT("%f"), GetMissionTickInterval()));
}
FText SPDGenericInputWrapper::GetMissionBranchDelayAsText() const 
{
	return FText::AsCultureInvariant(FString::Printf(TEXT("%f"), GetMissionBranchDelay()));
}
void SPDGenericInputWrapper::OnPauseStateChanged(ECheckBoxState NewState)
{
	GetValueMutable<EGenericInputSelector::EMissionTickPaused>() = NewState;
}
void SPDGenericInputWrapper::OnRepeatableStateChanged(ECheckBoxState NewState)
{
	GetValueMutable<EGenericInputSelector::EMissionRepeatable>() = NewState;
}
void SPDGenericInputWrapper::OnMissionIDChanged(int32 NewID)
{
	GetValueMutable<EGenericInputSelector::EMissionID>() = NewID;
}
void SPDGenericInputWrapper::OnMissionIDChangedViaTag(FGameplayTag MissionTag)
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	check(MissionSubsystem) //Sanity check, should always be a valid object here

	OnMissionIDChanged(MissionSubsystem->Utility.ResolveMIDViaTag(MissionTag));
}

void SPDGenericInputWrapper::OnMissionStateChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
	switch (SelectInfo)
	{
	default:
		return;
	case ESelectInfo::OnKeyPress:
	case ESelectInfo::OnMouseClick:
		break;
	}

	const FString ActualString = *SelectedItem.Get(); 
	UEnum* MissionStateEnum = StaticEnum<EPDMissionState>();
	const EPDMissionState NewMissionState = static_cast<EPDMissionState>(MissionStateEnum->GetIndexByNameString(ActualString));
	GetValueMutable<EGenericInputSelector::EMissionState>() = NewMissionState;

}
void SPDGenericInputWrapper::OnMissionBranchBehaviourChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
	switch (SelectInfo)
	{
	default:
		return;
	case ESelectInfo::OnKeyPress:
	case ESelectInfo::OnMouseClick:
		break;
	}

	const FString ActualString = *SelectedItem.Get();
	UEnum* MissionBehaviourEnum = StaticEnum<EPDMissionBranchBehaviour>();
	const EPDMissionBranchBehaviour NewBranchBehaviour = static_cast<EPDMissionBranchBehaviour>(MissionBehaviourEnum->GetIndexByNameString(ActualString));
	GetValueMutable<EGenericInputSelector::EMissionBranchBehaviour>() = NewBranchBehaviour;
}

void SPDGenericInputWrapper::OnMissionTickIntervalChanged(float NewVal)
{
	GetValueMutable<EGenericInputSelector::EMissionTickInterval>() = NewVal;
}
void SPDGenericInputWrapper::OnMissionBranchDelayChanged(float NewVal)
{
	GetValueMutable<EGenericInputSelector::EMissionBranchDelayTime>() = NewVal;
}


//
// Label As Pin
void SPDLabelAsPin::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	MissionRowName = InArgs._MissionRowName;
	SubCategoryObject = InGraphPinObj->PinType.PinSubCategoryObject;

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget> MakePinEntry(const TSharedRef<SWidget>& InnerLabel, const TSharedRef<SWidget>& InnerContent)
{
	constexpr float PinWidth = 350.0f;
	return SNew(SHorizontalBox) 
	+ SHorizontalBox::Slot()
	.FillWidth(0.1)
	.Padding(FMargin(24,24))
	[
		InnerLabel
	]
	+ SHorizontalBox::Slot()
	[
		SNew(SBox)
		.WidthOverride(PinWidth)
		.Content()
		[
			InnerContent
		]
	];	
}

TSharedRef<SWidget>	SPDLabelAsPin::GetDefaultValueWidget() 
{
	static UScriptStruct* MissionBaseStruct = TBaseStructure<FPDMissionBase>::Get();
	static UScriptStruct* TickBehaviourStruct = TBaseStructure<FPDMissionTickBehaviour>::Get();
	static UScriptStruct* MissionRulesStruct = TBaseStructure<FPDMissionRules>::Get();
	
	static UScriptStruct* MissionTagCompoundStruct = TBaseStructure<FPDMissionTagCompound>::Get();

	if (SubCategoryObject != nullptr)
	{
		if (SubCategoryObject.Get() == MissionBaseStruct)
		{
			return MakePinEntry(SGraphPin::GetDefaultValueWidget(), GenerateSettingsContent<FPDMissionBase>(GetCurrentMission(MissionRowName), StructureDetailsView));
		}
		if (SubCategoryObject.Get() == TickBehaviourStruct)
		{
			return MakePinEntry(SGraphPin::GetDefaultValueWidget(), GenerateSettingsContent<FPDMissionTickBehaviour>(GetCurrentMission(MissionRowName), StructureDetailsView));
		}
		if (SubCategoryObject.Get() == MissionRulesStruct)
		{
			// return MakePinEntry(SGraphPin::GetDefaultValueWidget(), GenerateSettingsContent<FPDMissionRules>(GetCurrentMission(MissionRowName), StructureDetailsView));
			return MakePinEntry(SGraphPin::GetDefaultValueWidget(), GenerateSettingsContent<FPDMissionTagCompound>(GetCurrentMission(MissionRowName), StructureDetailsView));
		}
	}

	return SGraphPin::GetDefaultValueWidget();
}

const FSlateBrush* SPDLabelAsPin::GetPinIcon() const
{
	static const FName NAME_PosePin_Connected("Graph.PosePin.Connected");
	return FAppStyle::GetBrush(NAME_PosePin_Connected);
}


//
// Numeric box wrapper

//
// PinFactory
TSharedPtr<SGraphPin> FPDAttributeGraphPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	/* Compare pin-category and subcategory to make sure the pin is of the correct type  */
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionName) 
	{
		return SNew(SPDAttributePin, InPin); 
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder)
	{
		return SNew(SPDNewKeyDataAttributePin, InPin);  // @todo SPDNewKeyDataAttributePin
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_MissionDataRef && InPin->PinType.PinSubCategoryObject == FPDMissionRow::StaticStruct())
	{
		return SNew(SPDDataAttributePin, InPin);  // @todo SPDDataAttributePin
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_SectionLabel)
	{
		SPDLabelAsPin::FArguments Args;
		UPDMissionGraphNode* AsMissionGraphNode = Cast<UPDMissionGraphNode>(InPin->GetOwningNode());
		if (AsMissionGraphNode)
		{
			Args.MissionRowName(AsMissionGraphNode->SelectedMissionRowName); 
		}
		return SArgumentNew(Args, SPDLabelAsPin, InPin);
	}

	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_TagSelector)
	{
		return SNew(SPDTagSelector, InPin);
	}
	if (InPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_GenericData)
	{
		SPDGenericInputWrapper::FArguments Args;
		const FName InnerPropertyName = InPin->PinType.PinSubCategory;

		EGenericInputSelector Type = EGenericInputSelector::MAX;
		if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionStateData, EStartState)){Type = EGenericInputSelector::EMissionState;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionBranchBehaviour, Type)){Type = EGenericInputSelector::EMissionBranchBehaviour;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionBranchBehaviour, DelayTime)){Type = EGenericInputSelector::EMissionBranchDelayTime;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionTickBehaviour, Interval)){Type = EGenericInputSelector::EMissionTickInterval;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionTickBehaviour, bIsPaused)){Type = EGenericInputSelector::EMissionTickPaused;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionTickBehaviour, DeltaValue)){Type = EGenericInputSelector::EMissionTickDeltaValue;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionStateData, bRepeatable)){Type = EGenericInputSelector::EMissionRepeatable;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionBranch, Branches)){Type = EGenericInputSelector::ENextMissionBranch;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionRow, Metadata)){Type = EGenericInputSelector::EMissionMetadata;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionMetadata, Name)){Type = EGenericInputSelector::EMissionMetadata;}
		else if (InnerPropertyName == GET_MEMBER_NAME_CHECKED(FPDMissionBaseExtension, mID)){Type = EGenericInputSelector::EMissionID;}

		UPDMissionGraphNode* AsMissionGraphNode = Cast<UPDMissionGraphNode>(InPin->GetOwningNode());
		if (AsMissionGraphNode)
		{
			Args.MissionRowName(AsMissionGraphNode->SelectedMissionRowName); 
		}
		Args.InputType(Type);

		return SArgumentNew(Args, SPDGenericInputWrapper, InPin);
	}

	
	return FGraphPanelPinFactory::CreatePin(InPin);
}


namespace SKnotNodeDefinitions
{
	/** Offset from the left edge to display comment toggle button at */
	static const float KnotCenterButtonAdjust = 3.f;

	/** Offset from the left edge to display comment bubbles at */
	static const float KnotCenterBubbleAdjust = 20.f;

	/** Knot node spacer sizes */
	static const FVector2D NodeSpacerSize(42.0f, 24.0f);
}

/////////////////////////////////////////////////////
// SMissionGraphPinKnot

void SMissionGraphPinKnot::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	SGraphPin::Construct(SGraphPin::FArguments().SideToSideMargin(0.0f), InPin);
}

void SMissionGraphPinKnot::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FDragDropOperation> Operation = DragDropEvent.GetOperation();
	if (Operation.IsValid() && Operation->IsOfType<FDragConnection>())
	{
		TSharedPtr<FDragConnection> DragConnectionOp = StaticCastSharedPtr<FDragConnection>(Operation);

		TArray<UEdGraphPin*> ValidPins;
		DragConnectionOp->ValidateGraphPinList(/*out*/ ValidPins);
		
		if (ValidPins.Num() > 0)
		{
			UEdGraphPin* PinToHoverOver = nullptr;
			UEdGraphNode* Knot = GraphPinObj->GetOwningNode();
			int32 InputPinIndex = -1;
			int32 OutputPinIndex = -1;

			if (Knot != nullptr && Knot->ShouldDrawNodeAsControlPointOnly(InputPinIndex, OutputPinIndex))
			{
				// Dragging to another pin, pick the opposite direction as a source to maximize connection chances
				PinToHoverOver = (ValidPins[0]->Direction == EGPD_Input) ? Knot->GetPinAt(OutputPinIndex) : Knot->GetPinAt(InputPinIndex);
				check(PinToHoverOver);
			}

			if (PinToHoverOver != nullptr)
			{
				DragConnectionOp->SetHoveredPin(PinToHoverOver);

				// Pins treat being dragged over the same as being hovered outside of drag and drop if they know how to respond to the drag action.
				SBorder::OnMouseEnter(MyGeometry, DragDropEvent);

				return;
			}
		}
	}

	SGraphPin::OnDragEnter(MyGeometry, DragDropEvent);
}

FSlateColor SMissionGraphPinKnot::GetPinColor() const
{
	// Make ourselves transparent if we're the input, since we are underneath the output pin and would double-blend looking ugly
	if (UEdGraphPin* PinObj = GetPinObj())
	{
		if (PinObj->Direction == EEdGraphPinDirection::EGPD_Input)
		{
			return FLinearColor::Transparent;
		} 
	}
	return SGraphPin::GetPinColor();
}

TSharedRef<SWidget>	SMissionGraphPinKnot::GetDefaultValueWidget()
{
	return SNullWidget::NullWidget;
}

TSharedRef<FDragDropOperation> SMissionGraphPinKnot::SpawnPinDragEvent(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins)
{
	// FAmbivalentDirectionDragConnection::FDraggedPinTable PinHandles;
	// PinHandles.Reserve(InStartingPins.Num());
	// // since the graph can be refreshed and pins can be reconstructed/replaced 
	// // behind the scenes, the DragDropOperation holds onto FGraphPinHandles 
	// // instead of direct widgets/graph-pins
	// for (const TSharedRef<SGraphPin>& PinWidget : InStartingPins)
	// {
	// 	PinHandles.Add(PinWidget->GetPinObj());
	// }
	//
	// TSharedRef<FAmbivalentDirectionDragConnection> Operation = FAmbivalentDirectionDragConnection::New(GetPinObj()->GetOwningNode(), InGraphPanel, PinHandles);
	// return Operation;

	return TSharedRef<FDragDropOperation>(); // @ todo, missing classes only available in private modules, will have to recreate them or load in the private modules myself
}

FReply SMissionGraphPinKnot::OnPinMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!GraphPinObj->bNotConnectable && IsEditingEnabled())
		{
			if (MouseEvent.IsAltDown())
			{
				// Normally break connections, but overloaded here to delete the node entirely
				const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

				UEdGraphNode* NodeToDelete = GetPinObj()->GetOwningNode();
				if (NodeToDelete != nullptr)
				{
					UEdGraph* Graph = NodeToDelete->GetGraph();
					if (Graph != nullptr)
					{
						const UEdGraphSchema* Schema = Graph->GetSchema();
						if (Schema != nullptr && Schema->SafeDeleteNodeFromGraph(Graph, NodeToDelete))
						{
							return FReply::Handled();
						}
					}
				}

				return FReply::Unhandled();
			}
			else if (MouseEvent.IsControlDown())
			{
				// Normally moves the connections from one pin to another, but moves the node instead since it's really representing a set of connections
				// Returning unhandled will cause the node behind us to catch it and move us
				return FReply::Unhandled();
			}
		}
	}

	return SGraphPin::OnPinMouseDown(SenderGeometry, MouseEvent);
}

//////////////////////////////////////////////////////////////////////////
// SGraphNodeKnot

void SMissionGraphNodeKnot::Construct(const FArguments& InArgs, UEdGraphNode* InKnot)
{
	int32 InputPinIndex = -1;
	int32 OutputPinIndex = -1;
	verify(InKnot->ShouldDrawNodeAsControlPointOnly(InputPinIndex, OutputPinIndex) == true &&
		InputPinIndex >= 0 && OutputPinIndex >= 0);
	SGraphNodeDefault::Construct(SGraphNodeDefault::FArguments().GraphNodeObj(InKnot));
}


void SMissionGraphNodeKnot::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	//@TODO: Keyboard focus on edit doesn't work unless the node is visible, but the text is just the comment and it's already shown in a bubble, so Transparent black it is...
	InlineEditableText = SNew(SInlineEditableTextBlock)
		.ColorAndOpacity(FLinearColor::Transparent)
		.Style(FAppStyle::Get(), "Graph.Node.NodeTitleInlineEditableText")
		.Text(this, &SMissionGraphNodeKnot::GetEditableNodeTitleAsText)
		.OnVerifyTextChanged(this, &SMissionGraphNodeKnot::OnVerifyNameTextChanged)
		.OnTextCommitted(this, &SMissionGraphNodeKnot::OnNameTextCommited)
		.IsReadOnly(this, &SMissionGraphNodeKnot::IsNameReadOnly)
		.IsSelected(this, &SMissionGraphNodeKnot::IsSelectedExclusively);

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );

	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				// Grab handle to be able to move the node
				SNew(SSpacer)
				.Size(SKnotNodeDefinitions::NodeSpacerSize)
				.Visibility(EVisibility::Visible)
				.Cursor(EMouseCursor::CardinalCross)
			]

			+SOverlay::Slot()
// 			.VAlign(VAlign_Center)
// 			.HAlign(HAlign_Center)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							// LEFT
							SAssignNew(LeftNodeBox, SVerticalBox)
						]
						+SOverlay::Slot()
						[
							// RIGHT
							SAssignNew(RightNodeBox, SVerticalBox)
						]
					]
				]
			]
		];
	// Create comment bubble
	const FSlateColor CommentColor = GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor;

	SAssignNew(CommentBubble, SCommentBubble)
	.GraphNode(GraphNode)
	.Text(this, &SGraphNode::GetNodeComment)
	.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
	.EnableTitleBarBubble(true)
	.EnableBubbleCtrls(true)
	.AllowPinning(true)
	.ColorAndOpacity(CommentColor)
	.GraphLOD(this, &SGraphNode::GetCurrentLOD)
	.IsGraphNodeHovered(this, &SGraphNode::IsHovered)
	.OnToggled(this, &SGraphNode::OnCommentBubbleToggled);

	GetOrAddSlot(ENodeZone::TopCenter)
	.SlotOffset(TAttribute<FVector2D>(this, &SMissionGraphNodeKnot::GetCommentOffset))
	.SlotSize( TAttribute<FVector2D>( CommentBubble.Get(), &SCommentBubble::GetSize))
	.AllowScaling( TAttribute<bool>(CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
	.VAlign(VAlign_Top)
	[
		CommentBubble.ToSharedRef()
	];
	CreatePinWidgets();
}

const FSlateBrush* SMissionGraphNodeKnot::GetShadowBrush(bool bSelected) const
{
	return bSelected ? FAppStyle::GetBrush(TEXT("Graph.Node.ShadowSelected")) : FAppStyle::GetNoBrush();
}

TSharedPtr<SGraphPin> SMissionGraphNodeKnot::CreatePinWidget(UEdGraphPin* Pin) const
{
	return SNew(SMissionGraphPinKnot, Pin);
}

void SMissionGraphNodeKnot::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();
	PinToAdd->SetShowLabel(false);

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			//.Padding(10, 4)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else
	{
		RightNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			//.Padding(10, 4)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}
}

FVector2D SMissionGraphNodeKnot::GetCommentOffset() const
{
	const bool bBubbleVisible = GraphNode->bCommentBubbleVisible || bAlwaysShowCommentBubble;
	const float ZoomAmount = GraphNode->bCommentBubblePinned && OwnerGraphPanelPtr.IsValid() ? OwnerGraphPanelPtr.Pin()->GetZoomAmount() : 1.f;
	const float NodeWidthOffset = bBubbleVisible ?	SKnotNodeDefinitions::KnotCenterBubbleAdjust * ZoomAmount :
													SKnotNodeDefinitions::KnotCenterButtonAdjust * ZoomAmount;

	return FVector2D(NodeWidthOffset - CommentBubble->GetArrowCenterOffset(), -CommentBubble->GetDesiredSize().Y);
}

void SMissionGraphNodeKnot::OnCommentBubbleToggled(bool bInCommentBubbleVisible)
{
	SGraphNode::OnCommentBubbleToggled(bInCommentBubbleVisible);
	bAlwaysShowCommentBubble = bInCommentBubbleVisible;
}

void SMissionGraphNodeKnot::OnCommentTextCommitted(const FText& NewComment, ETextCommit::Type CommitInfo)
{
	SGraphNode::OnCommentTextCommitted(NewComment, CommitInfo);
	if (!bAlwaysShowCommentBubble && !CommentBubble->TextBlockHasKeyboardFocus() && !CommentBubble->IsHovered())
	{
		// Hide the comment bubble if visibility hasn't changed
		CommentBubble->SetCommentBubbleVisibility(/*bVisible =*/false);
	}
}

void SPDNewMissionWizard::Construct(const FArguments &InArgs, UEdGraphPin* InPin)
{
	TagCombo = 
		SNew(SGameplayTagCombo)
		.Visibility(EVisibility::Visible)
		.Tag(this, &SPDNewMissionWizard::GetSelectedTag)
		.OnTagChanged_Lambda(
			[&](const FGameplayTag NewTag)
		{
			SelectedTag = NewTag;
		});

	NewMissionButton = 
		SNew(SButton)
		.Visibility(EVisibility::Visible)
		.IsEnabled(this, &SPDNewMissionWizard::IsButtonEnabled)
		.OnClicked(this, &SPDNewMissionWizard::OnClicked)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::AsCultureInvariant(FString(TEXT("Create Mission"))))
		];


	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			TagCombo.ToSharedRef()	
		]
		+ SHorizontalBox::Slot()
		[
			NewMissionButton.ToSharedRef()
		]
	];
}

FReply SPDNewMissionWizard::OnClicked()
{
	// TODO: Create new table entry here using the tag
    return FReply::Handled();
}

bool SPDNewMissionWizard::IsButtonEnabled() const
{
    return SelectedTag != FGameplayTag::EmptyTag;
}

#undef LOCTEXT_NAMESPACE

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