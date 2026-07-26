/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

// Mission editor
#include "PDMissionGraphTypes.h"

#include "Mission/FPDMissionEditor.h"
#include "Mission/Graph/PDMissionGraphNodes.h"
#include "Mission/Graph/PDMissionGraphNodes.h"
#include "Mission/Slate/PDMissionView.h"
#include "Subsystems/PDMissionSubsystem.h"
#include "Subsystems/PDMissionUtility.h"

// Engine
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "UObject/ConstructorHelpers.h"
#include "ObjectEditorUtils.h"
#include "Modules/ModuleManager.h"

#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Misc/PackageName.h"
#include "Logging/MessageLog.h"
#include "Misc/FeedbackContext.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/CompositeDataTable.h"

#define LOCTEXT_NAMESPACE "MissionGraph"


// Tab identifiers
const FName FPDMissionEditorTabs::GraphDetailsID(TEXT("MissionEditor_Properties"));
const FName FPDMissionEditorTabs::SearchID(TEXT("MissionEditor_Search"));
const FName FPDMissionEditorTabs::TreeEditorID(TEXT("MissionEditor_Tree"));

// Document tab identifiers
const FName FPDMissionEditorTabs::GraphEditorID(TEXT("Document"));

const FName FPDMissionGraphTypes::PinCategory_String("String");
const FName FPDMissionGraphTypes::PinCategory_Text("Text");

const FName FPDMissionGraphTypes::PinCategory_GenericData("GenericData");

const FName FPDMissionGraphTypes::PinCategory_SectionLabel("SectionLabel");
const FName FPDMissionGraphTypes::PinCategory_NewMission("NewMission");
const FName FPDMissionGraphTypes::PinCategory_TagSelector("TagSelector");
const FName FPDMissionGraphTypes::PinCategory_MissionName("MissionName");
const FName FPDMissionGraphTypes::PinCategory_MissionRow("MissionRow");
const FName FPDMissionGraphTypes::PinCategory_MissionDataRef("MissionDataRef");
const FName FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder("MissionRowKeyBuilder");
const FName FPDMissionGraphTypes::PinCategory_LogicalPath("LogicalPath");

const FText FPDMissionGraphTypes::NodeText_MainMission(LOCTEXT("Missions","MainMission"));
const FText FPDMissionGraphTypes::NodeText_SideMission(LOCTEXT("Missions","SideMission"));
const FText FPDMissionGraphTypes::NodeText_EventMission(LOCTEXT("Missions","EventMission"));



bool FindConditionNode(UEdGraphPin*& SourcePin, const bool bStartAsInputPin)
{
	for (;nullptr != SourcePin;)
	{
		if (SourcePin->LinkedTo.IsEmpty())
		{
			// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - No linked pins anymore  "))
			return false;
		}
		UPDMissionGraphNode* KnotSourceAsMissionNode = Cast<UPDMissionGraphNode>(SourcePin->LinkedTo[0]->GetOwningNode());
		UPDMissionGraphNode_Knot* KnotSourceAsKnot = Cast<UPDMissionGraphNode_Knot>(KnotSourceAsMissionNode);
		UPDMissionGraphNode_ConditionNode* KnotSourceAsCondition = Cast<UPDMissionGraphNode_ConditionNode>(KnotSourceAsMissionNode);
		if (KnotSourceAsKnot && nullptr == KnotSourceAsCondition)
		{
			// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Found knot, keep searching path  "))
			SourcePin = bStartAsInputPin ? KnotSourceAsKnot->GetInputPin() : KnotSourceAsKnot->GetOutputPin();
		}
		else if (KnotSourceAsCondition)
		{
			// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Found Condition, SUCCESS"))
			return true;
		}
		else 
		{
			// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Found Mission, FAIL"))
			return false;
		}
	}
	// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - No valid starting pin, FAIL"))
	return false;
}


UPDMissionGraphNode* UPDMissionEditorStatics::NodeOp::ResolveNextNode(const UEdGraphPin* PinOnPotentialKnot)
{
	UEdGraphPin* InPin = PinOnPotentialKnot->LinkedTo.IsEmpty() ? nullptr : PinOnPotentialKnot->LinkedTo[0];
	if  (nullptr == InPin)
	{
		return nullptr;
	}

		
	UPDMissionGraphNode* KnotSourceAsMissionNode = Cast<UPDMissionGraphNode>(InPin->GetOwningNode());
	if (KnotSourceAsMissionNode)
	{
		return KnotSourceAsMissionNode;
	}

	return nullptr;
}


