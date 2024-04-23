/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

// Mission editor
#include "Mission/Graph/PDMissionGraphSchema.h"
#include "Mission/Graph/PDMissionGraphSchemaActions.h"
#include "Mission/Graph/PDMissionGraphNodes.h"
#include "PDMissionGraphTypes.h"
#include "Mission/Graph/PDPinManager.h"

// Edgraph
#include "EdGraph/EdGraph.h"
#include "Textures/SlateIcon.h"

// Transations and Actions
#include "ToolMenu.h"
#include "ScopedTransaction.h"
#include "ToolMenuSection.h"
#include "GraphEditorActions.h"

#include "Internationalization/TextPackageNamespaceUtil.h"

// Commands
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Commands/GenericCommands.h"
#include "Engine/UserDefinedStruct.h"

#define LOCTEXT_NAMESPACE "MissionGraph"


#include "Mission/Graph/PDMissionGraphNodeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PDMissionGraphSchema)


//
// Graph Schema (Mission)

int32 UPDMissionGraphSchema::CurrentCacheRefreshID = 0;

UPDMissionGraphSchema::UPDMissionGraphSchema(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}


void UPDMissionGraphSchema::GetGraphNodeContextActions(FGraphContextMenuBuilder& ContextMenuBuilder, int32 SubNodeFlags) const
{
	const FPDMissionNodeHandle DummyData; // @todo replace dummy with actual data
	AddMissionNodeOptions(TEXT("Entry Point"), ContextMenuBuilder, DummyData);
}

void UPDMissionGraphSchema::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	if (Context->Node && Context->Pin == nullptr)
	{
		const UPDMissionGraphNode* MissionGraphNode = Cast<const UPDMissionGraphNode>(Context->Node);
		if (MissionGraphNode && MissionGraphNode->CanPlaceBreakpoints())
		{
			FToolMenuSection& Section = Menu->AddSection("EdGraphSchemaBreakpoints", LOCTEXT("BreakpointsHeader", "Breakpoints"));
			Section.AddMenuEntry(FGraphEditorCommands::Get().ToggleBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().AddBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().RemoveBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().EnableBreakpoint);
			Section.AddMenuEntry(FGraphEditorCommands::Get().DisableBreakpoint);
		}
	}
	
	if (Context->Node)
	{
		{
			FToolMenuSection& Section = Menu->AddSection("PDMissionGraphSchemaNodeActions", LOCTEXT("ClassActionsMenuHeader", "Node Actions"));
			Section.AddMenuEntry(FGenericCommands::Get().Delete);
			Section.AddMenuEntry(FGenericCommands::Get().Cut);
			Section.AddMenuEntry(FGenericCommands::Get().Copy);
			Section.AddMenuEntry(FGenericCommands::Get().Duplicate);

			Section.AddMenuEntry(FGraphEditorCommands::Get().BreakNodeLinks);
		}
	}

	Super::GetContextMenuActions(Menu, Context);
}

void UPDMissionGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakNodeLinks", "Break Node Links"));

	Super::BreakNodeLinks(TargetNode);
}

void UPDMissionGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakPinLinks", "Break Pin Links"));

	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

void UPDMissionGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
	const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "GraphEd_BreakSinglePinLink", "Break Pin Link"));

	Super::BreakSinglePinLink(SourcePin, TargetPin);
}

FLinearColor UPDMissionGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::White;
}

bool UPDMissionGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	check(Pin != nullptr);
	return Pin->bDefaultValueIsIgnored;
}

class FConnectionDrawingPolicy* UPDMissionGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FPDMissionGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

TSharedPtr<FEdGraphSchemaAction> UPDMissionGraphSchema::GetCreateCommentAction() const
{
	return TSharedPtr<FEdGraphSchemaAction>(static_cast<FEdGraphSchemaAction*>(new FMissionSchemaAction_AddComment));
}

int32 UPDMissionGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	if (Graph == nullptr) { return 0; }
	
	const TSharedPtr<SGraphEditor> GraphEditorPtr = SGraphEditor::FindGraphEditorForGraph(Graph);
	if (GraphEditorPtr.IsValid() == false) { return 0; }
	
	return GraphEditorPtr->GetNumberOfSelectedNodes();
}


void UPDMissionGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	FGraphNodeCreator<UPDMissionGraphNode_EntryPoint> NodeCreator(Graph);
	UPDMissionGraphNode_EntryPoint* MyNode = NodeCreator.CreateNode();
	NodeCreator.Finalize();
	SetNodeMetaData(MyNode, FNodeMetadata::DefaultGraphNode);
}

