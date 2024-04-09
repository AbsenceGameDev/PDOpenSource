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

	static const FName PinCategory_MissionRow;
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