TTuple<UPDMissionGraphNode*, UEdGraphPin*> UPDMissionEditorStatics::NodeOp::ResolveMissionNodeFromKnot(UEdGraphPin* PinOnPotentialKnot, const EEdGraphPinDirection PinDir)
{
	const bool bStartAsInputPin = PinDir == EEdGraphPinDirection::EGPD_Input;

	TTuple<UPDMissionGraphNode*, UEdGraphPin*> ResolvedNodeAndPin{nullptr, nullptr};
	if (UPDMissionGraphNode_Knot* TopLevelNodeAsKnot = Cast<UPDMissionGraphNode_Knot>(PinOnPotentialKnot->GetOwningNode()))
	{
		UEdGraphPin* InPin_Knot = bStartAsInputPin ? TopLevelNodeAsKnot->GetInputPin() : TopLevelNodeAsKnot->GetOutputPin();
		for (; nullptr != InPin_Knot;)
		{
			if (InPin_Knot->LinkedTo.IsEmpty())
			{
				InPin_Knot = nullptr;
				break;
			}
			
			UPDMissionGraphNode* KnotSourceAsMissionNode = Cast<UPDMissionGraphNode>(InPin_Knot->LinkedTo[0]->GetOwningNode());
			UPDMissionGraphNode_Knot* KnotSourceAsKnot = Cast<UPDMissionGraphNode_Knot>(KnotSourceAsMissionNode);
			if (KnotSourceAsMissionNode && nullptr == KnotSourceAsKnot)
			{
				ResolvedNodeAndPin.Key = KnotSourceAsMissionNode;
				ResolvedNodeAndPin.Value = PinOnPotentialKnot = InPin_Knot; 
				InPin_Knot = nullptr;
			}
			else if (KnotSourceAsKnot)
			{
				InPin_Knot = bStartAsInputPin ? KnotSourceAsKnot->GetInputPin() : KnotSourceAsKnot->GetOutputPin();
			}
		}
	}

	return ResolvedNodeAndPin;
}

bool UPDMissionEditorStatics::NodeOp::DoesNodePathContainConditionNode(UEdGraphPin* SourcePin, const EEdGraphPinDirection PinDir)
{
	// UE_LOG(LogTemp, Warning, TEXT("=========================CONDTEST========================="))
	// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - SourcePin(%s)"), *SourcePin->PinName.ToString())
	// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - PinDir(%s)"), PinDir == EEdGraphPinDirection::EGPD_Input ? *FString(TEXT("Input")) : *FString(TEXT("Output")) )
	if (UPDMissionGraphNode_ConditionNode* TopLevelNodeAsCondition = Cast<UPDMissionGraphNode_ConditionNode>(SourcePin->GetOwningNode()))
	{
		// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Top level node is cond - SUCCESS"))
		return true;
	}

	const bool bStartAsInputPin = PinDir == EEdGraphPinDirection::EGPD_Input;
	if(UPDMissionGraphNode_Knot* TopLevelNodeAsKnot = Cast<UPDMissionGraphNode_Knot>(SourcePin->GetOwningNode()))
	{
		// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Case: Top Level as Knot "))
		UEdGraphPin* InPin_Knot = bStartAsInputPin ? TopLevelNodeAsKnot->GetInputPin() : TopLevelNodeAsKnot->GetOutputPin();
		return FindConditionNode(InPin_Knot, bStartAsInputPin);
	}
	else if(UPDMissionGraphNode* TopLevelNodeAsMission = Cast<UPDMissionGraphNode>(SourcePin->GetOwningNode()))
	{
		// UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - Case: Top Level as Mission "))
		UEdGraphPin* InPin = SourcePin;
		return FindConditionNode(InPin, bStartAsInputPin);
	}
		
	UE_LOG(LogTemp, Warning, TEXT("CONDTEST: DoesNodePathContainConditionNode - No node in line is cond - FAIL"))
	return false;
}


bool UPDMissionEditorStatics::NodeOp::IsRowBasedMissionNode(UEdGraphNode* Node)
{
	return Cast<UPDMissionGraphNode>(Node) && nullptr == Cast<UPDMissionGraphNode_Knot>(Node) && nullptr == Cast<UPDMissionTransitionNode>(Node);
}


