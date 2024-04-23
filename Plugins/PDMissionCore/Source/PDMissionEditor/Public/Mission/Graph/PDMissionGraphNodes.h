/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include <EdGraph/EdGraphNode.h>

#include "PDMissionGraphNodeBase.h"
#include "PDMissionGraphTypes.h"

#include "PDMissionGraphNodes.generated.h"

class UPDMissionTransitionNode;
class UEdGraph;
class UEdGraphPin;
class UEdGraphSchema;
struct FDiffResults;
struct FDiffSingleResult;


/** Root node of this mission block */
UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode_EntryPoint : public UPDMissionGraphNode
{
	GENERATED_BODY()

public:
	UPDMissionGraphNode_EntryPoint(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}
	
	virtual void AllocateDefaultPins() override
	{
		// Custom pins (Selector Input)
		CreateMissionPin(); // @todo when set, generate the mission/questline in the graph using this node as its root

		// Logical Output
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("Out"));

		// Custom pins (Data Input)
		Super::AllocateDefaultPins(); // super call generates the entries for the mission data
	}
	
	virtual FLinearColor GetNodeBodyTintColor() const override
	{
		return MissionTreeColors::NodeBody::Default;
	}
};


UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode_MainQuest : public UPDMissionGraphNode
{
	GENERATED_BODY()

public:
	UPDMissionGraphNode_MainQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("In"));
		
		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)		
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("Out"));

		Super::AllocateDefaultPins(); // super call generates the entries for the mission data
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FPDMissionGraphTypes::NodeText_MainMission; ;
	}
	
	virtual FLinearColor GetNodeBodyTintColor() const override
	{
		return MissionTreeColors::NodeBody::Default;
	}
};

UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode_SideQuest : public UPDMissionGraphNode
{
	GENERATED_BODY()

public:
	UPDMissionGraphNode_SideQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("In"));

		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("Out"));

		Super::AllocateDefaultPins(); // super call generates the entries for the mission data
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FPDMissionGraphTypes::NodeText_SideMission;
	}	
	
	virtual FLinearColor GetNodeBodyTintColor() const override
	{
		return MissionTreeColors::NodeBody::Default;
	}
};


UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode_EventQuest : public UPDMissionGraphNode
{
	GENERATED_BODY()

public:
	UPDMissionGraphNode_EventQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("In"));

		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_LogicalPath, TEXT("Out"));

		Super::AllocateDefaultPins(); // super call generates the entries for the mission data
	}

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override
	{
		return FPDMissionGraphTypes::NodeText_EventMission;
	}		
	
	virtual FLinearColor GetNodeBodyTintColor() const override
	{
		return MissionTreeColors::NodeBody::Default;
	}
};


class SGraphNode;
UCLASS(MinimalAPI)
class UPDMissionGraphNode_Knot : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool ShouldOverridePinNames() const override;
	virtual FText GetPinNameOverride(const UEdGraphPin& Pin) const override;
	virtual void OnRenameNode(const FString& NewName) override;
	virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const override;
	virtual bool CanSplitPin(const UEdGraphPin* Pin) const override;
	virtual bool IsCompilerRelevant() const override { return false; }
	virtual UEdGraphPin* GetPassThroughPin(const UEdGraphPin* FromPin) const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& OutInputPinIndex, int32& OutOutputPinIndex) const override { OutInputPinIndex = 0; OutOutputPinIndex = 1; return true; }
	// End of UEdGraphNode interface
	
	UEdGraphPin* GetInputPin() const
	{
		return Pins[0];
	}

	UEdGraphPin* GetOutputPin() const
	{
		return Pins[1];
	}
};


UCLASS(MinimalAPI, config=Editor)
class UPDMissionTransitionNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	// @todo replace bound graph with a data ref to teh mission transition rules of the owning mission 
	// UPROPERTY()
	// TObjectPtr<class UEdGraph> BoundGraph;
	

	// The priority order of this transition. If multiple transitions out of a state go
	// true at the same time, the one with the smallest priority order will take precedent
	UPROPERTY(EditAnywhere, Category=Transition)
	int32 PriorityOrder;
	
	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual bool CanDuplicateNode() const override { return true; }
	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;
	virtual void PostPlacedNewNode() override;
	virtual void DestroyNode() override;
	//~ End UEdGraphNode Interface

	//~ Begin UPDMissionGraphNode Interface
	virtual UEdGraphPin* GetInputPin() const { return Pins[0]; }
	virtual UEdGraphPin* GetOutputPin() const { return Pins[1]; }
	//~ End UPDMissionGraphNode Interface

	// @return the name of this state
	PDMISSIONEDITOR_API FString GetMissionName() const;

	static UPDMissionGraphNode* GetLinkedNode(TArray<UEdGraphPin*>& Links, int32 LinkIdx);
	
	PDMISSIONEDITOR_API UPDMissionGraphNode* GetOwningMission() const;
	PDMISSIONEDITOR_API UPDMissionGraphNode* GetTargetMission() const;
	PDMISSIONEDITOR_API void CreateConnections(const UPDMissionGraphNode* PreviousState, const UPDMissionGraphNode* NextState);

	/**
	 * Relink transition head (where the arrow is of a state transition) to a new state.
	 * @param[in] NewTargetState The new transition target.
	 */
	PDMISSIONEDITOR_API void RelinkHead(const UPDMissionGraphNode* NewTargetState);

	/**
	 * Helper function to gather the transition nodes to be relinked by taking the graph selection into account as well.
	 * For example when relinking a transition holding several transition nodes but only a few are selected to be relinked.
	 */
	PDMISSIONEDITOR_API static TArray<UPDMissionTransitionNode*> GetListTransitionNodesToRelink(const UEdGraphPin* SourcePin, const UEdGraphPin* OldTargetPin, const TArray<UEdGraphNode*>& InSelectedGraphNodes);
	
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