/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Mission/Graph/PDMissionGraphNodeBase.h"
#include "Mission/Graph/PDMissionGraphSchema.h"
#include "Mission/Graph/PDMissionGraph.h"
#include "Mission/Graph/PDPinManager.h"
#include "Mission/Slate/PDMissionView.h"
#include "Subsystems/PDMissionUtility.h"
#include "Subsystems/PDMissionSubsystem.h"

#include "Interfaces/IPluginManager.h"
#include "UObject/ObjectSaveContext.h"


#define LOCTEXT_NAMESPACE "FMissionTreeNode"

UPDMissionGraphNode::UPDMissionGraphNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bMadeAfterOverridePinRemoval(false)
{
	static UScriptStruct* MissionRowStruct = TBaseStructure<FPDMissionRow>::Get();
	StructType = MissionRowStruct; 

	NodeInstance = nullptr;
	CopySubNodeIndex = 0;
	bIsReadOnly = false;
	bIsSubNode = false;
}

void UPDMissionGraphNode::InitializeInstance()
{
	// empty in base class
}

void UPDMissionGraphNode::PostPlacedNewNode()
{
	// NodeInstance can be already spawned by paste operation, don't override it

	const UClass* NodeClass = ClassData.GetClass(true);
	if (NodeClass == nullptr || (NodeInstance != nullptr)) { return; }
	
	const UEdGraph* MyGraph = GetGraph();
	const UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;
	if (GraphOwner == nullptr) { return;}
		
	// StructOnScopePropertyOwner = NewObject<UStruct>(GraphOwner, NodeStruct);
	NodeInstance = NewObject<UStruct>(GetTransientPackage(), TEXT("StructOnScope"), RF_Transient);
	NodeInstance->SetFlags(RF_Transactional);
	InitializeInstance();
	
	bMadeAfterOverridePinRemoval = true;	
}

bool UPDMissionGraphNode::CanDuplicateNode() const
{
	return bIsReadOnly ? false : Super::CanDuplicateNode();
}

bool UPDMissionGraphNode::CanUserDeleteNode() const
{
	// This is not working, must be controlled elsewhere. I need to look at the context action schema stuff again I think
	const bool bIsConnected = nullptr != Pins.FindByPredicate([](const UEdGraphPin* PinElem) -> bool{ return false == PinElem->LinkedTo.IsEmpty(); });
	return bIsReadOnly || bIsConnected ? false : Super::CanUserDeleteNode();
}

void UPDMissionGraphNode::PrepareForCopying()
{
	if (NodeInstance == nullptr) { return; }
	
	// Temporarily take ownership of the node instance, so that it is not deleted when cutting
	NodeInstance->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
}

void UPDMissionGraphNode::PostEditImport()
{
	ResetNodeOwner();
	if (NodeInstance == nullptr) { return; }

	InitializeInstance();
}

void UPDMissionGraphNode::PostEditUndo()
{
	UEdGraphNode::PostEditUndo();
	ResetNodeOwner();
	if (ParentNode == nullptr) { return; }

	ParentNode->SubNodes.AddUnique(this);
}

void UPDMissionGraphNode::PostCopyNode()
{
	ResetNodeOwner();
}

void UPDMissionGraphNode::ResetNodeOwner()
{
	if (NodeInstance == nullptr) { return; }
	
	const UEdGraph* MyGraph = GetGraph();
	UObject* GraphOwner = MyGraph ? MyGraph->GetOuter() : nullptr;

	NodeInstance->Rename(nullptr, GraphOwner, REN_DontCreateRedirectors | REN_DoNotDirty);
	NodeInstance->ClearFlags(RF_Transient);

	for (const TObjectPtr<UPDMissionGraphNode>& SubNode : SubNodes)
	{
		SubNode->ResetNodeOwner();
	}
}