UClass* UPDMissionGraphSchema::GetSubNodeClass(EMissionGraphSubNodeType SubNodeFlag) const
{
	// @todo. Make actual subnode classes, currenlty these are actually the main nodes
	TArray<FPDMissionNodeData> TempClassData;
	switch (SubNodeFlag)
	{
	case EMissionGraphSubNodeType::MainQuest:
		return UPDMissionGraphNode_MainQuest::StaticClass();
	case EMissionGraphSubNodeType::SideQuest:
		return UPDMissionGraphNode_SideQuest::StaticClass();
	case EMissionGraphSubNodeType::EventQuest:
		return UPDMissionGraphNode_EventQuest::StaticClass();
	default:
		break;
	}

	return nullptr;
}

void UPDMissionGraphSchema::AddAndSetupNodeAction(FGraphContextMenuBuilder& ContextMenuBuilder, FCategorizedGraphActionListBuilder& ListBuilder, const TPair<UClass*, FPDMissionNodeData>& NodePair)
{
	const FText NodeTypeName = FText::FromString(FName::NameToDisplayString(NodePair.Value.ToString(), false));
	const TSharedPtr<FMissionSchemaAction_NewNode> AddOpAction = MakeShared<FMissionSchemaAction_NewNode>(NodePair.Value.GetCategory(), NodeTypeName, FText::GetEmpty(), 0);
	UPDMissionGraphNode* OpNode = NewObject<UPDMissionGraphNode>(ContextMenuBuilder.OwnerOfTemporaries, NodePair.Key);
	OpNode->ClassData = NodePair.Value;
	AddOpAction->NodeTemplate = OpNode;
	
	ListBuilder.AddAction(AddOpAction);
}

struct FPrivateNodeSelector // @todo move out of here later
{
	static UClass* Op(UClass* TestClass)
	{
		if (TestClass->IsChildOf(UPDMissionGraphNode_MainQuest::StaticClass()))  { return UPDMissionGraphNode_MainQuest::StaticClass(); }
		if (TestClass->IsChildOf(UPDMissionGraphNode_SideQuest::StaticClass()))  { return UPDMissionGraphNode_SideQuest::StaticClass(); }
		if (TestClass->IsChildOf(UPDMissionGraphNode_EventQuest::StaticClass())) { return UPDMissionGraphNode_EventQuest::StaticClass(); }
		if (TestClass->IsChildOf(UPDMissionGraphNode_Knot::StaticClass()))       { return UPDMissionGraphNode_Knot::StaticClass(); }
		if (TestClass->IsChildOf(UPDMissionGraphNode_EntryPoint::StaticClass())) { return UPDMissionGraphNode_EntryPoint::StaticClass(); }
		if (TestClass->IsChildOf(UPDMissionGraphNode::StaticClass())) { return UPDMissionGraphNode::StaticClass(); }
		return UPDMissionGraphNode_EntryPoint::StaticClass(); // fallback
	}
};

void UPDMissionGraphSchema::GatherNativeClass(TMap<UClass*, FPDMissionNodeData>& NodeClasses, UClass* TestClass) const
{
	if (TestClass->HasAnyClassFlags(CLASS_Native) == false || TestClass->IsChildOf(UPDMissionGraphNode::StaticClass()) == false)
	{
		return;
	}

	FPDMissionNodeData NodeClass(TestClass);
	NodeClass.bHideParent = false;
	NodeClass.bIsHidden = false;
	NodeClasses.Emplace(FPrivateNodeSelector::Op(TestClass),NodeClass);
}

void UPDMissionGraphSchema::AddMissionNodeOptions(const FString& CategoryName, FGraphContextMenuBuilder& ContextMenuBuilder, const FPDMissionNodeHandle& NodeData) const
{
	// @todo fix after initial test
	FCategorizedGraphActionListBuilder ListBuilder(CategoryName);
	
	// gather all native classes
	TMap<UClass*, FPDMissionNodeData> NodeClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* TestClass = *It;
		GatherNativeClass(NodeClasses, TestClass);
	}

	// Add and set-up the gathered classes in NodeClasses 
	for (const TPair<UClass*, FPDMissionNodeData>& NodePair : NodeClasses)
	{
		AddAndSetupNodeAction(ContextMenuBuilder, ListBuilder, NodePair);
	}

	ContextMenuBuilder.Append(ListBuilder);
}

