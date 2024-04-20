﻿/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "MissionGraph/Slate/PDPinManager.h"

#include "ObjectEditorUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/UserDefinedStruct.h"
#include "Kismet/KismetMathLibrary.h"
#include "MissionGraph/PDMissionGraphSchema.h"
#include "UObject/ObjectSaveContext.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"


#define LOCTEXT_NAMESPACE "FMissionTreeNode"



UPDMissionNode_MakeStruct::FMakeStructPinManager::FMakeStructPinManager(const uint8* InSampleStructMemory)
	: FPDOptionalPinManager()
	, SampleStructMemory(InSampleStructMemory)
	, bHasAdvancedPins(false)
{
}

UPDMissionNode_MakeStruct::UPDMissionNode_MakeStruct(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bMadeAfterOverridePinRemoval(false)
{
}


bool PropertyValueToString_Direct(const FProperty* Property, const uint8* DirectValue, FString& OutForm, UObject* OwningObject, int32 PortFlags)
{
	check(Property && DirectValue);
	OutForm.Reset();

	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	if (StructProperty)
	{
		static UScriptStruct* VectorStruct = TBaseStructure<FVector>::Get();
		static UScriptStruct* RotatorStruct = TBaseStructure<FRotator>::Get();
		static UScriptStruct* TransformStruct = TBaseStructure<FTransform>::Get();
		static UScriptStruct* LinearColorStruct = TBaseStructure<FLinearColor>::Get();

		// Struct properties must be handled differently, unfortunately.  We only support FVector, FRotator, and FTransform
		if (StructProperty->Struct == VectorStruct)
		{
			FVector Vector;
			Property->CopySingleValue(&Vector, DirectValue);
			OutForm = FString::Printf(TEXT("%f,%f,%f"), Vector.X, Vector.Y, Vector.Z);
		}
		else if (StructProperty->Struct == RotatorStruct)
		{
			FRotator Rotator;
			Property->CopySingleValue(&Rotator, DirectValue);
			OutForm = FString::Printf(TEXT("%f,%f,%f"), Rotator.Pitch, Rotator.Yaw, Rotator.Roll);
		}
		else if (StructProperty->Struct == TransformStruct)
		{
			FTransform Transform;
			Property->CopySingleValue(&Transform, DirectValue);
			OutForm = Transform.ToString();
		}
		else if (StructProperty->Struct == LinearColorStruct)
		{
			FLinearColor Color;
			Property->CopySingleValue(&Color, DirectValue);
			OutForm = Color.ToString();
		}
	}

	bool bSucceeded = true;
	if (OutForm.IsEmpty())
	{
		const uint8* DefaultValue = DirectValue;	
		bSucceeded = Property->ExportText_Direct(OutForm, DirectValue, DefaultValue, OwningObject, PPF_SerializedAsImportText | PortFlags);
	}
	return bSucceeded;
}

bool PropertyValueToString(const FProperty* Property, const uint8* Container, FString& OutForm, UObject* OwningObject = nullptr, int32 PortFlags = PPF_None)
{
	return PropertyValueToString_Direct(Property, Property->ContainerPtrToValuePtr<const uint8>(Container), OutForm, OwningObject, PortFlags);
}

void UPDMissionNode_MakeStruct::FMakeStructPinManager::GetRecordDefaults(FProperty* TestProperty, FOptionalPinFromProperty& Record) const
{
	FPDOptionalPinManager::GetRecordDefaults(TestProperty, Record);
}

void UPDMissionNode_MakeStruct::FMakeStructPinManager::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, FProperty* Property) const
{
	FPDOptionalPinManager::CustomizePinData(Pin, SourcePropertyName, ArrayIndex, Property);
	if (Pin && Property)
	{
		const UPDMissionGraphSchema* Schema = GetDefault<UPDMissionGraphSchema>();
		check(Schema);

		// Should pin default value be filled as FText?
		const bool bIsText = Property->IsA<FTextProperty>();
		checkSlow(bIsText == ((UEdGraphSchema_K2::PC_Text == Pin->PinType.PinCategory) && !Pin->PinType.IsContainer()));

		const bool bIsObject = Property->IsA<FObjectPropertyBase>();
		checkSlow(bIsObject == ((UEdGraphSchema_K2::PC_Object == Pin->PinType.PinCategory || UEdGraphSchema_K2::PC_Class == Pin->PinType.PinCategory || 
			UEdGraphSchema_K2::PC_SoftObject == Pin->PinType.PinCategory || UEdGraphSchema_K2::PC_SoftClass == Pin->PinType.PinCategory) && !Pin->PinType.IsContainer()));

		if (Property->HasAnyPropertyFlags(CPF_AdvancedDisplay))
		{
			Pin->bAdvancedView = true;
			bHasAdvancedPins = true;
		}

		// @todo fix missing members
		// const FString& MetadataDefaultValue = Property->GetMetaData(TEXT("MakeStructureDefaultValue"));
		// if (!MetadataDefaultValue.IsEmpty())
		// {
		// 	Schema->SetPinAutogeneratedDefaultValue(Pin, MetadataDefaultValue);
		// 	return;
		// }
		//
		// if (nullptr != SampleStructMemory)
		// {
		// 	FString NewDefaultValue;
		// 	if (PropertyValueToString(Property, SampleStructMemory, NewDefaultValue))
		// 	{
		// 		if (Schema->IsPinDefaultValid(Pin, NewDefaultValue, nullptr, FText::GetEmpty()).IsEmpty())
		// 		{
		// 			Schema->SetPinAutogeneratedDefaultValue(Pin, NewDefaultValue);
		// 			return;
		// 		}
		// 	}
		// }
		//
		// Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
	}
}