void UPDMissionGraphNode::CreateMissionPin()
{
	UE_LOG(LogTemp, Warning, TEXT("================== MISSIONTEST ==================="));
	UE_LOG(LogTemp, Warning, TEXT("MISSIONTEST: UPDMissionGraphNode::CreateMissionPin"));

	FEdGraphTerminalType ValueTerminalType;
	ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Struct;
	ValueTerminalType.TerminalSubCategory = NAME_None;
	ValueTerminalType.TerminalSubCategoryObject = FPDMissionRow::StaticStruct();
		
	// FEdGraphPinType(FName InPinCategory, FName InPinSubCategory, UObject* InPinSubCategoryObject, EPinContainerType InPinContainerType, bool bInIsReference, const FEdGraphTerminalType& InValueTerminalType )
	FEdGraphPinType PinType_Mission(
		FPDMissionGraphTypes::PinCategory_MissionName,
		NAME_None,
		nullptr,
		EPinContainerType::None,
		false,
		ValueTerminalType);
	
	PinType_Mission.bIsConst = false;
	CreatePin(EGPD_Input, PinType_Mission, TEXT("Mission Root"), 2);
}

void UPDMissionGraphNode::CreateOutputBranchPins()
{
	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	FDataTableRowHandle* RowHandle = MissionSubsystem->Utility.MissionLookupViaRowName.Find(SelectedMissionRowName);
	if (RowHandle)
	{
		FPDMissionRow* SelectedMissionRow = RowHandle->GetRow<FPDMissionRow>(TEXT("UPDMissionGraphNode::CreateOutputBranchPins"));
		if(SelectedMissionRow)
		{
			int32 BranchIdx = 0;
			for (FPDMissionBranchElement& Branch : SelectedMissionRow->ProgressRules.NextMissionBranch.Branches)
			{
				const FString PinName = UPDMissionEditorStatics::NodeOp::BuildPinName(BranchIdx, Branch);
				CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, *PinName);
				++BranchIdx;

				// TODO: Need to tell the node it can add output buttons AND then I need to update the actual table with new entries for each
				// TODO: Think of potentially updating node when a branchtarget is changed in the property details view, as this part of the node might be obscured then this could be ignored perhaps
			}
		}
	}
}


void UPDMissionGraphNode::RefreshDataRefPins(const FName& MissionRowName)
{
	UE_LOG(LogTemp, Warning, TEXT("================== MISSIONTEST ==================="));
	UE_LOG(LogTemp, Warning, TEXT("MISSIONTEST: UPDMissionGraphNode::RefreshDataRefPins - %s"), *MissionRowName.ToString());

	SelectedMissionRowName = MissionRowName;
	if ((MissionRowName.ToString() == TAG_MakeNewMission.GetTag().ToString() || MissionRowName == NAME_None) && GetInputPin(0) != nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MISSIONTEST: Attempt create new mission"));

		// TODO/NOTE: 
		//	- This is currenlty unused, dead code basically, or even worse as it actually runs. 
		//	- Commenting it out and rethinking what we need when we want to create a new mission row

		// // Generate pin. TODO: Try to remember what the hell the plan was here nad then finish or scrap
		// FEdGraphTerminalType ValueTerminalType;
		// ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Struct;
		// ValueTerminalType.TerminalSubCategory = NAME_None;
		// ValueTerminalType.TerminalSubCategoryObject = FPDMissionRow::StaticStruct();
		
		// UEdGraphNode::FCreatePinParams PinParam;
		// PinParam.Index = 3;
		// PinParam.bIsReference = false;
		// PinParam.ContainerType = EPinContainerType::None;
		// PinParam.ValueTerminalType = ValueTerminalType;

		// FEdGraphPinType PinType_Key(FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder, NAME_None, FPDMissionRow::StaticStruct(), PinParam.ContainerType, PinParam.bIsReference, PinParam.ValueTerminalType);
		// PinType_Key.bIsConst = PinParam.bIsConst;
		// // CreatePin(EGPD_Input, PinType_Key, TEXT("New data key"), PinParam.Index);
	}

	// Always refresh, but TODO: Make it possible to save the state and apply on the new missions pins when 
	// changing from an existing mission to creating a new mission via 'TAG_MakeNewMission', it will be needed in certain cases
	GetMissionGraph()->Modify();
	ReallocateDefaultPins();
	GetMissionGraph()->NotifyGraphChanged();
	GetMissionGraph()->UpdateData();		

	PreviousMissionRowName = SelectedMissionRowName;
}

