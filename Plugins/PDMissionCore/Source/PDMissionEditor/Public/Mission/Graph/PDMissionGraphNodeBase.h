/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */
#pragma once

#include <CoreMinimal.h>

#include "PDMissionGraphTypes.h"
#include "PDMissionGraphNodeBase.generated.h"

class UPDMissionTransitionNode;

USTRUCT()
struct FPDNodeColours
{
	GENERATED_BODY()

	FLinearColor Title   = FLinearColor(40, 40, 40);	 // DarkGray
	FLinearColor Comment = FLinearColor::Gray;	
	FLinearColor Body    = FLinearColor(180, 180, 180); // LightGray
};

UCLASS()
class UPDMissionGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()
	
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
	virtual class UPDMissionGraph* GetMissionGraph();
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void PrepareForCopying() override;
	virtual bool CanDuplicateNode() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void DestroyNode() override;
	
	virtual TSharedPtr<SWidget> CreateNodeImage() const override;
	virtual FLinearColor GetNodeBodyTintColor() const override;
	virtual FLinearColor GetNodeCommentColor() const override;
	
	virtual void NodeConnectionListChanged() override;

	/** Create a visual widget to represent this node in a graph editor or graph panel.  If not implemented, the default node factory will be used. */
	virtual TSharedPtr<SGraphNode> CreateVisualWidget();

	/** Create the background image for the widget representing this node */
	// virtual TSharedPtr<SWidget> CreateNodeImage() const { return TSharedPtr<SWidget>(); }
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const override;
	virtual void FindDiffs(class UEdGraphNode* OtherNode, struct FDiffResults& Results) override;
	virtual FString GetPropertyNameAndValueForDiff(const FProperty* Prop, const uint8* PropertyAddr) const override;
	//~ End UEdGraphNode Interface

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditImport() override;
	virtual void PostEditUndo() override;
#endif
	// End UObject

	
	virtual FText GetMenuCategory() const;
	// @return the input pin for this state
	virtual UEdGraphPin* GetInputPin(int32 InputIndex = 0) const;
	// @return the output pin for this state
	virtual UEdGraphPin* GetOutputPin(int32 InputIndex = 0) const;
	virtual UEdGraph* GetBoundGraph() const { return nullptr; }

	virtual FText GetDescription() const;
	virtual void PostCopyNode();

	void AddSubNode(UPDMissionGraphNode* SubNode, class UEdGraph* ParentGraph);
	void RemoveSubNode(UPDMissionGraphNode* SubNode);
	virtual void RemoveAllSubNodes();
	virtual void OnSubNodeRemoved(UPDMissionGraphNode* SubNode);
	virtual void OnSubNodeAdded(UPDMissionGraphNode* SubNode);

	virtual int32 FindSubNodeDropIndex(UPDMissionGraphNode* SubNode) const;
	virtual void InsertSubNodeAt(UPDMissionGraphNode* SubNode, int32 DropIndex);

	/** check if node is sub-node */
	virtual bool IsSubNode() const;

	/** initialize instance object  */
	virtual void InitializeInstance();

	/** updates ClassData from node instance */
	virtual void UpdateNodeClassData();

	/** Check if node instance uses blueprint for its implementation */
	bool UsesBlueprint() const;

	/** check if node has any errors, used for assigning colors on graph */
	virtual bool HasErrors() const;

	virtual bool CanPlaceBreakpoints() const { return false; }
	
	void RefreshDataRefPins(const FName& MissionRowName);

	FString GetMissionName() { return "finish me"; }
	
	void GetMissionTransitions(TArray<UPDMissionTransitionNode*>& Array); // Get all mission transition for this node

	static void UpdateNodeDataFrom(UClass* InstanceClass, FPDMissionNodeData& UpdatedData);

protected:
	virtual void ResetNodeOwner();
	
	void CreateMissionPin();
	
	// Ensures the specified object is preloaded in case it is not invalid
	FORCEINLINE void PreloadObject(UObject* ReferencedObject)
	{
		const bool bCantPreload = ReferencedObject == nullptr || ReferencedObject->HasAnyFlags(RF_NeedLoad) == false;
		if (bCantPreload) { return; }
		ReferencedObject->GetLinker()->Preload(ReferencedObject);
	}

public:

	/** instance class */
	UPROPERTY()
	struct FPDMissionNodeData ClassData;

	UPROPERTY()
	FPDNodeColours NodeColours;	
	
	UPROPERTY()
	TObjectPtr<UStruct> NodeInstance;

	UPROPERTY(transient)
	TObjectPtr<UPDMissionGraphNode> ParentNode;

	UPROPERTY()
	TArray<TObjectPtr<UPDMissionGraphNode>> SubNodes;

	/** sub-node index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeIndex;

	/** if set, all modifications (including delete/cut) are disabled */
	UPROPERTY()
	uint32 bIsReadOnly : 1;

	/** if set, this node will be always considered as a sub-node */
	UPROPERTY()
	uint32 bIsSubNode : 1;
	
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