bool UPDMissionNode_MakeStruct::FMakeStructPinManager::CanTreatPropertyAsOptional(FProperty* TestProperty) const
{
	return FPDOptionalPinManager::CanTreatPropertyAsOptional(TestProperty);
}


void UPDMissionNode_MakeStruct::AllocateDefaultPins()
{
	if (StructType)
	{
		PreloadObject(StructType);
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, StructType, StructType->GetFName());

		// @todo fix missing members
		//bool bHasAdvancedPins = false;
		//{
		//	FStructOnScope StructOnScope(StructType);
		//	FMakeStructPinManager OptionalPinManager(StructOnScope.GetStructMemory(), GetBlueprint());
		//	OptionalPinManager.RebuildPropertyList(ShowPinForProperties, StructType);
		//	OptionalPinManager.CreateVisiblePins(ShowPinForProperties, StructType, EGPD_Input, this);
		//
		//	bHasAdvancedPins = OptionalPinManager.HasAdvancedPins();
		//}

		// Set container pin types to have their default values ignored, which will in turn
		// enable auto generation for any that are not set by the user. 
		for(UEdGraphPin* Pin : Pins)
		{
			Pin->bDefaultValueIsIgnored = Pin->bDefaultValueIsIgnored || Pin->PinType.IsContainer();
		}

		// @todo fix missing members		
		// // When struct has a lot of fields, mark their pins as advanced
		// if(!bHasAdvancedPins && Pins.Num() > 5)
		// {
		// 	for(int32 PinIndex = 3; PinIndex < Pins.Num(); ++PinIndex)
		// 	{
		// 		if(UEdGraphPin * EdGraphPin = Pins[PinIndex])
		// 		{
		// 			EdGraphPin->bAdvancedView = true;
		// 			bHasAdvancedPins = true;
		// 		}
		// 	}
		// }
		//
		// if (bHasAdvancedPins && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
		// {
		// 	AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
		// }
	}
}

void UPDMissionNode_MakeStruct::PreloadRequiredAssets()
{
	PreloadObject(StructType);
}

void UPDMissionNode_MakeStruct::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if(!StructType)
	{
		MessageLog.Error(*LOCTEXT("NoStruct_Error", "No Struct in @@").ToString(), this);
	}
	else
	{

		// @todo
		// UBlueprint* BP = GetBlueprint();
		// for (TFieldIterator<FProperty> It(StructType); It; ++It)
		// {
		// 	const FProperty* Property = *It;
		// 	if (CanBeExposed(Property, BP))
		// 	{
		// 		if (Property->ArrayDim > 1)
		// 		{
		// 			const UEdGraphPin* Pin = FindPin(Property->GetFName());
		// 			MessageLog.Warning(*LOCTEXT("StaticArray_Warning", "@@ - the native property is a static array, which is not supported by blueprints").ToString(), Pin);
		// 		}
		// 	}
		// }
		//
		// if (!bMadeAfterOverridePinRemoval)
		// {
		// 	MessageLog.Note(*NSLOCTEXT("K2Node", "OverridePinRemoval_SetFieldsInStruct", "Override pins have been removed from @@ in @@, it functions the same as it did but some functionality may be deprecated! This note will go away after you resave the asset!").ToString(), this, GetBlueprint());
		// }
	}
}