FText UPDMissionGraphNode::GetDescription() const
{
	FString StoredClassName = ClassData.GetDataEntryName();
	StoredClassName.RemoveFromEnd(TEXT("_C"));
	
	return StoredClassName.IsEmpty() == false ? FText::FromString(StoredClassName) : FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
}

FText UPDMissionGraphNode::GetTooltipText() const
{
	if (NodeInstance == nullptr)
	{
		FString StoredClassName = ClassData.GetDataEntryName();
		StoredClassName.RemoveFromEnd(TEXT("_C"));
		return FText::Format(LOCTEXT("NodeClassError", "Class {0} not found, make sure it's saved!"), FText::FromString(StoredClassName));
	}

	if (NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) == false)
	{
		return NodeInstance->GetClass()->GetToolTipText();
	}
	
	FText TooltipDesc;
	const FAssetData AssetData(NodeInstance->GetClass()->ClassGeneratedBy);
	FString Description = AssetData.GetTagValueRef<FString>(GET_MEMBER_NAME_CHECKED(UBlueprint, BlueprintDescription));
	if (Description.IsEmpty() == false)
	{
		Description.ReplaceInline(TEXT("\\n"), TEXT("\n"));
		TooltipDesc = FText::FromString(MoveTemp(Description));
	}

	return TooltipDesc;
}
const FVector2D IconDims(200.0f, 200.0f);
TSharedPtr<SWidget> UPDMissionGraphNode::CreateNodeImage() const
{
	static FString ResourcePath = IPluginManager::Get().FindPlugin("PDMissionCore")->GetBaseDir() / TEXT("Resources") / TEXT("pdMIcon.svg");
	static FSlateVectorImageBrush Brush = FSlateVectorImageBrush(ResourcePath, IconDims);
	const FSlateBrush* BrushUpClassed = static_cast<FSlateBrush*>(&Brush);
	SNew(SImage)
	.ColorAndOpacity(NodeColours.Body)
	.RenderOpacity(.5)
	.Image(BrushUpClassed);
	
	return Super::CreateNodeImage();
}

UEdGraphPin* UPDMissionGraphNode::GetInputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	int32 PinIndex = 0, FoundInputs = 0;
	const int32 PinLimit = Pins.Num();
	for (; PinIndex < PinLimit; PinIndex++)
	{
		if (Pins[PinIndex]->Direction != EGPD_Input) { continue; }
		
		if (InputIndex == FoundInputs) { return Pins[PinIndex]; }
		
		FoundInputs++;
	}

	return nullptr;
}

UEdGraphPin* UPDMissionGraphNode::GetOutputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	int32 PinIndex = 0, FoundInputs = 0;
	const int32 PinLimit = Pins.Num();	
	for (; PinIndex < PinLimit; PinIndex++)
	{
		if (Pins[PinIndex]->Direction != EGPD_Output) { continue; }
		
		if (InputIndex == FoundInputs) { return Pins[PinIndex]; }
		
		FoundInputs++;
	}

	return nullptr;
}

void UPDMissionGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin == nullptr) { return; }
	
	UEdGraphPin* OutputPin = GetOutputPin();
	if (GetSchema()->TryCreateConnection(FromPin, GetInputPin()))
	{
		FromPin->GetOwningNode()->NodeConnectionListChanged();
	}
	else if (OutputPin != nullptr && GetSchema()->TryCreateConnection(OutputPin, FromPin))
	{
		NodeConnectionListChanged();
	}
}

UPDMissionGraph* UPDMissionGraphNode::GetMissionGraph()
{
	return CastChecked<UPDMissionGraph>(GetGraph());
}

