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

#pragma once


#include <CoreMinimal.h>
#include <UObject/ObjectMacros.h>
#include <EdGraph/EdGraphNode.h>

#include "PDMissionGraph.h"
#include "PDMissionGraphTypes.h"

#include "PDMissionGraphNode.generated.h"

class UEdGraph;
class UEdGraphPin;
class UEdGraphSchema;
struct FDiffResults;
struct FDiffSingleResult;

UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** instance class */
	UPROPERTY()
	struct FPDMissionNodeData ClassData;

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

	/** error message for node */
	UPROPERTY()
	FString ErrorMessage;

	//~ Begin UEdGraphNode Interface
	virtual class UPDMissionGraph* GetMissionGraph();
	virtual void AutowireNewNode(UEdGraphPin* FromPin) override;
	virtual void PostPlacedNewNode() override;
	virtual void PrepareForCopying() override;
	virtual bool CanDuplicateNode() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void DestroyNode() override;
	virtual FText GetTooltipText() const override;
	virtual void NodeConnectionListChanged() override;
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

	/** reinitialize node instance */
	virtual bool RefreshNodeClass();

	/** updates ClassData from node instance */
	virtual void UpdateNodeClassData();

	/**
	 * Checks for any errors in this node and updates ErrorMessage with any resulting message
	 * Called every time the graph is serialized (i.e. loaded, saved, execution index changed, etc)
	 */
	virtual void UpdateErrorMessage();

	/** Check if node instance uses blueprint for its implementation */
	bool UsesBlueprint() const;

	/** check if node has any errors, used for assigning colors on graph */
	virtual bool HasErrors() const;

	virtual bool CanPlaceBreakpoints() const { return false; }
	
	static void UpdateNodeDataFrom(UClass* InstanceClass, FPDMissionNodeData& UpdatedData);

protected:

	virtual void ResetNodeOwner();


	void CreateMissionPin();
};


/** Root node of this mission block */
UCLASS()
class PDMISSIONEDITOR_API UPDMissionGraphNode_EntryPoint : public UPDMissionGraphNode
{
	GENERATED_BODY()
	UPDMissionGraphNode_EntryPoint(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}
	
	virtual void AllocateDefaultPins() override
	{
		// Custom pins (Input)
		CreateMissionPin(); // @todo when set, generate the mission/questline in the graph using this node as its root
		
		// No pins for requirements
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("Out"));
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
	UPDMissionGraphNode_MainQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("In"));
		
		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)		
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("Out"));
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
	UPDMissionGraphNode_SideQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("In"));

		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("Out"));
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
	UPDMissionGraphNode_EventQuest(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
	{
		bIsSubNode = false;
	}

	virtual void AllocateDefaultPins() override
	{
		// Simple pins (Input)
		CreatePin(EGPD_Input, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("In"));

		// Custom pins (Input)
		CreateMissionPin();

		// Simple pins (Output)
		CreatePin(EGPD_Output, FPDMissionGraphTypes::PinCategory_MultipleNodes, TEXT("Out"));
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