FText UPDMissionNode_MakeStruct::GetNodeTitle(ENodeTitleType::Type TitleType) const 
{
	if (StructType == nullptr)
	{
		return LOCTEXT("MakeNullStructTitle", "Make <unknown struct>");
	}
	else if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("StructName"), FText::FromName(StructType->GetFName()));
		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("MakeNodeTitle", "Make {StructName}"), Args), this);
	}
	return CachedNodeTitle;
}

FText UPDMissionNode_MakeStruct::GetTooltipText() const
{
	if (StructType == nullptr)
	{
		return LOCTEXT("MakeNullStruct_Tooltip", "Adds a node that create an '<unknown struct>' from its members");
	}
	else if (CachedTooltip.IsOutOfDate(this))
	{
		// FText::Format() is slow, so we cache this to save on performance
		CachedTooltip.SetCachedText(FText::Format(
			LOCTEXT("MakeStruct_Tooltip", "Adds a node that create a '{0}' from its members"),
			FText::FromName(StructType->GetFName())
		), this);
	}
	return CachedTooltip;
}

FSlateIcon UPDMissionNode_MakeStruct::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.MakeStruct_16x");
	return Icon;
}

FLinearColor UPDMissionNode_MakeStruct::GetNodeTitleColor() const
{
	if(const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>())
	{
		FEdGraphPinType PinType;
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = StructType;
		return K2Schema->GetPinTypeColor(PinType);
	}
	return UPDMissionGraphNode::GetNodeTitleColor();
}

bool UPDMissionNode_MakeStruct::CanBeMade(const UScriptStruct* Struct, const bool bForInternalUse)
{
	return (Struct && !Struct->HasMetaData(FBlueprintMetadata::MD_NativeMakeFunction) && UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Struct, bForInternalUse));
}

bool UPDMissionNode_MakeStruct::CanBeSplit(const UScriptStruct* Struct, UBlueprint* InBP)
{
	return true;
	
	// @todo fix missing members
	// if (CanBeMade(Struct))
	// {
	// 	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	// 	{
	// 		if (CanBeExposed(*It, InBP))
	// 		{
	// 			return true;
	// 		}
	// 	}
	// }
	// return false;
}

FNodeHandlingFunctor* UPDMissionNode_MakeStruct::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	// @todo fix missing members
	return nullptr; // new FKCHandler_MakeStruct(CompilerContext);
}

EPDRedirectType UPDMissionNode_MakeStruct::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	return EPDRedirectType::ERedirectType_None;
	
	// @todo fix missing members
	// EPDRedirectType Result = UPDMissionGraphNode::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	// if ((ERedirectType_None == Result) && DoRenamedPinsMatch(NewPin, OldPin, false))
	// {
	// 	Result = ERedirectType_Name;
	// }
	// return Result;
}

void UPDMissionNode_MakeStruct::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	// @todo fix missing members
	// Super::SetupMenuActions(ActionRegistrar, FMakeStructSpawnerAllowedDelegate::CreateStatic(&UPDMissionNode_MakeStruct::CanBeMade), EGPD_Input);
}

FText UPDMissionNode_MakeStruct::GetMenuCategory() const
{
	// @todo fix missing members
	return FText(); // FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Struct);
}

void UPDMissionNode_MakeStruct::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

	// @todo fix missing members
	// UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
	// if (Blueprint && !Blueprint->bBeingCompiled)
	// {
	// 	bMadeAfterOverridePinRemoval = true;
	// }
}

void UPDMissionNode_MakeStruct::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	// New nodes automatically have this set.
	bMadeAfterOverridePinRemoval = true;
}