void UPDMissionGraphSchema::GetPinDefaultValuesFromString(const FEdGraphPinType& PinType, UObject* OwningObject, const FString& NewDefaultValue, FString& UseDefaultValue, TObjectPtr<UObject>& UseDefaultObject, FText& UseDefaultText, bool bPreserveTextIdentity)
{
	if ((PinType.PinCategory == UEdGraphSchema_K2::PC_Object)
		|| (PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
		|| (PinType.PinCategory == UEdGraphSchema_K2::PC_Interface))
	{
		FString ObjectPathLocal = NewDefaultValue;
		ConstructorHelpers::StripObjectClass(ObjectPathLocal);

		// If this is not a full object path it's a relative path so should be saved as a string
		if (FPackageName::IsValidObjectPath(ObjectPathLocal))
		{
			FSoftObjectPath AssetRef = ObjectPathLocal;
			UseDefaultValue.Empty();
			// @todo: why are we resolving here? We should resolve explicitly 
			// during load or not at all
			if(!GCompilingBlueprint)
			{
				UseDefaultObject = AssetRef.TryLoad();
			}
			else
			{
				UseDefaultObject = AssetRef.ResolveObject();
			}
			UseDefaultText = FText::GetEmpty();
		}
		else
		{
			// "None" should be saved as empty string
			if (ObjectPathLocal == TEXT("None"))
			{
				ObjectPathLocal.Empty();
			}

			UseDefaultValue = MoveTemp(ObjectPathLocal);
			UseDefaultObject = nullptr;
			UseDefaultText = FText::GetEmpty();
		}
	}
	else if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text)
	{
		if (bPreserveTextIdentity)
		{
			UseDefaultText = FTextStringHelper::CreateFromBuffer(*NewDefaultValue);
		}
		else
		{
			FString PackageNamespace;
#if USE_STABLE_LOCALIZATION_KEYS
			if (GIsEditor)
			{
				PackageNamespace = TextNamespaceUtil::EnsurePackageNamespace(OwningObject);
			}
#endif // USE_STABLE_LOCALIZATION_KEYS
			UseDefaultText = FTextStringHelper::CreateFromBuffer(*NewDefaultValue, nullptr, *PackageNamespace);
		}
		UseDefaultObject = nullptr;
		UseDefaultValue.Empty();
	}
	else
	{
		UseDefaultValue = NewDefaultValue;
		UseDefaultObject = nullptr;
		UseDefaultText = FText::GetEmpty();

		if (PinType.PinCategory == UEdGraphSchema_K2::PC_Byte && UseDefaultValue.IsEmpty())
		{
			UEnum* EnumPtr = Cast<UEnum>(PinType.PinSubCategoryObject.Get());
			if (EnumPtr)
			{
				// Enums are stored as empty string in autogenerated defaults, but should turn into the first value in array 
				UseDefaultValue = EnumPtr->GetNameStringByIndex(0);
			}
		}
		else if ((PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject) || (PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass))
		{
			if (UseDefaultValue == FName(NAME_None).ToString())
			{
				UseDefaultValue.Reset();
			}
			else
			{
				ConstructorHelpers::StripObjectClass(UseDefaultValue);
			}
		}
	}
}

static void ConformAutogeneratedDefaultValuePackage(
	const FEdGraphPinType& PinType, 
	const UEdGraphNode* OwningNode, 
	FString& AutogeneratedDefaultValue, 
	const FText& DefaultTextValue
)
{
	if (PinType.PinCategory == UEdGraphSchema_K2::PC_Text &&
		!PinType.IsContainer() &&
		!AutogeneratedDefaultValue.IsEmpty())
	{
		// Attempt to find the correct package namespace to use for text within this pin
		// Favor using the node if we have it, as that will be most up-to-date
		FString PackageNamespace;
		if (OwningNode)
		{
			PackageNamespace = TextNamespaceUtil::GetPackageNamespace(OwningNode);
		}
		else if (!DefaultTextValue.IsEmpty())
		{
			PackageNamespace = TextNamespaceUtil::ExtractPackageNamespace(FTextInspector::GetNamespace(DefaultTextValue).Get(FString()));
		}

		if (!PackageNamespace.IsEmpty())
		{
			FText AutogeneratedDefaultTextValue;
			const TCHAR* Success = FTextStringHelper::ReadFromBuffer(*AutogeneratedDefaultValue, AutogeneratedDefaultTextValue);
			check(Success);

			// Conform the auto-generated default against this package ID, preserving its key to avoid determinism issues
			const FText ConformedAutogeneratedDefaultTextValue = TextNamespaceUtil::CopyTextToPackage(AutogeneratedDefaultTextValue, PackageNamespace, TextNamespaceUtil::ETextCopyMethod::PreserveKey);

			// IdenticalTo is a quick test for whether CopyTextToPackage returned the same text it was given (meaning there's nothing to update)
			if (!ConformedAutogeneratedDefaultTextValue.IdenticalTo(AutogeneratedDefaultTextValue))
			{
				// Fix-up the auto-generated default from the conformed value
				AutogeneratedDefaultValue.Reset();
				FTextStringHelper::WriteToBuffer(AutogeneratedDefaultValue, ConformedAutogeneratedDefaultTextValue);
			}
		}
	}
}

