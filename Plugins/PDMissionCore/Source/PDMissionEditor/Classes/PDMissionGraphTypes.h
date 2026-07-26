/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "ConnectionDrawingPolicy.h"
#include "PDMissionCommon.h"
#include "PDMissionGraphTypes.generated.h"

class FSlateWindowElementList;
class UEdGraph;

/** @brief Selector enum for mission types */
enum class EMissionGraphSubNodeType : uint8
{
	MainQuest,
	SideQuest,
	EventQuest,

	// Visual nodes
	Knot,
	Transition,
};

/** @brief Selector enum for mission modifiers */
enum class EMissionGraphSubNodeModifier : uint8
{
	Unique,
	Repeatable_Req,
	Repeatable_Always,
};

/** @brief Currently unused node data struct, might remove and replace any commented out usage of it */
USTRUCT()
struct PDMISSIONEDITOR_API FPDMissionNodeData
{
	GENERATED_USTRUCT_BODY()

	FPDMissionNodeData(): bIsHidden(0), bHideParent(0) {}
	FPDMissionNodeData(UClass* InStruct);
	FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UClass* InStruct);
	FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InStruct);

	FString ToString() const;
	FString GetDataEntryName() const;
	FText GetCategory() const;
	FString GetDisplayName() const;
	FText GetTooltip() const;
	UClass* GetClass(bool bSilent = false);
	
	FORCEINLINE FString GetPackageName() const { return GeneratedPackage; }

	bool operator==(const FPDMissionNodeData& Other) const;
	bool operator!=(const FPDMissionNodeData& Other) const;

	
	/** set when child class masked this one out (e.g. always use game specific class instead of engine one) */
	uint32 bIsHidden : 1;

	/** set when class wants to hide parent class from selection (just one class up hierarchy) */
	uint32 bHideParent : 1;

private:

	/** pointer to ustruct */
	TWeakObjectPtr<UClass> Class;

	/** path to structs owning asset if it's not loaded yet */
	UPROPERTY()
	FString AssetName;
	
	UPROPERTY()
	FString GeneratedPackage;

	/** resolved name of class from asset data */
	UPROPERTY()
	FString ClassName;

	/** User-defined category for this class */
	UPROPERTY()
	FText Category;
};

/** @brief Editor tab IDs */
struct FPDMissionEditorTabs
{
	// Tab identifiers
	static const FName GraphDetailsID;
	static const FName SearchID;

	// Document tab identifiers
	static const FName GraphEditorID;
	static const FName TreeEditorID;
};

/** @brief Node handle */
USTRUCT()
struct PDMISSIONEDITOR_API FPDMissionNodeHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mission Subsystem", Meta = (RowType="/Script/PDMissionCore.PDMissionRow"))
	FDataTableRowHandle DataTarget;
	
	UPROPERTY()
	TArray<FEditedDocumentInfo> LastEditedDocuments;
};


#define FOnCanPair(Token)\
void On##Token (); \
bool Can##Token () const; \

/** @brief Debugger handler, currently most functiosn are not implemented */
class PDMISSIONEDITOR_API FPDMissionDebuggerHandler : public TSharedFromThis<FPDMissionDebuggerHandler>
{
public:
	// Debugging
	/** Setup commands */
	void BindDebuggerToolbarCommands();
	
	/** Refresh the debugger display */
	void RefreshDebugger();
	
	FText HandleGetDebugKeyValue(const FName& InKeyName, bool bUseCurrentState) const;
	float HandleGetDebugTimeStamp(bool bUseCurrentState) const;

	FOnCanPair(EnableBreakpoint)
	FOnCanPair(ToggleBreakpoint)
	FOnCanPair(DisableBreakpoint)
	FOnCanPair(AddBreakpoint)
	FOnCanPair(RemoveBreakpoint)
	FOnCanPair(SearchMissionDatabase)
	
	FReply JumpToNode(const UEdGraphNode* Node);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void UpdateToolbar();

	bool IsPropertyEditable() const;
	bool IsDebuggerReady() const;
	bool IsDebuggerPaused() const;
	
	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	FText GetDebuggerActorDesc() const;
};