bool UPDMissionEditorStatics::RowOp::IsMissionValid(const FName& SelectedMissionRowName, FDataTableRowHandle*& OutRowHandlePtr)
{
	OutRowHandlePtr = UPDMissionStatics::GetMissionSubsystem()->Utility.MissionLookupViaRowName.Find(SelectedMissionRowName);
	return nullptr != OutRowHandlePtr;
}
void UPDMissionEditorStatics::RowOp::SetValuesOnBranchTargetAtIndex(const FName& SourceMissionRowName, int32 BranchIdx, const FName& BranchTargetMissionName, const FString Ctxt)
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	FDataTableRowHandle* RowHandlePtr = MissionSubsystem->Utility.MissionLookupViaRowName.Find(SourceMissionRowName);
	FPDMissionBranchElement UpdatedBranchElement = GetBranch(RowHandlePtr, BranchIdx, Ctxt);

	// BranchElem.bIsDirectBranch; // TODO handle this visually somehow, need to think about how though
	// BranchElem.BranchConditions; // TODO handle this visually somehow, need to think about how though
	// BranchElem.TargetBehaviour; // TODO handle this visually somehow, need to think about how though
	if (FDataTableRowHandle* TargetRowHandlePtr = MissionSubsystem->Utility.MissionLookupViaRowName.Find(BranchTargetMissionName))
	{
		UpdatedBranchElement.Target = *TargetRowHandlePtr;
	}

	UpdateBranch(RowHandlePtr, BranchIdx, UpdatedBranchElement, Ctxt);
}


FPDMissionNodeData::FPDMissionNodeData(UClass* InClass) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass)
{
	Category = GetCategory();
	if (InClass) { ClassName = InClass->GetName(); }
}

FPDMissionNodeData::FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UClass* InClass) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass),
	AssetName(InGeneratedClassPath.GetAssetName().ToString()),
	GeneratedPackage(InGeneratedClassPath.GetPackageName().ToString()),
	ClassName(InGeneratedClassPath.GetAssetName().ToString())
{
	Category = GetCategory();
}

FPDMissionNodeData::FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InClass) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass),
	AssetName(InAssetName),
	GeneratedPackage(InGeneratedClassPackage),
	ClassName(InClassName)
{
	Category = GetCategory();
}

FString FPDMissionNodeData::ToString() const
{
	FString ShortName = GetDisplayName();
	if (!ShortName.IsEmpty())
	{
		return ShortName;
	}

	const UClass* MyClass = Class.Get();
	if (MyClass)
	{
		FString ClassDesc = MyClass->GetName();
		const int32 ShortNameIdx = ClassDesc.Find(TEXT("_"), ESearchCase::CaseSensitive);
		if (ShortNameIdx != INDEX_NONE)
		{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3 ) || ENGINE_MAJOR_VERSION < 5
			ClassDesc.MidInline(ShortNameIdx + 1, MAX_int32, false);
#else
			ClassDesc.MidInline(ShortNameIdx + 1, MAX_int32, EAllowShrinking::No);
#endif
		}

		return ClassDesc;
	}

	return AssetName;
}

FString FPDMissionNodeData::GetDataEntryName() const
{
	return Class.IsValid() ? Class->GetName() : ClassName;
}

FString FPDMissionNodeData::GetDisplayName() const
{
	return Class.IsValid() ? Class->GetMetaData(TEXT("DisplayName")) : FString();
}

FText FPDMissionNodeData::GetTooltip() const
{
	return Class.IsValid() ? Class->GetToolTipText() : FText::GetEmpty();
}

FText FPDMissionNodeData::GetCategory() const
{
	return Class.IsValid() ? FObjectEditorUtils::GetCategoryText(Class.Get()) : Category;
}


UClass* FPDMissionNodeData::GetClass(bool bSilent)
{
	UClass* RetClass = Class.Get();
	if (RetClass == nullptr && GeneratedPackage.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(nullptr, *GeneratedPackage, LOAD_NoRedirects);
		if (Package)
		{
			Package->FullyLoad();

			RetClass = FindObject<UClass>(Package, *ClassName);

			GWarn->EndSlowTask();
			Class = RetClass;
		}
		else
		{
			GWarn->EndSlowTask();

			if (!bSilent)
			{
				FMessageLog EditorErrors("EditorErrors");
				EditorErrors.Error(LOCTEXT("PackageLoadFail", "Package Load Failed"));
				EditorErrors.Info(FText::FromString(GeneratedPackage));
				EditorErrors.Notify(LOCTEXT("PackageLoadFail", "Package Load Failed"));
			}
		}
	}

	return RetClass;
}