bool UPDMissionGraphNode::IsSubNode() const
{
	return bIsSubNode || (ParentNode != nullptr);
}

FLinearColor UPDMissionGraphNode::GetNodeBodyTintColor() const
{
	return NodeColours.Body;
}

FLinearColor UPDMissionGraphNode::GetNodeTitleColor() const
{
	return NodeColours.Title;
}

FLinearColor UPDMissionGraphNode::GetNodeCommentColor() const
{
	return NodeColours.Comment;
}

FText UPDMissionGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return Super::GetNodeTitle(TitleType);
}

void UPDMissionGraphNode::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	GetMissionGraph()->UpdateData();
}

void UPDMissionGraphNode::OnPinRemoved(UEdGraphPin* PinToRemove)
{
	if(PinToRemove)
	{
		UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
		FDataTableRowHandle* RowHandlePtr = MissionSubsystem->Utility.MissionLookupViaRowName.Find(SelectedMissionRowName);
		if (RowHandlePtr)
		{
			FPDMissionRow* MissionRow = RowHandlePtr->GetRow<FPDMissionRow>(TEXT("UPDMissionGraphNode::OnPinRemoved"));
			if (MissionRow)
			{
				const int32 BranchIdx = UPDMissionEditorStatics::NodeOp::GetBranchIdxFromPinName(PinToRemove);
				TArray<FPDMissionBranchElement>& Branches = MissionRow->ProgressRules.NextMissionBranch.Branches;
				if (Branches.IsValidIndex(BranchIdx))
				{
					RowHandlePtr->DataTable->MarkPackageDirty();
					Branches.RemoveAt(BranchIdx);
				}
			}
		}
	}	
}


TSharedPtr<SGraphNode> UPDMissionGraphNode::CreateVisualWidget()
{
	return SNew(SMissionGraphNode, this);
}

bool UPDMissionGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	check(DesiredSchema);
	return DesiredSchema->GetClass()->IsChildOf(UPDMissionGraphSchema::StaticClass());
}

FString DescribeProperty(const FProperty* Prop, const uint8* PropertyAddr)
{
	FString ExportedStringValue;
	const FStructProperty* StructProp = CastField<const FStructProperty>(Prop);
	const FFloatProperty* FloatProp = CastField<const FFloatProperty>(Prop);

	if (StructProp && StructProp->Struct && StructProp->Struct->IsChildOf(TBaseStructure<FGameplayTag>::Get()))
	{
		ExportedStringValue = reinterpret_cast<const FGameplayTag*>(PropertyAddr)->ToString();

#if WITH_EDITOR
		const FString CategoryLimit = StructProp->GetMetaData(TEXT("Categories"));
		if (!CategoryLimit.IsEmpty() && ExportedStringValue.StartsWith(CategoryLimit))
		{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 3 ) || ENGINE_MAJOR_VERSION < 5
			ExportedStringValue.MidInline(CategoryLimit.Len(), MAX_int32, false);
#else
			ExportedStringValue.MidInline(CategoryLimit.Len(), MAX_int32, EAllowShrinking::No);
#endif
		}
#endif
	}
	else if (FloatProp)
	{
		// special case for floats to remove unnecessary zeros
		const float FloatValue = FloatProp->GetPropertyValue(PropertyAddr);
		ExportedStringValue = FString::SanitizeFloat(FloatValue);
	}
	else
	{
		Prop->ExportTextItem_Direct(ExportedStringValue, PropertyAddr, nullptr, nullptr, PPF_PropertyWindow, nullptr);
	}

	const bool bIsBool = Prop->IsA(FBoolProperty::StaticClass());
	return FString::Printf(TEXT("%s: %s"), *FName::NameToDisplayString(Prop->GetName(), bIsBool), *ExportedStringValue);
}

FString UPDMissionGraphNode::GetPropertyNameAndValueForDiff(const FProperty* Prop, const uint8* PropertyAddr) const
{
	return DescribeProperty(Prop, PropertyAddr);
}