/** @brief Graph/Node type */
USTRUCT()
struct PDMISSIONEDITOR_API FPDMissionGraphTypes
{
	GENERATED_USTRUCT_BODY()

	static const FName PinCategory_MissionName;
	static const FName PinCategory_String;
	static const FName PinCategory_Text;
	static const FName PinCategory_GenericData;	
	static const FName PinCategory_SectionLabel;
	static const FName PinCategory_NewMission;
	static const FName PinCategory_TagSelector;
	static const FName PinCategory_MissionRow;
	static const FName PinCategory_MissionDataRef;
	static const FName PinCategory_MissionRowKeyBuilder;
	static const FName PinCategory_LogicalPath;
	static const FText NodeText_MainMission;
	static const FText NodeText_SideMission;
	static const FText NodeText_EventMission;
};

/** @brief Constexpr colours */
namespace MissionTreeColors
{
	namespace NodeBody
	{
		constexpr FLinearColor MainQuest(0.24f, 0.055f, 0.715f);
		constexpr FLinearColor SideQuest(0.1f, 0.05f, 0.2f);
		constexpr FLinearColor EventQuest(0.0f, 0.07f, 0.4f);
		constexpr FLinearColor Default(0.15f, 0.15f, 0.15f);
		constexpr FLinearColor Root(0.5f, 0.5f, 0.5f, 0.1f);
		constexpr FLinearColor Error(1.0f, 0.0f, 0.0f);
	}

	namespace NodeBorder
	{
		constexpr FLinearColor Inactive(0.08f, 0.08f, 0.08f);
		constexpr FLinearColor Root(0.2f, 0.2f, 0.2f, 0.2f);
		constexpr FLinearColor Selected(1.00f, 0.08f, 0.08f);
		constexpr FLinearColor Disconnected(0.f, 0.f, 0.f);
		constexpr FLinearColor BrokenWithParent(1.f, 0.f, 1.f);
		constexpr FLinearColor QuickFind(0.f, 0.8f, 0.f);
	}

	namespace Pin
	{
		constexpr FLinearColor MainQuestPath(0.9f, 0.2f, 0.15f);
		constexpr FLinearColor SideQuestPath(1.0f, 0.7f, 0.0f);
		constexpr FLinearColor EventQuestPath(0.13f, 0.03f, 0.4f);
		constexpr FLinearColor Default(0.02f, 0.02f, 0.02f);
		constexpr FLinearColor SingleNode(0.02f, 0.02f, 0.02f);
	}
	
	namespace Connection
	{
		constexpr FLinearColor Default(1.0f, 1.0f, 1.0f);
	}

	namespace Debugger
	{
		constexpr FLinearColor SearchSucceeded(0.0f, 1.0f, 0.0f);
		constexpr FLinearColor SearchFailed(1.0f, 0.0f, 0.0f);

		constexpr FLinearColor DescHeader(0.3f, 0.8f, 0.4f);
		constexpr FLinearColor DescKeys(0.3f, 0.4f, 0.8f);
	}

	namespace Action
	{
		constexpr FLinearColor DragMarker(1.0f, 1.0f, 0.2f);
	}
}


/** @brief (Node-)Connectivity policy, how will nodes hook up to eachother */
class FPDMissionGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FPDMissionGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	virtual void DetermineLinkGeometry(
		FArrangedChildren& ArrangedNodes, 
		TSharedRef<SWidget>& OutputPinWidget,
		UEdGraphPin* OutputPin,
		UEdGraphPin* InputPin,
		/*out*/ FArrangedWidget*& StartWidgetGeometry,
		/*out*/ FArrangedWidget*& EndWidgetGeometry
		) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;
	// End of FConnectionDrawingPolicy interface

protected:
	void Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params);

protected:
	UEdGraph* GraphObj;

	TMap<UEdGraphNode*, int32> NodeWidgetMap;	

	// Draw line-based circle (no solid filling)
	void DrawCircle(const FVector2D& Center, float Radius, const FLinearColor& Color, const int NumLineSegments);
	TArray<FVector2D> TempPoints;

	static constexpr float RelinkHandleHoverRadius = 20.0f;
};