bool FPDMissionNodeData::operator==(const FPDMissionNodeData& Other) const
{
	return
		bHideParent      == Other.bHideParent      &&
		bIsHidden        == Other.bIsHidden        &&
		Class.Get()      == Other.Class.Get()      &&
		AssetName        == Other.AssetName        &&
		GeneratedPackage == Other.GeneratedPackage &&
		ClassName        == Other.ClassName        &&
		Category.EqualTo(Other.Category);
}

bool FPDMissionNodeData::operator!=(const FPDMissionNodeData& Other) const
{
	return (Other == *this) == false;
}

void FPDMissionDebuggerHandler::BindDebuggerToolbarCommands()
{
	// @todo
}

void FPDMissionDebuggerHandler::RefreshDebugger()
{
	// @todo
}

FText FPDMissionDebuggerHandler::HandleGetDebugKeyValue(const FName& InKeyName, bool bUseCurrentState) const
{
	// @todo
	return FText::GetEmpty();
}

float FPDMissionDebuggerHandler::HandleGetDebugTimeStamp(bool bUseCurrentState) const
{
	// @todo
	return 0.0f;
}

void FPDMissionDebuggerHandler::OnEnableBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnToggleBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnDisableBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnAddBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnRemoveBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnSearchMissionDatabase()
{
	// @todo
}

FReply FPDMissionDebuggerHandler::JumpToNode(const UEdGraphNode* Node)
{
	// @todo

	return FReply::Handled();
}
void FPDMissionDebuggerHandler::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	// @todo
}

void FPDMissionDebuggerHandler::UpdateToolbar()
{
	// @todo
}


bool FPDMissionDebuggerHandler::CanEnableBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanToggleBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanDisableBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanAddBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanRemoveBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanSearchMissionDatabase() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsPropertyEditable() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsDebuggerReady() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsDebuggerPaused() const
{
	// @todo
	return false;
}

TSharedRef<SWidget> FPDMissionDebuggerHandler::OnGetDebuggerActorsMenu()
{
	// @todo
	return SNew(SGraphEditor);
}

FText FPDMissionDebuggerHandler::GetDebuggerActorDesc() const
{
	// @todo
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE


FPDMissionGraphConnectionDrawingPolicy::FPDMissionGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
}

void FPDMissionGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	Params.bUserFlag2 = true;
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = 1.5f;

	if (InputPin)
	{
		if (UPDMissionTransitionNode* TransNode = Cast<UPDMissionTransitionNode>(InputPin->GetOwningNode()))
		{
			const bool IsInputPinHovered = HoveredPins.Contains(InputPin);
			Params.WireColor = SGraphNodeMissionTransition::StaticGetTransitionColor(TransNode, IsInputPinHovered);
			Params.bUserFlag1 = 0x0; // never bidirectional
		}
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}

	// Make the transition that is currently relinked, semi-transparent.
	for (const FRelinkConnection& Connection : RelinkConnections)
	{
		if (InputPin == nullptr || OutputPin == nullptr)
		{
			continue;
		}
		const FGraphPinHandle SourcePinHandle = Connection.SourcePin;
		const FGraphPinHandle TargetPinHandle = Connection.TargetPin;

		// Skip all transitions that don't start at the node our dragged and relink transition starts from
		if (OutputPin->GetOwningNode()->NodeGuid == SourcePinHandle.NodeGuid)
		{
			// Safety check to verify if the node is a transition node
			if (UPDMissionTransitionNode* TransitonNode = Cast<UPDMissionTransitionNode>(InputPin->GetOwningNode()))
			{
				if (UEdGraphPin* TransitionOutputPin = TransitonNode->GetOutputPin())
				{
					if (TargetPinHandle.NodeGuid == TransitionOutputPin->GetOwningNode()->NodeGuid)
					{
						Params.WireColor.A *= 0.2f;
					}
				}
			}
		}
	}
}