void UPDMissionGraphNode::FindDiffs(UEdGraphNode* OtherNode, FDiffResults& Results)
{
	Super::FindDiffs(OtherNode, Results);

	const UPDMissionGraphNode* OtherGraphNode = Cast<UPDMissionGraphNode>(OtherNode);
	if (OtherGraphNode == nullptr || NodeInstance == nullptr || OtherGraphNode->NodeInstance == nullptr)
	{
		return;
	}
	
	FDiffSingleResult Diff;
	Diff.Diff = EDiffType::NODE_PROPERTY;
	Diff.Node1 = this;
	Diff.Node2 = OtherNode;
	Diff.ToolTip = LOCTEXT("DIF_NodeInstancePropertyToolTip", "A property of the node instance has changed");
	Diff.Category = EDiffType::MODIFICATION;
	
	DiffProperties(NodeInstance->GetClass(), OtherGraphNode->NodeInstance->GetClass(), NodeInstance, OtherGraphNode->NodeInstance, Results, Diff);
}

void UPDMissionGraphNode::AddSubNode(UPDMissionGraphNode* SubNode, class UEdGraph* ParentGraph)
{
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	Modify();

	SubNode->SetFlags(RF_Transactional);

	// set outer to be the graph so it doesn't go away
	SubNode->Rename(nullptr, ParentGraph, REN_NonTransactional);
	SubNode->ParentNode = this;

	SubNode->CreateNewGuid();
	SubNode->PostPlacedNewNode();
	SubNode->AllocateDefaultPins();
	SubNode->AutowireNewNode(nullptr);

	SubNode->NodePosX = 0;
	SubNode->NodePosY = 0;

	SubNodes.Add(SubNode);
	OnSubNodeAdded(SubNode);

	ParentGraph->NotifyGraphChanged();
	GetMissionGraph()->UpdateData();
}

void UPDMissionGraphNode::OnSubNodeAdded(UPDMissionGraphNode* SubNode)
{
	// empty in base class
}

void UPDMissionGraphNode::RemoveSubNode(UPDMissionGraphNode* SubNode)
{
	Modify();
	SubNodes.RemoveSingle(SubNode);

	OnSubNodeRemoved(SubNode);
}

void UPDMissionGraphNode::RemoveAllSubNodes()
{
	SubNodes.Reset();
}

void UPDMissionGraphNode::OnSubNodeRemoved(UPDMissionGraphNode* SubNode)
{
	// empty in base class
}

int32 UPDMissionGraphNode::FindSubNodeDropIndex(UPDMissionGraphNode* SubNode) const
{
	return SubNodes.IndexOfByKey(SubNode);
}

void UPDMissionGraphNode::InsertSubNodeAt(UPDMissionGraphNode* SubNode, int32 DropIndex)
{
	int32 ResultIndex = DropIndex > -1 ? SubNodes.Insert(SubNode, DropIndex) : SubNodes.Add(SubNode);
}

void UPDMissionGraphNode::DestroyNode()
{
	if (ParentNode) { ParentNode->RemoveSubNode(this); }

	UEdGraphNode::DestroyNode();
}

bool UPDMissionGraphNode::UsesBlueprint() const
{
	return NodeInstance && NodeInstance->GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
}

void UPDMissionGraphNode::UpdateNodeClassData()
{
	if (NodeInstance == nullptr) { return; }
	UpdateNodeDataFrom(NodeInstance->GetClass(), ClassData);
}

void UPDMissionGraphNode::GetMissionTransitions(TArray<UPDMissionTransitionNode*>& Array)
{
	// @todo finish implement 
}

void UPDMissionGraphNode::UpdateNodeDataFrom(UClass* InstanceClass, FPDMissionNodeData& UpdatedData)
{
	if (InstanceClass == nullptr) { return; }
	
	if (const UBlueprint* BPOwner = Cast<UBlueprint>(InstanceClass->ClassGeneratedBy))
	{
		UpdatedData = FPDMissionNodeData(BPOwner->GetName(), BPOwner->GetOutermost()->GetName(), InstanceClass->GetName(), InstanceClass);
		return;
	}
	
	if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(InstanceClass))
	{
		UpdatedData = FPDMissionNodeData(BPGC->GetClassPathName(), BPGC);
		return;
	}
	
	UpdatedData = FPDMissionNodeData(InstanceClass);
}