#if UE_ENABLE_INCLUDE_ORDER_DEPRECATED_IN_5_2
#include "CoreMinimal.h"
#endif


/** @brief Asset action that is applied to all UPDMissionDataTable class. Replaces their default editor */
class PDMISSIONEDITOR_API FAssetTypeActions_MissionEditor : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "FAssetTypeActions_MissionEditor", "Mission Databank"); }
	virtual FColor GetTypeColor() const override { return FColor(149,70,255); }
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>() ) override;
	virtual uint32 GetCategories() override;
	// virtual void PerformAssetDiff(UObject* Asset1, UObject* Asset2, const struct FRevisionInfo& OldRevision, const struct FRevisionInfo& NewRevision) const override;

private:
	
	void OpenInDefaults(UDataTable* OldBank, UDataTable* NewBank) const;
};

/** @brief Child class to use as filter for about TypeAction */
UCLASS()
class UPDMissionDataTable : public UDataTable
{
	GENERATED_BODY()

public:
	
};


/** @brief Table row that is meant for an intermediary/transient datatable that is created by teh mission graph when in the mission graph and editing */
USTRUCT()
struct PDMISSIONEDITOR_API FPDAssociativeMissionEditingRow : public FTableRowBase // (secret table type ):
{
	GENERATED_BODY()
	
	/** @brief Flag to tell if we have marked the target row for removal  */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Mission|Data")
	uint8 bMarkForRemoval = false;

	/** @brief The row this row will update */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Mission|Data")
	FDataTableRowHandle TargetRow{};

	/** @brief If true when TargetRow is invalid, means it is a new row to be stored in the target datatable at save */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Mission|Data")
	uint8 bWasUpdated = false; 

	/** @brief The row data that will be stored in the target */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, Category = "Mission|Data")
	FPDMissionRow UpdateRowData{};
};

// TODO: Pseudo code, thoughts to explore later
// USTRUCT()
// struct PDMISSIONEDITOR_API FPDCreationDelegateParams
// {
// 	GENERATED_BODY()
// 	FPDMissionRow* MissionRow = nullptr;
// 	int32 OptionalBranchIndex = INDEX_NONE;
// 	TSharedPtr<class IStructureDetailsView> StructureDetailsView;
// 	TSharedPtr<class IMenu> CondPopup;
// };
//
// DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FPDWidgetCreationDelegate, FPDCreationDelegateParams)


/**
 *	@brief Static functions exposed to blueprint
 *  @note Many of these pinname manipulations are unsafe due to no bounds checking. Use very responsibly and only when you know a branch pin exists
 */