void FPDMissionGraphConnectionDrawingPolicy::DetermineLinkGeometry(
	FArrangedChildren& ArrangedNodes,
	TSharedRef<SWidget>& OutputPinWidget,
	UEdGraphPin* OutputPin,
	UEdGraphPin* InputPin,
	/*out*/ FArrangedWidget*& StartWidgetGeometry,
	/*out*/ FArrangedWidget*& EndWidgetGeometry
	)
{
	if (UPDMissionGraphNode_EntryPoint* EntryNode = Cast<UPDMissionGraphNode_EntryPoint>(OutputPin->GetOwningNode()))
	{
		StartWidgetGeometry = PinGeometries->Find(OutputPinWidget);

		UPDMissionGraphNode* State = CastChecked<UPDMissionGraphNode>(InputPin->GetOwningNode());
		int32 StateIndex = NodeWidgetMap.FindChecked(State);
		EndWidgetGeometry = &(ArrangedNodes[StateIndex]);
	}
	else if (UPDMissionTransitionNode* TransNode = Cast<UPDMissionTransitionNode>(InputPin->GetOwningNode()))
	{
		UPDMissionGraphNode* PrevState = TransNode->GetOwningMission();
		UPDMissionGraphNode* NextState = TransNode->GetTargetMission();
		if ((PrevState != NULL) && (NextState != NULL))
		{
			int32* PrevNodeIndex = NodeWidgetMap.Find(PrevState);
			int32* NextNodeIndex = NodeWidgetMap.Find(NextState);
			if ((PrevNodeIndex != NULL) && (NextNodeIndex != NULL))
			{
				StartWidgetGeometry = &(ArrangedNodes[*PrevNodeIndex]);
				EndWidgetGeometry = &(ArrangedNodes[*NextNodeIndex]);
			}
		}
	}
	else
	{
		StartWidgetGeometry = PinGeometries->Find(OutputPinWidget);

		if (TSharedPtr<SGraphPin>* pTargetWidget = PinToPinWidgetMap.Find(InputPin))
		{
			TSharedRef<SGraphPin> InputWidget = (*pTargetWidget).ToSharedRef();
			EndWidgetGeometry = PinGeometries->Find(InputWidget);
		}
	}
}

void FPDMissionGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

// NEW NOTE 0: Sometimes we fail to generate a new branch it seems like, not sure why it happens and as it happens sporadically have been hard to debug but seems like it could be a datarace, 
// 		TODO: Remember to set up the the RW lock on modfying the branch data
// NEW NOTE 1: This does in fact not only iterate one direction, but I've blocked off my custom logic unless source is made from a valid mission row type
// OLD NOTE: I am pretty certain this always parses 'left' to 'right' in the graph, and it would probably break if it read any other way, source pin would break, good to keep in mind if epic ever changes the graph parsing logic. Also consider rewriting
void FPDMissionGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();

	// TODO In the middle or remaking this, but need to push and commit as I need to head out

	UPDMissionEditorSubsystem::ProcessPinConnection(Params);
	FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
}


void FPDMissionGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

	const FVector2D SeedPoint = EndPoint;
	const FVector2D AdjustedStartPoint = FGeometryHelper::FindClosestPointOnGeom(PinGeometry, SeedPoint);

	Params.bUserFlag2 = false; // bUserFlag2 is used to indicate whether the drawn arrow is a preview transition (the temporary transition when creating or relinking).
	DrawSplineWithArrow(AdjustedStartPoint, EndPoint, Params);
}


void FPDMissionGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	Internal_DrawLineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);

	// Is the connection bidirectional?
	if (Params.bUserFlag1)
	{
		Internal_DrawLineWithArrow(EndAnchorPoint, StartAnchorPoint, Params);
	}
}

