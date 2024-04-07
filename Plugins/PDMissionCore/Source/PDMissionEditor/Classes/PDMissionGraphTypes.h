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
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "PDMissionGraphTypes.generated.h"

USTRUCT()
struct PDMISSIONEDITOR_API FPDMissionNodeData
{
	GENERATED_USTRUCT_BODY()

	FPDMissionNodeData(): bIsHidden(0), bHideParent(0) {}
	FPDMissionNodeData(UStruct* InStruct, const FString& InDeprecatedMessage);
	FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UStruct* InStruct);
	FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UStruct* InStruct);

	FString ToString() const;
	FString GetDataEntryName() const;
	FText GetCategory() const;
	FString GetDisplayName() const;
	FText GetTooltip() const;
	UStruct* GetStruct(bool bSilent = false);


	FORCEINLINE bool IsBlueprint() const { return AssetName.Len() > 0; }
	FORCEINLINE bool IsDeprecated() const { return DeprecatedMessage.Len() > 0; }
	FORCEINLINE FString GetDeprecatedMessage() const { return DeprecatedMessage; }
	FORCEINLINE FString GetPackageName() const { return GeneratedPackage; }

	/** set when child class masked this one out (e.g. always use game specific class instead of engine one) */
	uint32 bIsHidden : 1;

	/** set when class wants to hide parent class from selection (just one class up hierarchy) */
	uint32 bHideParent : 1;

private:

	/** pointer to ustruct */
	TWeakObjectPtr<UStruct> Struct;

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

struct PDMISSIONEDITOR_API FPDMissionDataNode
{
	FPDMissionNodeData Data;
	FString ParentClassName;

	TSharedPtr<FPDMissionDataNode> ParentNode;
	TArray<TSharedPtr<FPDMissionDataNode> > SubNodes;

	void AddUniqueSubNode(TSharedPtr<FPDMissionDataNode> SubNode);
};

struct PDMISSIONEDITOR_API FPDMissionDataNodeHelper
{
	DECLARE_MULTICAST_DELEGATE(FOnPackageListUpdated);

	FPDMissionDataNodeHelper(UClass* InRootClass);
	~FPDMissionDataNodeHelper();

	void GatherClasses(const UClass* BaseClass, TArray<FPDMissionNodeData>& AvailableClasses);
	static FString GetDeprecationMessage(const UClass* Class);

	void OnAssetAdded(const struct FAssetData& AssetData);
	void OnAssetRemoved(const struct FAssetData& AssetData);
	void InvalidateCache();
	void OnReloadComplete(EReloadCompleteReason Reason);

	static void AddUnknownClass(const FPDMissionNodeData& ClassData);
	static bool IsClassKnown(const FPDMissionNodeData& ClassData);
	static FOnPackageListUpdated OnPackageListUpdated;

	static int32 GetObservedBlueprintClassCount(UClass* BaseNativeClass);
	static void AddObservedBlueprintClasses(UClass* BaseNativeClass);
	void UpdateAvailableBlueprintClasses();

	/** Adds a single class to the list of hidden classes */
	void AddForcedHiddenClass(UClass* Class);

	/** Overrides all previously set hidden classes */
	void SetForcedHiddenClasses(const TSet<UClass*>& Classes);

	void SetGatherBlueprints(bool bGather);

private:

	UClass* RootNodeClass;
	TSharedPtr<FPDMissionDataNode> RootNode;
	static TArray<FName> UnknownPackages;
	static TMap<UClass*, int32> BlueprintClassCount;
	TSet<UClass*> ForcedHiddenClasses;
	bool bGatherBlueprints = true;

	TSharedPtr<FPDMissionDataNode> CreateClassDataNode(const struct FAssetData& AssetData);
	TSharedPtr<FPDMissionDataNode> FindBaseClassNode(TSharedPtr<FPDMissionDataNode> Node, const FString& ClassName);
	void FindAllSubClasses(TSharedPtr<FPDMissionDataNode> Node, TArray<FPDMissionNodeData>& AvailableClasses);

	void BuildClassGraph();
	void AddClassGraphChildren(TSharedPtr<FPDMissionDataNode> Node, TArray<TSharedPtr<FPDMissionDataNode> >& NodeList);

	bool IsHidingClass(UClass* Class);
	bool IsHidingParentClass(UClass* Class);
	bool IsPackageSaved(FName PackageName);
};

struct FPDMissionEditorTabs
{
	// Tab identifiers
	static const FName GraphDetailsID;
	static const FName SearchID;

	// Document tab identifiers
	static const FName GraphEditorID;
	static const FName TreeEditorID;
};

USTRUCT()
struct PDMISSIONEDITOR_API FPDMissionGraph_NodeData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Mission Subsystem", Meta = (RowType="/Script/PDMissionCore.PDMissionRow"))
	FDataTableRowHandle DataTarget;
	
	UPROPERTY()
	TArray<FEditedDocumentInfo> LastEditedDocuments;
};


class PDMISSIONEDITOR_API FPDMissionDebuggerHandler : public TSharedFromThis<FPDMissionDebuggerHandler>
{
public:
	// Debugging
	/** Setup commands */
	void BindDebuggerToolbarCommands();
	
	/** Refresh the debugger's display */
	void RefreshDebugger();
	
	FText HandleGetDebugKeyValue(const FName& InKeyName, bool bUseCurrentState) const;
	float HandleGetDebugTimeStamp(bool bUseCurrentState) const;
	
	void OnEnableBreakpoint();
	void OnToggleBreakpoint();
	void OnDisableBreakpoint();
	void OnAddBreakpoint();
	void OnRemoveBreakpoint();
	void SearchMissionDatabase();
	
	void JumpToNode(const UEdGraphNode* Node);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	void UpdateToolbar();
	
	bool CanEnableBreakpoint() const;
	bool CanToggleBreakpoint() const;
	bool CanDisableBreakpoint() const;
	bool CanAddBreakpoint() const;
	bool CanRemoveBreakpoint() const;
	bool CanSearchMissionDatabase() const;
	
	bool IsPropertyEditable() const;
	bool IsDebuggerReady() const;
	bool IsDebuggerPaused() const;
	
	TSharedRef<class SWidget> OnGetDebuggerActorsMenu();
	FText GetDebuggerActorDesc() const;
};