UCLASS(BlueprintType)
class PDMISSIONEDITOR_API UPDMissionEditorStatics : public UObject
{
	GENERATED_BODY()

public:
	typedef struct NodeOp
	{
		static FORCEINLINE int32 GetBranchIdxFromPinName(UEdGraphPin* Pin) 
		{
			FString PinStr = Pin->PinName.ToString();
			const int32 PrioTextIndex = FString(TEXT("Prio ")).Len();
			const int32 ColonTextIndex = PinStr.Find(FString(TEXT(" : ")));
			PinStr = PinStr.Left(ColonTextIndex); // Prio 54 :  a  s  d  a öekad  ->  Prio 54
			PinStr = PinStr.Right(PinStr.Len() - PrioTextIndex);
			return FCString::Atoi(*PinStr);
		}
	
		static FORCEINLINE void OffsetPinName(UEdGraphPin* Pin) 
		{
			FString PinStr = Pin->PinName.ToString();
			const int32 ColonTextIndex = PinStr.Find(FString(TEXT(" : ")));
			const int32 BranchIdx = GetBranchIdxFromPinName(Pin);
	
			const int32 NewBranchIdx = BranchIdx - 1;
			if (NewBranchIdx >= 0)
			{
				const int32 MissionTextIndex = FString(TEXT(" : ")).Len();
				const FString MissionNamePart = Pin->PinName.ToString().RightChop(ColonTextIndex + MissionTextIndex);
				const FString PinPrio = FString::Printf(TEXT("Prio %i : "), NewBranchIdx);
				const FString PinName = PinPrio + MissionNamePart;
				
				Pin->PinName = FName(*PinName);
			}
		}	
	
		static FORCEINLINE FString BuildPinName(int32 BranchIdx, const FName& MissionName)
		{
			const FString PinPrio = FString::Printf(TEXT("Prio %i : "), BranchIdx);
			const FString PinName = PinPrio + FString{MissionName != NAME_None ? MissionName.ToString() : TEXT("CONNECT ME")};
			return PinName;
		}
		static FORCEINLINE FString BuildPinName(int32 BranchIdx, const FString& MissionName)
		{
			const FString PinPrio = FString::Printf(TEXT("Prio %i : "), BranchIdx);
			const FString PinName = PinPrio + FString{false == MissionName.IsEmpty() ? MissionName : TEXT("CONNECT ME")};
			return PinName;
		}
		static FORCEINLINE FString BuildPinName(int32 BranchIdx, const FPDMissionBranchElement& Branch)
		{
			const FName& MissionName = Branch.Target.RowName;
			const FString PinPrio = FString::Printf(TEXT("Prio %i : "), BranchIdx);
			const FString PinName = PinPrio + FString{MissionName != NAME_None ? MissionName.ToString() : TEXT("CONNECT ME")};
			return PinName;
		}

		static FORCEINLINE int32 FindDirectionIndex(const UEdGraphPin* SourcePin, const TArray<UEdGraphPin*>& NodePins, EEdGraphPinDirection CountDirection)
		{
			int32 DirPinIdx = INDEX_NONE;
			for(UEdGraphPin* PinIt : NodePins)
			{
				switch(CountDirection)
				{
					case EGPD_Input:
					DirPinIdx += PinIt->Direction == EEdGraphPinDirection::EGPD_Input;
					break;

					case EGPD_Output: case EGPD_MAX:
					DirPinIdx += PinIt->Direction > EEdGraphPinDirection::EGPD_Input;
					break;
				}

				if (SourcePin == PinIt)
				{
					break;
				}
			}
			return DirPinIdx;
		}
		
		static PDMISSIONEDITOR_API class UPDMissionGraphNode* ResolveNextNode(const UEdGraphPin* PinOnPotentialKnot);
		static PDMISSIONEDITOR_API TTuple<class UPDMissionGraphNode*, UEdGraphPin*> ResolveMissionNodeFromKnot(UEdGraphPin* SourcePin, const EEdGraphPinDirection PinDir);
		static PDMISSIONEDITOR_API bool DoesNodePathContainConditionNode(UEdGraphPin* SourcePin, const EEdGraphPinDirection PinDir);
		static PDMISSIONEDITOR_API bool IsRowBasedMissionNode(UEdGraphNode* Node);


		// TODO: Thoughts to explore later
		// static PDMISSIONEDITOR_API FReply GenericOnClicked(bool& bOpenedNodeOutParam, UPDMissionGraphNode* SourceNode, UEdGraphPin* FinalSourcePin,TSharedRef<const SWidget> SpawnWidgetPathSource, bool bLookForBranch, FPDWidgetCreationDelegate Delegate, FPDCreationDelegateParams& MutableParams);

	};