void FPDMissionGraphConnectionDrawingPolicy::Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	//@TODO: Should this be scaled by zoom factor?
	const float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = StartAnchorPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = EndAnchorPoint + DirectionBias - LengthBias;
	FLinearColor ArrowHeadColor = Params.WireColor;

	// Draw a line/spline
	DrawConnection(WireLayerID, StartPoint, EndPoint, Params);

	const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
	const double AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

	// Draw the transition grab handles in case the mouse is hovering the transition
	bool StartHovered = false;
	bool EndHovered = false;
	const FVector FVecMousePos = FVector(LocalMousePosition.X, LocalMousePosition.Y, 0.0f);
	const FVector ClosestPoint = FMath::ClosestPointOnSegment(FVecMousePos,
		FVector(StartPoint.X, StartPoint.Y, 0.0f),
		FVector(EndPoint.X, EndPoint.Y, 0.0f));
	if ((ClosestPoint - FVecMousePos).Length() < RelinkHandleHoverRadius)
	{
		StartHovered = FVector2D(StartPoint - LocalMousePosition).Length() < RelinkHandleHoverRadius;
		EndHovered = FVector2D(EndPoint - LocalMousePosition).Length() < RelinkHandleHoverRadius;
		FVector2D HoverIndicatorPosition = StartHovered ? StartPoint : EndPoint;

		// Set the hovered pin results. This will be used by the SGraphPanel again.
		const float SquaredDistToPin1 = (Params.AssociatedPin1 != nullptr) ? (StartPoint - LocalMousePosition).SizeSquared() : FLT_MAX;
		const float SquaredDistToPin2 = (Params.AssociatedPin2 != nullptr) ? (EndPoint - LocalMousePosition).SizeSquared() : FLT_MAX;
		if (EndHovered)
		{
			SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2, SquaredDistToPin2, SquaredDistToPin1, SquaredDistToPin2, true);
		}

		// Draw grab handles only in case no relinking operation is performed
		if (RelinkConnections.IsEmpty() && Params.bUserFlag2)
		{
			if (EndHovered)
			{
				// Draw solid orange circle behind the arrow head in case the arrow head is hovered (area that enables a relink).
				FSlateRoundedBoxBrush RoundedBoxBrush = FSlateRoundedBoxBrush(
					FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), 9.0f, FStyleColors::AccentOrange, 100.0f);

				FSlateDrawElement::MakeBox(DrawElementsList,
					ArrowLayerID-1, // Draw behind the arrow
					FPaintGeometry(EndPoint - ArrowRadius, BubbleImage->ImageSize * ZoomFactor, ZoomFactor),
					&RoundedBoxBrush);

				ArrowHeadColor = FLinearColor::Black;
			}
			else
			{
				// Draw circle around the arrow in case the transition is hovered (mouse close or over transition line or arrow head).
				const int CircleLineSegments = 16;
				const float CircleRadius = 10.0f;
				const FVector2D CircleCenter = EndPoint - UnitDelta * 2.0f;
				DrawCircle(CircleCenter, CircleRadius, Params.WireColor, CircleLineSegments);
			}
		}
	}

	// Draw the number of relinked transitions on the preview transition.
	if (!RelinkConnections.IsEmpty() && !Params.bUserFlag2)
	{
		// Get the number of actually relinked transitions.
		int32 NumRelinkedTransitions = 0;
		for (const FRelinkConnection& Connection : RelinkConnections)
		{
			NumRelinkedTransitions += UPDMissionTransitionNode::GetListTransitionNodesToRelink(Connection.SourcePin, Connection.TargetPin, SelectedGraphNodes).Num();

			if (UPDMissionGraphNode_EntryPoint* EntryNode = Cast<UPDMissionGraphNode_EntryPoint>(Connection.SourcePin->GetOwningNode()))
			{
				NumRelinkedTransitions += 1;
			}
		}

		const FVector2D TransitionCenter = StartAnchorPoint + DeltaPos * 0.5f;
		const FVector2D TextPosition = TransitionCenter + Normal * 15.0f * ZoomFactor;

		FSlateDrawElement::MakeText(
			DrawElementsList,
			ArrowLayerID,
			FPaintGeometry(TextPosition, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
			FText::AsNumber(NumRelinkedTransitions),
			FCoreStyle::Get().GetFontStyle("SmallFont"));
	}

	// Draw the transition arrow triangle
	FSlateDrawElement::MakeRotatedBox(
		DrawElementsList,
		ArrowLayerID,
		FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
		ArrowImage,
		ESlateDrawEffect::None,
		static_cast<float>(AngleInRadians),
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		ArrowHeadColor
	);
}

void FPDMissionGraphConnectionDrawingPolicy::DrawCircle(const FVector2D& Center, float Radius, const FLinearColor& Color, const int NumLineSegments)
{
	TempPoints.Empty();

	const float NumFloatLineSegments = (float)NumLineSegments;
	for (int i = 0; i <= NumLineSegments; i++)
	{
		const float Angle = (i / NumFloatLineSegments) * TWO_PI;

		FVector2D PointOnCircle;
		PointOnCircle.X = cosf(Angle) * Radius;
		PointOnCircle.Y = sinf(Angle) * Radius;
		TempPoints.Add(PointOnCircle);
	}

	FSlateDrawElement::MakeLines(
		DrawElementsList,
		ArrowLayerID + 1,
		FPaintGeometry(Center, FVector2D(Radius, Radius) * ZoomFactor, ZoomFactor),
		TempPoints,
		ESlateDrawEffect::None,
		Color
	);
}

void FPDMissionGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
}

FVector2D FPDMissionGraphConnectionDrawingPolicy::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	const FVector2D Delta = End - Start;
	const FVector2D NormDelta = Delta.GetSafeNormal();

	return NormDelta;
}