bool UPDMissionGraphNode::HasErrors() const
{
	return NodeInstance == nullptr;
}

// This is horrible, I know.. 
// TODO: I should really just make an subclass to UEdGraphPin and use that instead, 
// and store branch data there so we can have the pin name always reflect the proper name 
// while still being able to easily target the branch index in the datatablerow
void UPDMissionGraphNode::RemoveBranchPin(UEdGraphPin* Pin)
{
	check(Pin->Direction != EGPD_Input);

	int32 FoundPinIdx = INDEX_NONE;
	TArray<UEdGraphPin*> OutPins = Pins.FilterByPredicate([](const UEdGraphPin* PinElem) -> bool {return PinElem->Direction != EEdGraphPinDirection::EGPD_Input;});
	for (UEdGraphPin* OutPin : OutPins)
	{
		if (Pin == OutPin)
		{
			break;
		}
		++FoundPinIdx;
	};

	RemovePin(Pin);

	TArray<UEdGraphPin*> RemainderOutPins = Pins.FilterByPredicate([](const UEdGraphPin* PinElem) -> bool {return PinElem->Direction != EEdGraphPinDirection::EGPD_Input;});
	const int32 OutPinLim = RemainderOutPins.Num();
	for (int32 OutPinIdx = FoundPinIdx; OutPinIdx < OutPinLim; OutPinIdx++)
	{
		if (false == RemainderOutPins.IsValidIndex(OutPinIdx))
		{
			break;
		}

		UEdGraphPin* OutPin = RemainderOutPins[OutPinIdx];
		UPDMissionEditorStatics::NodeOp::OffsetPinName(OutPin);
	};

	UEdGraph* ParentGraph = GetGraph();
	const FScopedTransaction Transaction(LOCTEXT("AddNode", "Add Node"));
	ParentGraph->Modify();
	Modify();

	ParentGraph->NotifyGraphChanged();
	GetMissionGraph()->UpdateData();
}
void UPDMissionGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Pin && Context->Pin->Direction != EEdGraphPinDirection::EGPD_Input)
	{
		FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaPinActions", LOCTEXT("PinActionsMenuHeader", "Pin Actions"));
		Section.AddMenuEntry(
			"RemovePin",
			LOCTEXT("RemovePin", "Remove branch"),
			LOCTEXT("RemovePinTooltip", "Remove this mission branch pin"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(const_cast<UPDMissionGraphNode*>(this), &UPDMissionGraphNode::RemoveBranchPin, const_cast<UEdGraphPin*>(Context->Pin))
			)
		);
	}
}

void UPDMissionGraphNode::AllocateDefaultPins()
{
	if (StructType == nullptr) { return; }
	
	PreloadObject(StructType);
	
	FStructOnScope StructOnScope(StructType);
	FPDOptionalPinManager OptionalPinManager(StructOnScope.GetStructMemory());
	OptionalPinManager.RebuildPropertyList(ShowPinForProperties, StructType);
	OptionalPinManager.CreateVisiblePins(ShowPinForProperties, StructType, EGPD_Input, this);
	
	for(UEdGraphPin* Pin : Pins)
	{
		Pin->bDefaultValueIsIgnored = Pin->bDefaultValueIsIgnored || Pin->PinType.IsContainer();
	}
}