void UPDMissionNode_MakeStruct::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && !Ar.IsTransacting() && !HasAllFlags(RF_Transient))
	{
		// @todo fix missing members
		// UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForNode(this);
		// if (Blueprint && !bMadeAfterOverridePinRemoval)
		// {
		// 	// Check if this node actually requires warning the user that functionality has changed.
		// 	bMadeAfterOverridePinRemoval = true;
		// 	if (StructType != nullptr)
		// 	{
		// 		FOptionalPinManager PinManager;
		//
		// 		// Have to check if this node is even in danger.
		// 		for (FOptionalPinFromProperty& PropertyEntry : ShowPinForProperties)
		// 		{
		// 			FProperty* Property = StructType->FindPropertyByName(PropertyEntry.PropertyName);
		// 			bool bNegate = false;
		// 			if (FProperty* OverrideProperty = PropertyCustomizationHelpers::GetEditConditionProperty(Property, bNegate))
		// 			{
		// 				bool bHadOverridePropertySeparation = false;
		// 				for (FOptionalPinFromProperty& OverridePropertyEntry : ShowPinForProperties)
		// 				{
		// 					if (OverridePropertyEntry.PropertyName == OverrideProperty->GetFName())
		// 					{
		// 						bHadOverridePropertySeparation = true;
		// 						break;
		// 					}
		// 				}
		//
		// 				bMadeAfterOverridePinRemoval = false;
		// 				UEdGraphPin* Pin = FindPin(Property->GetFName());
		//
		// 				if (bHadOverridePropertySeparation)
		// 				{
		// 					UEdGraphPin* OverridePin = FindPin(OverrideProperty->GetFName());
		// 					if (OverridePin)
		// 					{
		// 						// Override pins are always booleans
		// 						check(OverridePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean);
		// 						// If the old override pin's default value was true, then the override should be marked as enabled
		// 						PropertyEntry.bIsOverrideEnabled = OverridePin->DefaultValue.ToBool();
		// 						// It had an override pin, so conceptually the override pin is visible
		// 						PropertyEntry.bIsOverridePinVisible = true;
		// 						// Because there was an override pin visible for this property, this property will be forced to have a pin
		// 						PropertyEntry.bShowPin = true;
		// 					}
		// 					else
		// 					{
		// 						// No override pin, ensure all override bools are false
		// 						PropertyEntry.bIsOverrideEnabled = false;
		// 						PropertyEntry.bIsOverridePinVisible = false;
		// 					}
		// 				}
		// 				else
		// 				{
		// 					if (Pin)
		// 					{
		// 						PropertyEntry.bIsOverrideEnabled = true;
		// 						PropertyEntry.bIsOverridePinVisible = true;
		// 					}
		// 				}
		//
		// 				// If the pin for this property, which sets the property's value, does not exist then the user was not trying to set the value
		// 				PropertyEntry.bIsSetValuePinVisible = Pin != nullptr;
		// 			}
		// 		}
		// 	}
		// }
	}
}

void UPDMissionNode_MakeStruct::ConvertDeprecatedNode(UEdGraph* Graph, bool bOnlySafeChanges)
{
	const UPDMissionGraphSchema* Schema = GetDefault<UPDMissionGraphSchema>();

	// User may have since deleted the struct type
	if (StructType == nullptr)
	{
		return;
	}

	// Check to see if the struct has a native make/break that we should try to convert to.
	if (StructType->HasMetaData(FBlueprintMetadata::MD_NativeMakeFunction))
	{
		UFunction* MakeNodeFunction = nullptr;

		// If any pins need to change their names during the conversion, add them to the map.
		TMap<FName, FName> OldPinToNewPinMap;
		if (StructType == TBaseStructure<FRotator>::Get())
		{
			MakeNodeFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, MakeRotator));
			OldPinToNewPinMap.Add(TEXT("Rotator"), UEdGraphSchema_K2::PN_ReturnValue);
		}
		else if (StructType == TBaseStructure<FVector>::Get())
		{
			MakeNodeFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED_ThreeParams(UKismetMathLibrary, MakeVector, double, double, double));
			OldPinToNewPinMap.Add(TEXT("Vector"), UEdGraphSchema_K2::PN_ReturnValue);
		}
		else if (StructType == TBaseStructure<FVector2D>::Get())
		{
			MakeNodeFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED_TwoParams(UKismetMathLibrary, MakeVector2D, double, double));
			OldPinToNewPinMap.Add(TEXT("Vector2D"), UEdGraphSchema_K2::PN_ReturnValue);
		}
		else
		{
			// StructType->
			const FString& MetaData = StructType->GetMetaData(FBlueprintMetadata::MD_NativeMakeFunction);
			MakeNodeFunction = FindObject<UFunction>(nullptr, *MetaData, true);

			if (MakeNodeFunction)
			{
				OldPinToNewPinMap.Add(*StructType->GetName(), UEdGraphSchema_K2::PN_ReturnValue);
			}
		}

		// @todo fix missing members
		// if (MakeNodeFunction)
		// {
		// 	Schema->ConvertDeprecatedNodeToFunctionCall(this, MakeNodeFunction, OldPinToNewPinMap, Graph);
		// }
	}
}