void UPDMissionGraphSchema::ResetPinToAutogeneratedDefaultValue(UEdGraphPin* Pin, bool bCallModifyCallbacks) const
{
	if (Pin->bOrphanedPin)
	{
		UEdGraphNode* Node = Pin->GetOwningNode();
		Node->PinConnectionListChanged(Pin);
	}
	else
	{
		// Autogenerated value has unreliable package namespace for text value, hack fix it up now:
		ConformAutogeneratedDefaultValuePackage(Pin->PinType, Pin->GetOwningNodeUnchecked(), Pin->AutogeneratedDefaultValue, Pin->DefaultTextValue);

		GetPinDefaultValuesFromString(Pin->PinType, Pin->GetOwningNodeUnchecked(), Pin->AutogeneratedDefaultValue, Pin->DefaultValue, Pin->DefaultObject, Pin->DefaultTextValue, false);

		if (bCallModifyCallbacks)
		{
			UEdGraphNode* Node = Pin->GetOwningNode();
			Node->PinDefaultValueChanged(Pin);
		}
	}
}

void UPDMissionGraphSchema::SetPinAutogeneratedDefaultValue(UEdGraphPin* Pin, const FString& NewValue) const
{
	Pin->AutogeneratedDefaultValue = NewValue;

	ResetPinToAutogeneratedDefaultValue(Pin, false);
}

// @todo review
// UK2Node* UPDMissionGraphSchema::ConvertDeprecatedNodeToFunctionCall(UK2Node* OldNode, UFunction* NewFunction, TMap<FName, FName>& OldPinToNewPinMap, UEdGraph* Graph) const
// {
// 	UK2Node_CallFunction* CallFunctionNode = NewObject<UK2Node_CallFunction>(Graph);
// 	check(CallFunctionNode);
// 	CallFunctionNode->SetFlags(RF_Transactional);
// 	Graph->AddNode(CallFunctionNode, false, false);
// 	CallFunctionNode->SetFromFunction(NewFunction);
// 	CallFunctionNode->CreateNewGuid();
// 	CallFunctionNode->PostPlacedNewNode();
// 	CallFunctionNode->AllocateDefaultPins();
//
// 	if (!ReplaceOldNodeWithNew(OldNode, CallFunctionNode, OldPinToNewPinMap))
// 	{
// 		// Failed, destroy node
// 		CallFunctionNode->DestroyNode();
// 		return nullptr;
// 	}
// 	return CallFunctionNode;
// }


// bool UPDMissionGraphSchema::IsAutoCreateRefTerm(const UEdGraphPin* Pin)
// {
// 	check(Pin);
//
// 	bool bIsAutoCreateRefTerm = false;
// 	if (!Pin->PinName.IsNone())
// 	{
// 		if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Pin->GetOwningNode()))
// 		{
// 			if (UFunction* TargetFunction = FuncNode->GetTargetFunction())
// 			{
// 				static TArray<FString> AutoCreateParameterNames;
// 				GetAutoEmitTermParameters(TargetFunction, AutoCreateParameterNames);
// 				bIsAutoCreateRefTerm = AutoCreateParameterNames.Contains(Pin->PinName.ToString());
// 			}
// 		}
// 	}
//
// 	return bIsAutoCreateRefTerm;
// }