void UPDMissionGraphNode::ReallocateDefaultPins()
{
	if (StructType == nullptr) { return; }
	
	PreloadObject(StructType);
	FStructOnScope StructOnScope(StructType);
	FPDOptionalPinManager OptionalPinManager(StructOnScope.GetStructMemory());


	TArray<UEdGraphPin*> OldPins = Pins;

	const bool bIsEntryPointNode = 
		nullptr == OldPins.FindByPredicate(
			[](const UEdGraphPin* PinElem) -> bool
			{
				return PinElem->Direction == EGPD_Input && PinElem->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath;
			});
	if (false == bIsEntryPointNode)
	{
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("In"));
	}
	
	CreateMissionPin();
	CreateOutputBranchPins();
	// CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("Out"));

	const bool bNewMission = SelectedMissionRowName == TAG_MakeNewMission.GetTag().ToString();
	OptionalPinManager.RebuildPropertyList(ShowPinForProperties, StructType);
	OptionalPinManager.CreateVisiblePins(ShowPinForProperties, StructType, EGPD_Input, this, bNewMission);

	TArray<UEdGraphPin*> NewPins = Pins.FilterByPredicate(
		[OldPins](const UEdGraphPin* NewPinElem) -> bool
		{
			return false == OldPins.Contains(NewPinElem);
		});

	CopyLogicalPath(OldPins, NewPins);

	for (UEdGraphPin* OldPin : OldPins) 
	{
		RemovePin(OldPin);
	}

	for(UEdGraphPin* Pin : Pins)
	{
		Pin->bDefaultValueIsIgnored = Pin->bDefaultValueIsIgnored || Pin->PinType.IsContainer();
	}
}

void UPDMissionGraphNode::CopyLogicalPath(TArray<UEdGraphPin*> OldPins, TArray<UEdGraphPin*> NewPins)
{
#define ISLOGICALPATH(Elem) Elem->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath
#define ISINPUT(Elem) Elem->Direction == EEdGraphPinDirection::EGPD_Input && ISLOGICALPATH(Elem)
#define ISOUTPUT(Elem) Elem->Direction != EEdGraphPinDirection::EGPD_Input && ISLOGICALPATH(Elem)

	UEdGraphPin** OldInPin = OldPins.FindByPredicate([](const UEdGraphPin* PinElem) -> bool {return ISINPUT(PinElem);});
	UEdGraphPin** NewInPin = NewPins.FindByPredicate([](const UEdGraphPin* PinElem) -> bool {return ISINPUT(PinElem);});

	UEdGraphPin** OldOutPin = OldPins.FindByPredicate([](const UEdGraphPin* PinElem) -> bool {return ISOUTPUT(PinElem);});
	UEdGraphPin** NewOutPin = NewPins.FindByPredicate([](const UEdGraphPin* PinElem) -> bool {return ISOUTPUT(PinElem);});

	// Copying links to logical path, w
	if (OldInPin && NewInPin && *OldInPin && *NewInPin)
	{
		for (UEdGraphPin* OldLinkToPin : (*OldInPin)->LinkedTo){(*NewInPin)->MakeLinkTo(OldLinkToPin);}
	}
	if (OldOutPin && NewOutPin && *OldOutPin && *NewOutPin)
	{
		for (UEdGraphPin* OldLinkToPin : (*OldOutPin)->LinkedTo){(*NewOutPin)->MakeLinkTo(OldLinkToPin);}
	}	
}	


void UPDMissionGraphNode::PreloadRequiredAssets()
{
	static UScriptStruct* MissionRowStruct = TBaseStructure<FPDMissionRow>::Get();
    StructType = MissionRowStruct;
	
	PreloadObject(StructType);
}

void UPDMissionGraphNode::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	// @todo Validation needed when the system has been more fleshed out
}

FSlateIcon UPDMissionGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.MakeStruct_16x");
	return Icon;
}

FText UPDMissionGraphNode::GetMenuCategory() const
{
	// @todo review if needed
	return FText(); // FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Struct);
}

void UPDMissionGraphNode::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	// @todo review if needed
}


void UPDMissionGraphNode::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && !Ar.IsTransacting() && !HasAllFlags(RF_Transient))
	{
		// @todo review if needed
	}
}


#undef LOCTEXT_NAMESPACE // "FMissionTreeNode"


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