#define LOCTEXT_NAMESPACE "AssetTypeActions"

uint32 FAssetTypeActions_MissionEditor::GetCategories()
{
	return EAssetTypeCategories::Gameplay;
}

void FAssetTypeActions_MissionEditor::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Object : InObjects)
	{
		UDataTable* Bank = Cast<UDataTable>(Object);
		if (Bank == nullptr) { continue; }

		TArray<FName> SlowRandomKeyList{};
		Bank->GetRowMap().GetKeys(SlowRandomKeyList);
		FPDMissionNodeHandle ConstructedNodeData;
		ConstructedNodeData.DataTarget.DataTable = Bank;
		ConstructedNodeData.DataTarget.RowName = SlowRandomKeyList.IsEmpty() ? NAME_None : SlowRandomKeyList[0];
		FFPDMissionGraphEditor::CreateMissionEditor(Mode, EditWithinLevelEditor, ConstructedNodeData);
	}
}

UClass* FAssetTypeActions_MissionEditor::GetSupportedClass() const
{
	return UPDMissionDataTable::StaticClass();
}


void FAssetTypeActions_MissionEditor::OpenInDefaults(UDataTable* OldBank, UDataTable* NewBank) const
{
	const FString OldTextFilename = DumpAssetToTempFile(OldBank);
	const FString NewTextFilename = DumpAssetToTempFile(NewBank);

	// Get diff program to use
	const FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	// AssetToolsModule.Get().CreateDiffProcess(DiffCommand, OldTextFilename, NewTextFilename); // @todo
}

UPDMissionEditorSubsystem* UPDMissionEditorSubsystem::Get()
{
	return GEngine->GetEngineSubsystem<UPDMissionEditorSubsystem>();
}


// TODO: Move this logic into the plugins editor module, forgot I was in the plugins game module
void UPDMissionEditorSubsystem::QueueConditionNode(const FPDPendingConditionNode& PendingConditionNode)
{
	check(PendingConditionNode.SourceNode.IsValid())

	// TODO/NOTE: If I run into any threading errors then add a RWLock for the queue
	PendingNodesToCreate.Emplace(PendingConditionNode.SourceNode.Get(), PendingConditionNode);
}
void UPDMissionEditorSubsystem::SpawnConditionNodes()
{
	for (const auto&[Key, PendingConditionNode] : PendingNodesToCreate)
	{
		const FScopedTransaction Transaction(FText::FromString("Auto Insert Node from Wire State"));
		PendingConditionNode.TargetGraph->Modify();		

		UPDMissionGraphNode_ConditionNode* NewConditionNode = NewObject<UPDMissionGraphNode_ConditionNode>(PendingConditionNode.TargetGraph);
		NewConditionNode->CreateNewGuid();


		NewConditionNode->NodePosX = PendingConditionNode.SpawnLocation.X;
		NewConditionNode->NodePosY = PendingConditionNode.SpawnLocation.Y;
		PendingConditionNode.TargetGraph->AddNode(NewConditionNode, true, true);
		NewConditionNode->PostPlacedNewNode();
		NewConditionNode->AllocateDefaultPins();


		UPDMissionGraphNode* SourceNode = Cast<UPDMissionGraphNode>(PendingConditionNode.SourceNode);
		UEdGraphPin* SourcePin = nullptr; 
		if (SourceNode && SourceNode->Pins.IsValidIndex(PendingConditionNode.SourceBranchPinIdx))
		{
			SourcePin = SourceNode->Pins[PendingConditionNode.SourceBranchPinIdx];
			NewConditionNode->GetInputPin()->MakeLinkTo(SourcePin);
		}


		UPDMissionGraphNode* TargetNode = Cast<UPDMissionGraphNode>(PendingConditionNode.TargetNode);
		UPDMissionGraphNode* TargetNodeAsKnot = Cast<UPDMissionGraphNode_Knot>(PendingConditionNode.TargetNode);
		if(TargetNode && nullptr == TargetNodeAsKnot)
		{
			UEdGraphPin** TargetPin = TargetNode->Pins.FindByPredicate([](const UEdGraphPin* PinElem)
			{ 
				return
					PinElem->Direction == EEdGraphPinDirection::EGPD_Input
					&& PinElem->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath;
			});

			if (TargetPin && *TargetPin)
			{
				if (SourcePin)
				{
					(*TargetPin)->BreakLinkTo(SourcePin);
				}
				NewConditionNode->GetOutputPin()->MakeLinkTo(*TargetPin);
			}
			else 
			{
				UE_LOG(LogTemp, Error, TEXT("UPDMissionEditorSubsystem::SpawnConditionNodes - Could not find valid traget pin to connect to"))
			}
		}
		else if (TargetNodeAsKnot)
		{
			if (SourcePin)
			{
				TargetNodeAsKnot->GetInputPin()->BreakLinkTo(SourcePin);
			}
			NewConditionNode->GetOutputPin()->MakeLinkTo(TargetNodeAsKnot->GetInputPin());
		}

		// TODO: Connect to neighbours 

		PendingConditionNode.TargetGraph->NotifyGraphChanged();		
	}
	PendingNodesToCreate.Empty();
}