FString UPDMissionGraphSchema::IsPinDefaultValid(const UEdGraphPin* Pin, const FString& NewDefaultValue, TObjectPtr<UObject> NewDefaultObject, const FText& InNewDefaultText) const
{
	check(Pin);

	FFormatNamedArguments MessageArgs;
	MessageArgs.Add(TEXT("PinName"), Pin->GetDisplayName());
	
	const bool bIsArray = Pin->PinType.IsArray();
	const bool bIsSet = Pin->PinType.IsSet();
	const bool bIsMap = Pin->PinType.IsMap();
	const bool bIsReference = Pin->PinType.bIsReference;
	const bool bIsAutoCreateRefTerm = false; // ;IsAutoCreateRefTerm(Pin);
	
	if(bIsAutoCreateRefTerm == false)
	{
		// No harm in leaving a function result node input (aka function output) unconnected - the property will be initialized correctly
		// as empty:
		UPDMissionGraphNode* Result = Cast<UPDMissionGraphNode>(Pin->GetOwningNode());
		const bool bIsFunctionOutput = Result != nullptr && ensure(Pin->Direction == EEdGraphPinDirection::EGPD_Input);
		
		if(bIsFunctionOutput == false)
		{
			if( bIsArray )
			{
				FText MsgFormat = LOCTEXT("BadArrayDefaultVal", "Array inputs (like '{PinName}') must have an input wired into them (try connecting a MakeArray node).");
				return FText::Format(MsgFormat, MessageArgs).ToString();
			}
			else if( bIsSet )
			{
				FText MsgFormat = LOCTEXT("BadSetDefaultVal", "Set inputs (like '{PinName}') must have an input wired into them (try connecting a MakeSet node).");
				return FText::Format(MsgFormat, MessageArgs).ToString();
			}
			else if ( bIsMap )
			{
				FText MsgFormat = LOCTEXT("BadMapDefaultVal", "Map inputs (like '{PinName}') must have an input wired into them (try connecting a MakeMap node).");
				return FText::Format(MsgFormat, MessageArgs).ToString();
			}
			else if( bIsReference )
			{
				FText MsgFormat = LOCTEXT("BadRefDefaultVal", "'{PinName}' in action '{ActionName}' must have an input wired into it (\"by ref\" params expect a valid input to operate on).");
				MessageArgs.Add(TEXT("ActionName"), Pin->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView));
				return FText::Format(MsgFormat, MessageArgs).ToString();
			}
		}
	}
	return FString();
}

