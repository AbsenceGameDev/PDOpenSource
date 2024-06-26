﻿/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#include "Mission/Graph/PDPinManager.h"
#include "Mission/Graph/PDMissionGraphSchema.h"
#include "Mission/Graph/PDMissionGraphNodeBase.h"

#define LOCTEXT_NAMESPACE "FMissionTreeNode"

FPDOptionalPinManager::FPDOptionalPinManager(const uint8* InSampleStructMemory)
	: SampleStructMemory(InSampleStructMemory) {}

bool PropertyValueToString_Direct(const FProperty* Property, const uint8* DirectValue, FString& OutForm, UObject* OwningObject, int32 PortFlags)
{
	check(Property && DirectValue);
	OutForm.Reset();

	// @todo review if I need some custom text format or if ExportText_Direct will work for all our mission user-datatypes

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


void FPDOptionalPinManager::CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, FProperty& Property, UPDMissionGraphNode* OwnerNode, EPDPinCustomizer CurrentPinHandling) const
{
	const UPDMissionGraphSchema* Schema = GetDefault<UPDMissionGraphSchema>();
	check(Schema);

	bool bSkipRescursion = false;
	bool bSkipCurrentPin = false;
	switch (CurrentPinHandling)
	{
	case EPDPinCustomizer::STOPDEPTHLIM:
		bSkipRescursion = true;
	case EPDPinCustomizer::CONTINUE:
		{ // Scoped
			if (Pin == nullptr) { return; }
		
			Pin->PersistentGuid = FGuid::NewGuid();
			
			if (Property.HasAnyPropertyFlags(CPF_AdvancedDisplay))
			{
				Pin->bAdvancedView = true;
			}
				
			const FString& MetadataDefaultValue = Property.GetMetaData(TEXT("MakeStructureDefaultValue"));
			if (MetadataDefaultValue.IsEmpty() == false)
			{
				Schema->SetPinAutogeneratedDefaultValue(Pin, MetadataDefaultValue);
				return;
			}
		
			if (SampleStructMemory == nullptr)
			{
				Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
				return;
			}
		}
	default: bSkipCurrentPin = true; break;
	}
	
	FString NewDefaultValue;
	if (PropertyValueToString(&Property, SampleStructMemory, NewDefaultValue) &&
		(bSkipCurrentPin || (bSkipCurrentPin == false && Schema->IsPinDefaultValid(Pin, NewDefaultValue, nullptr, FText::GetEmpty()).IsEmpty())))
	{
		UE_LOG(LogTemp, Warning, TEXT("NewDefaultValue: %s"), *NewDefaultValue)
		
		// @todo Rewrite slightly so it does not expand past the mission datatypes, this currently also expands the gameplay tag containers and such within into their own field
		// @todo Cont: Make sure to create more pincategories to cover all the base datatypes and apply them on
		// @todo Cont: the pin factory for custom input fields to allow to manipulate the referenced table-row directly on the node 
		FStructProperty* StructProperty = CastField<FStructProperty>(&Property);
		TObjectPtr<UScriptStruct> InnerStruct = StructProperty != nullptr ? StructProperty->Struct : nullptr;
		if (InnerStruct != nullptr && bSkipRescursion == false) 
		{
			FStructOnScope StructOnScope(InnerStruct);
			FPDOptionalPinManager NestedPinManager(StructOnScope.GetStructMemory());
			
			TArray<FOptionalPinFromProperty> ShowInnerPinForProperties;
			NestedPinManager.RebuildPropertyList(ShowInnerPinForProperties, InnerStruct);
			NestedPinManager.CreateVisiblePins(ShowInnerPinForProperties, InnerStruct, EGPD_Input, OwnerNode);
		}
		else
		{
			Schema->SetPinAutogeneratedDefaultValue(Pin, NewDefaultValue);
		}
		
		return;
	}
		
	Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
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

void FPDOptionalPinManager::CreateVisiblePins(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, EEdGraphPinDirection Direction, UPDMissionGraphNode* TargetNode)
{
	const UPDMissionGraphSchema* Schema = GetDefault<UPDMissionGraphSchema>();
	
	for (FOptionalPinFromProperty& PropertyEntry : Properties)
	{
		FProperty* OuterProperty = FindFieldChecked<FProperty>(SourceStruct, PropertyEntry.PropertyName);
		if (OuterProperty == nullptr) { continue; }
		
		// Not an array property
		FEdGraphPinType ReturnPinType;
		if (Schema->ConvertPropertyToPinType(OuterProperty,ReturnPinType) == false) { continue; }
		
		// Create the pin
		if (PropertyEntry.bShowPin == false) { continue; }

		// ignore but step past these:
		static UScriptStruct* MissionBaseStruct = TBaseStructure<FPDMissionBase>::Get();
		static UScriptStruct* TickBehaviourStruct = TBaseStructure<FPDMissionTickBehaviour>::Get();
		static UScriptStruct* MissionRulesStruct = TBaseStructure<FPDMissionRules>::Get();
		static UScriptStruct* MissionTagCompoundStruct = TBaseStructure<FPDMissionTagCompound>::Get();
		static UScriptStruct* MissionMetadataStruct = TBaseStructure<FPDMissionMetadata>::Get();
		// ignore and don't step further:
		static UScriptStruct* TagMetadataStruct = TBaseStructure<FGameplayTag>::Get();

		// Filtering out the top-level datatypes and the low level datataype 
		EPDPinCustomizer CreateTopLevelPin = EPDPinCustomizer::CONTINUE;
		const FStructProperty* StructProperty = CastField<FStructProperty>(OuterProperty);
		if (StructProperty != nullptr)
		{
			CreateTopLevelPin =
			StructProperty->Struct == MissionBaseStruct
			|| StructProperty->Struct == TickBehaviourStruct
			|| StructProperty->Struct == MissionRulesStruct
			|| StructProperty->Struct == MissionTagCompoundStruct
			|| StructProperty->Struct == MissionMetadataStruct
			? EPDPinCustomizer::SKIPPAST
			: StructProperty->Struct == TagMetadataStruct
			? EPDPinCustomizer::STOPDEPTHLIM
			: EPDPinCustomizer::CONTINUE;
		}

		UEdGraphPin* NewPin = nullptr;
		switch (CreateTopLevelPin)
		{
		case EPDPinCustomizer::CONTINUE:
		case EPDPinCustomizer::STOPDEPTHLIM:
			{ // Scoped
				const FName PinName = PropertyEntry.PropertyName;
				NewPin = TargetNode->CreatePin(Direction, ReturnPinType, PinName);
				NewPin->PinFriendlyName = FText::FromString(PropertyEntry.PropertyFriendlyName.IsEmpty() ? PinName.ToString() : PropertyEntry.PropertyFriendlyName);
				NewPin->bNotConnectable = !PropertyEntry.bIsSetValuePinVisible;
				NewPin->bDefaultValueIsIgnored = !PropertyEntry.bIsSetValuePinVisible;
				Schema->ConstructBasicPinTooltip(*NewPin, PropertyEntry.PropertyTooltip, NewPin->PinToolTip);
			}
		default: break;
		}

		// This version of CustomizePinData steps into the mission row and sets up the inner properties as pins 
		CustomizePinData(NewPin, PropertyEntry.PropertyName, INDEX_NONE, *OuterProperty, TargetNode, CreateTopLevelPin);
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