	//  Updating Mission branch data to reflect the new node
	typedef struct RowOp
	{
		static FORCEINLINE void AddBranch(FPDMissionRow* MissionRowPtr, const FPDMissionBranchElement& OptionalValue = FPDMissionBranchElement{})
		{
			if (MissionRowPtr) {MissionRowPtr->ProgressRules.NextMissionBranch.Branches.Emplace(OptionalValue);}
		}
		static FORCEINLINE void RemoveBranch(FPDMissionRow* MissionRowPtr, int32 RemoveIdx)
		{
			if (MissionRowPtr && MissionRowPtr->ProgressRules.NextMissionBranch.Branches.IsValidIndex(RemoveIdx)) 
			{
				MissionRowPtr->ProgressRules.NextMissionBranch.Branches.Emplace(FPDMissionBranchElement{});
			}
		}		
		static FORCEINLINE void UpdateBranch(FPDMissionRow* MissionRowPtr,  int32 UpdateIdx, const FPDMissionBranchElement& NewValue)
		{
			if (MissionRowPtr && MissionRowPtr->ProgressRules.NextMissionBranch.Branches.IsValidIndex(UpdateIdx)) 
			{
				MissionRowPtr->ProgressRules.NextMissionBranch.Branches[UpdateIdx] = NewValue;
			}
		}
		static FORCEINLINE const FPDMissionBranchElement& GetBranch(FPDMissionRow* MissionRowPtr,  int32 BranchIdx)
		{
			static const FPDMissionBranchElement Dummy{};
			if (MissionRowPtr && MissionRowPtr->ProgressRules.NextMissionBranch.Branches.IsValidIndex(BranchIdx)) 
			{
				return MissionRowPtr->ProgressRules.NextMissionBranch.Branches[BranchIdx];
			}
			return Dummy;
		}

		static FORCEINLINE void AddBranch(FDataTableRowHandle* RowHandlePtr, const FString Ctxt = TEXT(""))
		{
			AddBranch(RowHandlePtr->GetRow<FPDMissionRow>(Ctxt));
		}
		static FORCEINLINE void RemoveBranch(FDataTableRowHandle* RowHandlePtr, int32 RemoveIdx, const FString Ctxt = TEXT(""))
		{
			RemoveBranch(RowHandlePtr->GetRow<FPDMissionRow>(Ctxt), RemoveIdx);
		}
		static FORCEINLINE void UpdateBranch(FDataTableRowHandle* RowHandlePtr, int32 UpdateIdx, const FPDMissionBranchElement& NewValue, const FString Ctxt = TEXT(""))
		{
			UpdateBranch(RowHandlePtr->GetRow<FPDMissionRow>(Ctxt), UpdateIdx, NewValue);
		}
		static FORCEINLINE const FPDMissionBranchElement& GetBranch(FDataTableRowHandle* RowHandlePtr, int32 BranchIdx, const FString Ctxt = TEXT(""))
		{
			return GetBranch(RowHandlePtr->GetRow<FPDMissionRow>(Ctxt), BranchIdx);
		}
		static PDMISSIONEDITOR_API bool IsMissionValid(const FName& SelectedMissionRowName, FDataTableRowHandle*& OutRowHandlePtr);
		static PDMISSIONEDITOR_API void SetValuesOnBranchTargetAtIndex(const FName& SourceMissionRowName, int32 BranchIdx, const FName& BranchTargetMissionName, const FString Ctxt = TEXT(""));
	private:
	};
};




/**
 * @brief 
 * @todo move to new file
 */
struct PDMISSIONEDITOR_API FPDPendingConditionNode
{
	UPROPERTY()
	UEdGraph* TargetGraph = nullptr;
	FIntVector2 SpawnLocation = {};

	UPROPERTY()
	TWeakObjectPtr<UEdGraphNode> SourceNode = nullptr;
	UPROPERTY()
	int32 SourceBranchPinIdx = INDEX_NONE;
	UPROPERTY()
	TWeakObjectPtr<UEdGraphNode> TargetNode = nullptr;
};


/**
 * @brief 
 * @todo move to new file
 */
UCLASS(Blueprintable)
class PDMISSIONEDITOR_API UPDMissionEditorSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:

	static UPDMissionEditorSubsystem* Get();

	
	void QueueConditionNode(const FPDPendingConditionNode& PendingConditionNode);
	void SpawnConditionNodes();	
	static void ProcessPinConnection(const FConnectionParams& Params);	
	

private:
	// TQueue<FPDPendingConditionNode> PendingNodesToCreate;
	TMap<UEdGraphNode*, FPDPendingConditionNode> PendingNodesToCreate;


};



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