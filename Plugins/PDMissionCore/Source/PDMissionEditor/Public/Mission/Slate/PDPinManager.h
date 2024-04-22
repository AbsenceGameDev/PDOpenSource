/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include <CoreMinimal.h>


#include "Mission/Graph/PDMissionGraphNode.h"
#include "PDPinManager.generated.h"

// Manager to build or refresh a list of optional pins
struct PDMISSIONEDITOR_API FPDOptionalPinManager : public FOptionalPinManager
{
public:
	virtual ~FPDOptionalPinManager() { }

	
	// Customize automatically created pins if desired
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, FProperty* Property) const override;
	
	/** Should the specified property be displayed by default */
	virtual void GetRecordDefaults(FProperty* TestProperty, FOptionalPinFromProperty& Record) const;

	/** Can this property be managed as an optional pin (with the ability to be shown or hidden) */
	virtual bool CanTreatPropertyAsOptional(FProperty* TestProperty) const;

	// hide and call base in funciton, as we can't change these declaration in 'FOptionalPinManager' to be virtual
	void RebuildPropertyList(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct);
	void CreateVisiblePins(TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, EEdGraphPinDirection Direction, class UPDMissionGraphNode* TargetNode, uint8* StructBasePtr = nullptr, uint8* DefaultsPtr = nullptr);
	
	/** Helper function to make consistent behavior between nodes that use optional pins */
	static void CacheShownPins(const TArray<FOptionalPinFromProperty>& OptionalPins, TArray<FName>& OldShownPins);

	/** Helper function to make consistent behavior between nodes that use optional pins */
	static void EvaluateOldShownPins(const TArray<FOptionalPinFromProperty>& OptionalPins, TArray<FName>& OldShownPins, UPDMissionGraphNode* Node);


protected:
	virtual void PostInitNewPin(UEdGraphPin* Pin, FOptionalPinFromProperty& Record, int32 ArrayIndex, FProperty* Property, uint8* PropertyAddress, uint8* DefaultPropertyAddress) const {}
	virtual void PostRemovedOldPin(FOptionalPinFromProperty& Record, int32 ArrayIndex, FProperty* Property, uint8* PropertyAddress, uint8* DefaultPropertyAddress) const {}
	void RebuildProperty(FProperty* TestProperty, FName CategoryName, TArray<FOptionalPinFromProperty>& Properties, UStruct* SourceStruct, TMap<FName, FOldOptionalPinSettings>& OldSettings);
};




enum EPDRedirectType
{
	/* The pins do not match */
	ERedirectType_None,
	/* The pins match by name or redirect and have the same type (or we're ok with the mismatched type) */
	ERedirectType_Name,
	/* The pins match via a redirect and the value needs to also be redirected */
	ERedirectType_Value,
	/* The pins differ by type and have a default value*/
	ERedirectType_DefaultValue,
};


// @todo rewrite kismet makestruct node for usage in the mission graph as intended
UCLASS(MinimalAPI)
class UPDMissionNode_MakeStruct : public UPDMissionGraphNode
{
	GENERATED_UCLASS_BODY()
	
	/**
	* Returns false if:
	*   1. The Struct has a 'native make' method
	* Returns true if:
	*   1. The Struct is tagged as BlueprintType
	*   and
	*   2. The Struct has any property that is tagged as CPF_BlueprintVisible and is not CPF_BlueprintReadOnly
	*/
	PDMISSIONEDITOR_API static bool CanBeMade(const UScriptStruct* Struct, bool bForInternalUse = false);
	
	/** Can this struct be used as a split pin */
	PDMISSIONEDITOR_API static bool CanBeSplit(const UScriptStruct* Struct, UBlueprint* InBP);

	// UObject interface
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void Serialize(FArchive& Ar) override;
	// End of UObject interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void PreloadRequiredAssets();
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PostPlacedNewNode() override;
	//~ End  UEdGraphNode Interface

	//~ Begin K2Node Interface
	virtual bool NodeCausesStructuralBlueprintChange() const { return false; }
	virtual bool IsNodePure() const { return true; }
	virtual bool DrawNodeAsVariable() const { return false; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const;
	virtual EPDRedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const;
	virtual FText GetMenuCategory() const;
	virtual void ConvertDeprecatedNode(UEdGraph* Graph, bool bOnlySafeChanges);
	//~ End K2Node Interface

protected:
	// Ensures the specified object is preloaded.  ReferencedObject can be NULL.
	void PreloadObject(UObject* ReferencedObject)
	{
		if ((ReferencedObject != NULL) && ReferencedObject->HasAnyFlags(RF_NeedLoad))
		{
			ReferencedObject->GetLinker()->Preload(ReferencedObject);
		}
	}
	
	
	struct FMakeStructPinManager : public FPDOptionalPinManager
	{
		const uint8* const SampleStructMemory;
	public:
		FMakeStructPinManager(const uint8* InSampleStructMemory);

		bool HasAdvancedPins() const { return bHasAdvancedPins; }
	protected:
		virtual void GetRecordDefaults(FProperty* TestProperty, FOptionalPinFromProperty& Record) const override;
		virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex, FProperty* Property) const override;
		virtual bool CanTreatPropertyAsOptional(FProperty* TestProperty) const override;

		/** set by GetRecordDefaults(), mutable as it is a const function */
		mutable bool bHasAdvancedPins;
	};

	
public:
	/** Helper property to handle upgrades from an old system of displaying pins for
	 *	the override values that properties referenced as a conditional of being set in a struct */
	UPROPERTY()
	bool bMadeAfterOverridePinRemoval;

	UPROPERTY(EditAnywhere, Category=PinOptions, EditFixedSize)
	TArray<FOptionalPinFromProperty> ShowPinForProperties;

private:
	/** Constructing FText strings can be costly, so we cache the node's title/tooltip */
	FNodeTextCache CachedTooltip;
	FNodeTextCache CachedNodeTitle;

	TArray<FName> OldShownPins;

	/** Class that this variable is defined in.  */
	UPROPERTY()
	TObjectPtr<UScriptStruct> StructType;	
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