void UPDMissionGraphSchema::SetPinAutogeneratedDefaultValueBasedOnType(UEdGraphPin* Pin) const
{
	// If you update this logic you'll probably need to update UEdGraphSchema_K2::ShouldHidePinDefaultValue!
	UScriptStruct* ColorStruct = TBaseStructure<FLinearColor>::Get();
	UScriptStruct* Vector2DStruct = TBaseStructure<FVector2D>::Get();
	UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();
	UScriptStruct* Vector3fStruct = TVariantStructure<FVector3f>::Get();
	UScriptStruct* RotatorStruct = TBaseStructure<FRotator>::Get();
	
	FString NewValue;

	// Create a useful default value based on the pin type
	if(Pin->PinType.IsContainer() )
	{
		NewValue = FString();
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
	{
		NewValue = TEXT("0");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int64)
	{
		NewValue = TEXT("0");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Byte)
	{
		UEnum* EnumPtr = Cast<UEnum>(Pin->PinType.PinSubCategoryObject.Get());
		if(EnumPtr)
		{
			// First element of enum can change. If the enum is { A, B, C } and the default value is A, 
			// the defult value should not change when enum will be changed into { N, A, B, C }
			NewValue = FString();
		}
		else
		{
			NewValue = TEXT("0");
		}
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
	{
		NewValue = TEXT("0.0");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
	{
		NewValue = TEXT("false");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		NewValue = TEXT("None");
	}
	else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
	{
		if  ((Pin->PinType.PinSubCategoryObject == VectorStruct) || (Pin->PinType.PinSubCategoryObject == Vector3fStruct) || (Pin->PinType.PinSubCategoryObject == RotatorStruct))
		{
			// This is a slightly different format than is produced by PropertyValueToString, but changing it has backward compatibility issues
			NewValue = TEXT("0, 0, 0");
		}
		else if ((Pin->PinType.PinSubCategoryObject == ColorStruct))
		{
			NewValue = TEXT("0, 0, 0, 0");
		}
		else if ((Pin->PinType.PinSubCategoryObject == Vector2DStruct))
		{
			NewValue = TEXT("0, 0");
		}
	}

	// PropertyValueToString also has cases for LinerColor and Transform, LinearColor is identical to export text so is fine, the Transform case is specially handled in the vm
	SetPinAutogeneratedDefaultValue(Pin, NewValue);
}


void UPDMissionGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// @todo Use PinCategory
	const bool bNoParent = (ContextMenuBuilder.FromPin == nullptr);
	if (bNoParent || (ContextMenuBuilder.FromPin->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath))
	{
		const FPDMissionNodeHandle DummyData; // @todo replace dummy with actual data
		AddMissionNodeOptions(TEXT("Entry Point"), ContextMenuBuilder, DummyData);
	}
	
	// @todo Use PinCategory
	if (bNoParent)
	{
		ContextMenuBuilder.AddAction(MakeShared<FMissionGraphSchemaAction_AutoArrange>(FText::GetEmpty(), LOCTEXT("AutoArrange", "Auto Arrange"), FText::GetEmpty(), 0));
		ContextMenuBuilder.AddAction(MakeShared<FMissionSchemaAction_AddComment>(LOCTEXT("AddComment", "Add new comment"), LOCTEXT("AddComment", "Add new comment")));
	}
}

const FPinConnectionResponse UPDMissionGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	if (PinA == nullptr || PinB == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinNull", "One or Both of the pins was null"));
	}

	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorSameNode", "Both are on the same node"));
	}

	// Compare the directions
	if ((PinA->Direction == EGPD_Input) && (PinB->Direction == EGPD_Input))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorInput", "Can't connect input node to input node"));
	}
	if ((PinB->Direction == EGPD_Output) && (PinA->Direction == EGPD_Output))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOutput", "Can't connect output node to output node"));
	}

	class FNodeVisitorCycleChecker
	{
	public:
		/** Check whether a loop in the graph would be caused by linking the passed-in nodes */
		bool CheckForLoop(UEdGraphNode* StartNode, UEdGraphNode* EndNode)
		{
			VisitedNodes.Add(EndNode);
			return TraverseInputNodesToRoot(StartNode);
		}

	private:
		/**
		 * Helper function for CheckForLoop()
		 * @param	Node	The node to start traversal at
		 * @return true if we reached a root node (i.e. a node with no input pins), false if we encounter a node we have already seen
		 */
		bool TraverseInputNodesToRoot(UEdGraphNode* Node)
		{
			VisitedNodes.Add(Node);

			// Follow every input pin until we cant any more ('root') or we reach a node we have seen (cycle)
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				UEdGraphPin* MyPin = Node->Pins[PinIndex];

				if (MyPin->Direction != EGPD_Input) { continue; }
				
				for (int32 LinkedPinIndex = 0; LinkedPinIndex < MyPin->LinkedTo.Num(); ++LinkedPinIndex)
				{
					UEdGraphPin* OtherPin = MyPin->LinkedTo[LinkedPinIndex];
					if (OtherPin == nullptr) { continue; }
					
					UEdGraphNode* OtherNode = OtherPin->GetOwningNode();
					return VisitedNodes.Contains(OtherNode) ? false : TraverseInputNodesToRoot(OtherNode);
				}
			}

			return true;
		}

		TSet<UEdGraphNode*> VisitedNodes;
	};

	const bool bPinALogical = PinA->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath;
	const bool bPinBLogical = PinB->PinType.PinCategory == FPDMissionGraphTypes::PinCategory_LogicalPath;
	if ((bPinALogical || bPinBLogical) && PinA->PinType.PinCategory != PinB->PinType.PinCategory)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("PinErrorOutput", "Can't connect logical path to non-logical path"));
	}

	// @todo write support for -> // return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, LOCTEXT("PinConnectReplace", "Replace connection"));
	// @todo write support for -> // return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, LOCTEXT("PinConnectReplace", "Replace connection"));
	// @todo write support for -> // return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, LOCTEXT("PinConnectReplace", "Replace connection"));
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("PinConnect", "Connect nodes"));
}

const FPinConnectionResponse UPDMissionGraphSchema::CanMergeNodes(const UEdGraphNode* NodeA, const UEdGraphNode* NodeB) const
{
	// Make sure the nodes are not the same 
	if (NodeA == NodeB)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Both are the same node"));
	}

	const bool bIsSubnode_A = Cast<UPDMissionGraphNode>(NodeA) && Cast<UPDMissionGraphNode>(NodeA)->IsSubNode();
	const bool bIsSubnode_B = Cast<UPDMissionGraphNode>(NodeB) && Cast<UPDMissionGraphNode>(NodeB)->IsSubNode();
	// const bool bIsTask_B = NodeB->IsA(UPDMissionGraphNode_Task::StaticClass());

	if (bIsSubnode_A && (bIsSubnode_B)) // || bIsTask_B))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT(""));
}

void UPDMissionGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FScopedTransaction Transaction(LOCTEXT("CreateRerouteNodeOnWire", "Create Reroute Node"));

	//@TODO: This constant is duplicated from inside of SGraphNodeKnot
	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	// Create a new knot
	UEdGraph* OwningGraph = PinA->GetOwningNode()->GetGraph();
	
	if (ensure(OwningGraph))
	{
		FGraphNodeCreator<UPDMissionGraphNode_Knot> NodeCreator(*OwningGraph);
		UPDMissionGraphNode_Knot* MyNode = NodeCreator.CreateNode();
		MyNode->NodePosX = KnotTopLeft.X;
		MyNode->NodePosY = KnotTopLeft.Y;
		//MyNode->SnapToGrid(SNAP_GRID);
	 	NodeCreator.Finalize();
	
		//UK2Node_Knot* NewKnot = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_Knot>(ParentGraph, KnotTopLeft, EK2NewNodeFlags::SelectNewNode);
	
		// Move the connections across (only notifying the knot, as the other two didn't really change)
		PinA->BreakLinkTo(PinB);
		PinA->MakeLinkTo((PinA->Direction == EGPD_Output) ? CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetInputPin() : CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetOutputPin());
		PinB->MakeLinkTo((PinB->Direction == EGPD_Output) ? CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetInputPin() : CastChecked<UPDMissionGraphNode_Knot>(MyNode)->GetOutputPin());
	}
}


bool UPDMissionGraphSchema::GetPropertyCategoryInfo(const FProperty* TestProperty, FName& OutCategory, FName& OutSubCategory, UObject*& OutSubCategoryObject, bool& bOutIsWeakPointer)
{
	if (const FStructProperty* StructProperty = CastField<const FStructProperty>(TestProperty))
	{
		OutCategory = FPDMissionGraphTypes::PinCategory_MissionRow;
		OutSubCategoryObject = StructProperty->Struct;
		// Match IsTypeCompatibleWithProperty and erase REINST_ structs here:
		if(UUserDefinedStruct* UDS = Cast<UUserDefinedStruct>(StructProperty->Struct))
		{
			UUserDefinedStruct* RealStruct = UDS->PrimaryStruct.Get();
			if(RealStruct != nullptr) { OutSubCategoryObject = RealStruct; }
		}
	}
	else if (TestProperty->IsA<FFloatProperty>())
	{
		// @todo
		// OutCategory = PC_Real;
		// OutSubCategory = PC_Float;
	}
	else if (TestProperty->IsA<FDoubleProperty>())
	{
		// @todo
		// OutCategory = PC_Real;
		// OutSubCategory = PC_Double;
	}
	else if (TestProperty->IsA<FInt64Property>())
	{
		// @todo
		// OutCategory = PC_Int64;
	}
	else if (TestProperty->IsA<FIntProperty>())
	{
		// @todo
		// OutCategory = PC_Int;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
	}
	else if (const FByteProperty* ByteProperty = CastField<const FByteProperty>(TestProperty))
	{
		// @todo
		
		// OutCategory = PC_Byte;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
		// else
		// {
		// 	OutSubCategoryObject = ByteProperty->Enum;
		// }
	}
	else if (const FEnumProperty* EnumProperty = CastField<const FEnumProperty>(TestProperty))
	{
		// @todo
		
		
		// // K2 only supports byte enums right now - any violations should have been caught by UHT or the editor
		// if (!EnumProperty->GetUnderlyingProperty()->IsA<FByteProperty>())
		// {
		// 	OutCategory = TEXT("unsupported_enum_type: enum size is larger than a byte");
		// 	return false;
		// }
		//
		// OutCategory = PC_Byte;
		//
		// if (TestProperty->HasMetaData(FBlueprintMetadata::MD_Bitmask))
		// {
		// 	OutSubCategory = PSC_Bitmask;
		// }
		// else
		// {
		// 	OutSubCategoryObject = EnumProperty->GetEnum();
		// }
	}
	else if (TestProperty->IsA<FNameProperty>())
	{
		// @todo
		// OutCategory = PC_Name;
	}
	else if (TestProperty->IsA<FStrProperty>())
	{
		// @todo
		// OutCategory = PC_String;
	}
	else if (TestProperty->IsA<FTextProperty>())
	{
		// @todo
		// OutCategory = PC_Text;
	}
	else if (const FFieldPathProperty* FieldPathProperty = CastField<const FFieldPathProperty>(TestProperty))
	{
		// @todo
		// OutCategory = PC_FieldPath;
		//OutSubCategoryObject = SoftObjectProperty->PropertyClass; @todo: FProp
	}
	else
	{
		OutCategory = TEXT("bad_type");
		return false;
	}

	return true;
}

