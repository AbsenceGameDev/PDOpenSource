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
	FPDMissionNodeData(UClass* InStruct, const FString& InDeprecatedMessage);
	FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UClass* InStruct);
	FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InStruct);

	FString ToString() const;
	FString GetDataEntryName() const;
	FText GetCategory() const;
	FString GetDisplayName() const;
	FText GetTooltip() const;
	UClass* GetClass(bool bSilent = false);

	FORCEINLINE FString GetDeprecatedMessage() const { return DeprecatedMessage; }
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

	/** message for deprecated class */
	FString DeprecatedMessage;
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
	
	// 'Source code' graphs (of type UPDMissionGraph)
	UPROPERTY()
	TArray<TObjectPtr<UEdGraph>> SourceGraphs;
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

	static const FName PinCategory_Name;
	static const FName PinCategory_String;
	static const FName PinCategory_Text;
	static const FName PinCategory_MissionRow;
	static const FName PinCategory_MissionDataRef;
	static const FName PinCategory_MissionRowKeyBuilder;
	static const FName PinCategory_MultipleNodes;
	static const FName PinCategory_SingleComposite;
	static const FName PinCategory_SingleTask;
	static const FName PinCategory_SingleNode;
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
protected:
	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;

public:
	FPDMissionGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	// FConnectionDrawingPolicy interface 
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& PinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;
	// End of FConnectionDrawingPolicy interface

protected:
	void Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params);
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