// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "MissionGraphTypes.generated.h"

USTRUCT()
struct PDMISSIONEDITOR_API FMissionNodeClassData
{
	GENERATED_USTRUCT_BODY()

	FMissionNodeClassData(): bIsHidden(0), bHideParent(0) {}
	FMissionNodeClassData(UClass* InClass, const FString& InDeprecatedMessage);
	FMissionNodeClassData(const FTopLevelAssetPath& InGeneratedClassPath, UClass* InClass);
	FMissionNodeClassData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InClass);

	FString ToString() const;
	FString GetClassName() const;
	FText GetCategory() const;
	FString GetDisplayName() const;
	FText GetTooltip() const;
	UClass* GetClass(bool bSilent = false);
	bool IsAbstract() const;

	FORCEINLINE bool IsBlueprint() const { return AssetName.Len() > 0; }
	FORCEINLINE bool IsDeprecated() const { return DeprecatedMessage.Len() > 0; }
	FORCEINLINE FString GetDeprecatedMessage() const { return DeprecatedMessage; }
	FORCEINLINE FString GetPackageName() const { return GeneratedClassPackage; }

	/** set when child class masked this one out (e.g. always use game specific class instead of engine one) */
	uint32 bIsHidden : 1;

	/** set when class wants to hide parent class from selection (just one class up hierarchy) */
	uint32 bHideParent : 1;

private:

	/** pointer to uclass */
	TWeakObjectPtr<UClass> Class;

	/** path to class if it's not loaded yet */
	UPROPERTY()
	FString AssetName;
	
	UPROPERTY()
	FString GeneratedClassPackage;

	/** resolved name of class from asset data */
	UPROPERTY()
	FString ClassName;

	/** User-defined category for this class */
	UPROPERTY()
	FText Category;

	/** message for deprecated class */
	FString DeprecatedMessage;
};

struct PDMISSIONEDITOR_API FMissionNodeClassNode
{
	FMissionNodeClassData Data;
	FString ParentClassName;

	TSharedPtr<FMissionNodeClassNode> ParentNode;
	TArray<TSharedPtr<FMissionNodeClassNode> > SubNodes;

	void AddUniqueSubNode(TSharedPtr<FMissionNodeClassNode> SubNode);
};

struct PDMISSIONEDITOR_API FMissionNodeClassHelper
{
	DECLARE_MULTICAST_DELEGATE(FOnPackageListUpdated);

	FMissionNodeClassHelper(UClass* InRootClass);
	~FMissionNodeClassHelper();

	void GatherClasses(const UClass* BaseClass, TArray<FMissionNodeClassData>& AvailableClasses);
	static FString GetDeprecationMessage(const UClass* Class);

	void OnAssetAdded(const struct FAssetData& AssetData);
	void OnAssetRemoved(const struct FAssetData& AssetData);
	void InvalidateCache();
	void OnReloadComplete(EReloadCompleteReason Reason);

	static void AddUnknownClass(const FMissionNodeClassData& ClassData);
	static bool IsClassKnown(const FMissionNodeClassData& ClassData);
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
	TSharedPtr<FMissionNodeClassNode> RootNode;
	static TArray<FName> UnknownPackages;
	static TMap<UClass*, int32> BlueprintClassCount;
	TSet<UClass*> ForcedHiddenClasses;
	bool bGatherBlueprints = true;

	TSharedPtr<FMissionNodeClassNode> CreateClassDataNode(const struct FAssetData& AssetData);
	TSharedPtr<FMissionNodeClassNode> FindBaseClassNode(TSharedPtr<FMissionNodeClassNode> Node, const FString& ClassName);
	void FindAllSubClasses(TSharedPtr<FMissionNodeClassNode> Node, TArray<FMissionNodeClassData>& AvailableClasses);

	void BuildClassGraph();
	void AddClassGraphChildren(TSharedPtr<FMissionNodeClassNode> Node, TArray<TSharedPtr<FMissionNodeClassNode> >& NodeList);

	bool IsHidingClass(UClass* Class);
	bool IsHidingParentClass(UClass* Class);
	bool IsPackageSaved(FName PackageName);
};


struct PDMISSIONEDITOR_API FPDMissionDebuggerHandler
{
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