void UPDMissionEditorSubsystem::ProcessPinConnection(const FConnectionParams& Params)
{
	UEdGraphPin* SourcePin = Params.AssociatedPin1; 
	UEdGraphPin* TargetPin = Params.AssociatedPin2;
	 
	if (SourcePin && TargetPin && UPDMissionEditorStatics::NodeOp::IsRowBasedMissionNode(Cast<UPDMissionGraphNode>(SourcePin->GetOwningNode())))
	{
		UPDMissionGraphNode* SourceMissionNode = Cast<UPDMissionGraphNode>(SourcePin->GetOwningNode());

		auto[TargetMissionNode, ResolvedTargetPin] = UPDMissionEditorStatics::NodeOp::ResolveMissionNodeFromKnot(TargetPin, EEdGraphPinDirection::EGPD_Output);
		if (nullptr == TargetMissionNode)
		{
			TargetMissionNode = Cast<UPDMissionGraphNode>(TargetPin->GetOwningNode());
		}

		if (UPDMissionEditorStatics::NodeOp::IsRowBasedMissionNode(SourceMissionNode) 
			&& UPDMissionEditorStatics::NodeOp::IsRowBasedMissionNode(TargetMissionNode))
		{
			const int32 OutPinIdx = UPDMissionEditorStatics::NodeOp::FindDirectionIndex(SourcePin, SourceMissionNode->Pins, EEdGraphPinDirection::EGPD_Output);
			if (INDEX_NONE == OutPinIdx)
			{
				return;
			}
			
			const bool bNodePathHasCondition = UPDMissionEditorStatics::NodeOp::DoesNodePathContainConditionNode(SourcePin, EEdGraphPinDirection::EGPD_Output);
			if (false == bNodePathHasCondition)
			{
				FPDPendingConditionNode NodeSpawnData;
				NodeSpawnData.SourceNode = SourceMissionNode;
				NodeSpawnData.SourceBranchPinIdx = Params.AssociatedPin1->GetOwningNode()->Pins.IndexOfByPredicate([Params](const UEdGraphPin*PinElem) -> bool {return PinElem == Params.AssociatedPin1;});
				NodeSpawnData.TargetNode =  UPDMissionEditorStatics::NodeOp::ResolveNextNode(Params.AssociatedPin1);
				NodeSpawnData.TargetGraph = SourceMissionNode->GetGraph();

				constexpr int32 NodeOffsetLarge = 512 + 256;
				constexpr int32 NodeOffsetSmall = 32;
				constexpr int32 BranchOffsetMultiplier = 3;
				const int32 NodeOffsetY = NodeOffsetSmall + (NodeOffsetSmall * BranchOffsetMultiplier * OutPinIdx);
				const int32 NewNodePostionX = SourceMissionNode->NodePosX + (SourceMissionNode->NodeWidth == 0 ? NodeOffsetLarge : SourceMissionNode->NodeWidth + NodeOffsetSmall);
				const int32 NewNodePostionY = SourceMissionNode->NodePosY + NodeOffsetY;
				
				NodeSpawnData.SpawnLocation = FIntVector2{NewNodePostionX, NewNodePostionY};
				UPDMissionEditorSubsystem::Get()->QueueConditionNode(NodeSpawnData);
			}

			SourcePin->PinName = FName(*UPDMissionEditorStatics::NodeOp::BuildPinName(OutPinIdx, TargetMissionNode->GetMissionName()));
			UPDMissionEditorStatics::RowOp::SetValuesOnBranchTargetAtIndex(SourceMissionNode->SelectedMissionRowName, OutPinIdx, TargetMissionNode->SelectedMissionRowName, TEXT("FPDMissionGraphConnectionDrawingPolicy::DrawConnection"));
		}
	}	
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