bool UPDMissionGraphSchema::ConvertPropertyToPinType(const FProperty* Property, /*out*/ FEdGraphPinType& TypeOut) const
{
	if (Property == nullptr)
	{
		TypeOut.PinCategory = TEXT("bad_type");
		return false;
	}

	TypeOut.PinSubCategory = NAME_None;
	
	// Handle whether or not this is an array property
	const FMapProperty* MapProperty = CastField<const FMapProperty>(Property);
	const FSetProperty* SetProperty = CastField<const FSetProperty>(Property);
	const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(Property);
	const FProperty* TestProperty = Property;
	if (MapProperty)
	{
		TestProperty = MapProperty->KeyProp;

		// set up value property:
		UObject* SubCategoryObject = nullptr;
		bool bIsWeakPtr = false;
		bool bResult = const_cast<UPDMissionGraphSchema*>(this)->GetPropertyCategoryInfo(MapProperty->ValueProp, TypeOut.PinValueType.TerminalCategory,
		                                                                                 TypeOut.PinValueType.TerminalSubCategory, SubCategoryObject, bIsWeakPtr);
		TypeOut.PinValueType.TerminalSubCategoryObject = SubCategoryObject;

		if (bIsWeakPtr)
		{
			return false;
		}

		if (!bResult)
		{
			return false;
		}

		// Ensure that the value term will be identified as a wrapper type if the source property has that flag set.
		if(MapProperty->ValueProp->HasAllPropertyFlags(CPF_UObjectWrapper))
		{
			TypeOut.PinValueType.bTerminalIsUObjectWrapper = true;
		}
	}
	else if (SetProperty)
	{
		TestProperty = SetProperty->ElementProp;
	}
	else if (ArrayProperty)
	{
		TestProperty = ArrayProperty->Inner;
	}
	TypeOut.ContainerType = FEdGraphPinType::ToPinContainerType(ArrayProperty != nullptr, SetProperty != nullptr, MapProperty != nullptr);
	TypeOut.bIsReference = Property->HasAllPropertyFlags(CPF_OutParm|CPF_ReferenceParm);
	TypeOut.bIsConst     = Property->HasAllPropertyFlags(CPF_ConstParm);

	// This flag will be set on the key/inner property for container types, so check the test property.
	TypeOut.bIsUObjectWrapper = TestProperty->HasAllPropertyFlags(CPF_UObjectWrapper);

	// @todo
	// // Check to see if this is the wildcard property for the target container type
	//
	// if(IsWildcardProperty(Property))
	// {
	// 	TypeOut.PinCategory = PC_Wildcard;
	// 	if(MapProperty)
	// 	{
	// 		TypeOut.PinValueType.TerminalCategory = PC_Wildcard;
	// 	}
	// }
	// else
	// {
	// 	UObject* SubCategoryObject = nullptr;
	// 	bool bIsWeakPointer = false;
	// 	bool bResult = GetPropertyCategoryInfo(TestProperty, TypeOut.PinCategory, TypeOut.PinSubCategory, SubCategoryObject, bIsWeakPointer);
	// 	TypeOut.bIsWeakPointer = bIsWeakPointer;
	// 	TypeOut.PinSubCategoryObject = SubCategoryObject;
	// 	if (!bResult)
	// 	{
	// 		return false;
	// 	}
	// }
	//
	// if (TypeOut.PinSubCategory == PSC_Bitmask)
	// {
	// 	const FString& BitmaskEnumName = TestProperty->GetMetaData(TEXT("BitmaskEnum"));
	// 	if(!BitmaskEnumName.IsEmpty())
	// 	{
	// 		// @TODO: Potentially replace this with a serialized UEnum reference on the FProperty (e.g. FByteProperty::Enum)
	// 		TypeOut.PinSubCategoryObject = UClass::TryFindTypeSlow<UEnum>(BitmaskEnumName);
	// 	}
	// }

	return true;
}



bool UPDMissionGraphSchema::IsCacheVisualizationOutOfDate(int32 InVisualizationCacheID) const
{
	return CurrentCacheRefreshID != InVisualizationCacheID;
}

int32 UPDMissionGraphSchema::GetCurrentVisualizationCacheID() const
{
	return CurrentCacheRefreshID;
}

void UPDMissionGraphSchema::ForceVisualizationCacheClear() const
{
	++CurrentCacheRefreshID;
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