void FPDOptionalPinManager::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, FProperty* Property) const
{
	FOptionalPinManager::CustomizePinData(Pin, SourcePropertyName, ArrayIndex, Property);

	if (Pin == nullptr || Property == nullptr) { return; }
	
	const UUserDefinedStruct* UDStructure = Cast<const UUserDefinedStruct>(Property->GetOwnerStruct());
	if (UDStructure == nullptr || UDStructure->EditorData == nullptr) { return; }
		
	const FStructVariableDescription* VarDesc =
		FStructureEditorUtils::GetVarDesc(UDStructure)
		.FindByPredicate(FStructureEditorUtils::FFindByNameHelper<FStructVariableDescription>(Property->GetFName()));
	if (VarDesc == nullptr) { return; }
	
	Pin->PersistentGuid = VarDesc->VarGuid;
}


void FPDOptionalPinManager::GetRecordDefaults(FProperty* TestProperty, FOptionalPinFromProperty& Record) const
{
	FOptionalPinManager::GetRecordDefaults(TestProperty,Record);
}

bool FPDOptionalPinManager::CanTreatPropertyAsOptional(FProperty* TestProperty) const
{
	return FOptionalPinManager::CanTreatPropertyAsOptional(TestProperty);
}

void FPDOptionalPinManager::RebuildPropertyList(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct)
{
	FOptionalPinManager::RebuildPropertyList(Properties, SourceStruct);
}

void FPDOptionalPinManager::RebuildProperty(FProperty* TestProperty, FName CategoryName, TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, TMap<FName, FOldOptionalPinSettings>& OldSettings)
{
	FOptionalPinManager::RebuildProperty(TestProperty, CategoryName, Properties, SourceStruct, OldSettings);
}

void FPDOptionalPinManager::CreateVisiblePins(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, EEdGraphPinDirection Direction, UPDMissionGraphNode* TargetNode, uint8* StructBasePtr, uint8* DefaultsPtr)
{
	const UPDMissionGraphSchema* Schema = GetDefault<UPDMissionGraphSchema>();

	for (FOptionalPinFromProperty& PropertyEntry : Properties)
	{
		FProperty* OuterProperty = FindFieldChecked<FProperty>(SourceStruct, PropertyEntry.PropertyName);
		if (OuterProperty == nullptr) { continue; }
		
		// Do we treat an array property as one pin, or a pin per entry in the array?
		// Depends on if we have an instance of the struct to work with.
		FArrayProperty* ArrayProperty = CastField<FArrayProperty>(OuterProperty);
		if ((ArrayProperty != nullptr) && (StructBasePtr != nullptr))
		{
			FProperty* InnerProperty = ArrayProperty->Inner;
			FEdGraphPinType PinType;
			if (Schema->ConvertPropertyToPinType(InnerProperty, /*out*/ PinType) == false)
			{
				continue;
			}
			
			FScriptArrayHelper_InContainer ArrayHelper(ArrayProperty, StructBasePtr);
			for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
			{
				// Create the pin
				UEdGraphPin* NewPin = NULL;
				if (PropertyEntry.bShowPin)
				{
					const FName PinName = *FString::Printf(TEXT("%s_%d"), *PropertyEntry.PropertyName.ToString(), Index);
					FFormatNamedArguments Args;
					Args.Add(TEXT("PinName"), FText::FromString(PropertyEntry.PropertyFriendlyName.IsEmpty() ? PinName.ToString() : PropertyEntry.PropertyFriendlyName));
					Args.Add(TEXT("Index"), Index);
					const FText PinFriendlyName = FText::Format(LOCTEXT("PinFriendlyNameWithIndex", "{PinName} {Index}"), Args);
					NewPin = TargetNode->CreatePin(Direction, PinType, PinName);
					NewPin->PinFriendlyName = PinFriendlyName;
					NewPin->bNotConnectable = !PropertyEntry.bIsSetValuePinVisible;
					NewPin->bDefaultValueIsIgnored = !PropertyEntry.bIsSetValuePinVisible;
					Schema->ConstructBasicPinTooltip(*NewPin, PropertyEntry.PropertyTooltip, NewPin->PinToolTip);
					// Allow the derived class to customize the created pin
					CustomizePinData(NewPin, PropertyEntry.PropertyName, Index, InnerProperty);
				}
				
				// Let derived classes take a crack at transferring default values
				uint8* ValuePtr = ArrayHelper.GetRawPtr(Index);
				uint8* DefaultValuePtr = nullptr;
				FScriptArrayHelper_InContainer DefaultsArrayHelper(ArrayProperty, DefaultsPtr);
				if (DefaultsPtr && DefaultsArrayHelper.IsValidIndex(Index))
				{
					DefaultValuePtr = DefaultsArrayHelper.GetRawPtr(Index);
				}
				
				NewPin != nullptr ?
					PostInitNewPin(NewPin, PropertyEntry, Index, ArrayProperty->Inner, ValuePtr, DefaultValuePtr)
					: PostRemovedOldPin(PropertyEntry, Index, ArrayProperty->Inner, ValuePtr, DefaultValuePtr);
			}
			
		}
		else
		{
			// Not an array property
			FEdGraphPinType PinType;
			if (Schema->ConvertPropertyToPinType(OuterProperty, /*out*/ PinType) == false)
			{
				continue;
			}
			
			// Create the pin
			UEdGraphPin* NewPin = nullptr;
			if (PropertyEntry.bShowPin)
			{
				const FName PinName = PropertyEntry.PropertyName;
				NewPin = TargetNode->CreatePin(Direction, PinType, PinName);
				NewPin->PinFriendlyName = FText::FromString(PropertyEntry.PropertyFriendlyName.IsEmpty() ? PinName.ToString() : PropertyEntry.PropertyFriendlyName);
				NewPin->bNotConnectable = !PropertyEntry.bIsSetValuePinVisible;
				NewPin->bDefaultValueIsIgnored = !PropertyEntry.bIsSetValuePinVisible;
				Schema->ConstructBasicPinTooltip(*NewPin, PropertyEntry.PropertyTooltip, NewPin->PinToolTip);
				// Allow the derived class to customize the created pin
				CustomizePinData(NewPin, PropertyEntry.PropertyName, INDEX_NONE, OuterProperty);
			}
			
			// Let derived classes take a crack at transferring default values
			if (StructBasePtr == nullptr) { continue; }
			
			uint8* ValuePtr = OuterProperty->ContainerPtrToValuePtr<uint8>(StructBasePtr);
			uint8* DefaultValuePtr = DefaultsPtr ? OuterProperty->ContainerPtrToValuePtr<uint8>(DefaultsPtr) : nullptr;
			NewPin != nullptr ?
				PostInitNewPin(NewPin, PropertyEntry, INDEX_NONE, OuterProperty, ValuePtr, DefaultValuePtr)
				: PostRemovedOldPin(PropertyEntry, INDEX_NONE, OuterProperty, ValuePtr, DefaultValuePtr);
		}
	}
}

void FPDOptionalPinManager::CacheShownPins(const TArray<FOptionalPinFromProperty>& OptionalPins, TArray<FName>& OldShownPins)
{
	FOptionalPinManager::CacheShownPins(OptionalPins, OldShownPins);
}

void FPDOptionalPinManager::EvaluateOldShownPins(const TArray<FOptionalPinFromProperty>& OptionalPins, TArray<FName>& OldShownPins, UPDMissionGraphNode* Node)
{
	for (const FOptionalPinFromProperty& ShowPinForProperty : OptionalPins)
	{
		if (ShowPinForProperty.bShowPin || OldShownPins.Contains(ShowPinForProperty.PropertyName) == false)
		{
			continue;
		}

		UEdGraphPin* Pin = Node->FindPin(ShowPinForProperty.PropertyName);
		if (Pin == nullptr) { continue; }
		
		Pin->SetSavePinIfOrphaned(false);
	}
	OldShownPins.Reset();
}



#undef LOCTEXT_NAMESPACE // "FMissionTreeNode"


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