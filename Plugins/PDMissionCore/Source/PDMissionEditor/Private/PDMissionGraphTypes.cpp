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

#include "PDMissionGraphTypes.h"

#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Misc/FeedbackContext.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Blueprint.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "ObjectEditorUtils.h"
#include "Logging/MessageLog.h"
#include "AssetRegistry/ARFilter.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "MissionGraph"


// Tab identifiers
const FName FPDMissionEditorTabs::GraphDetailsID(TEXT("MissionEditor_Properties"));
const FName FPDMissionEditorTabs::SearchID(TEXT("MissionEditor_Search"));
const FName FPDMissionEditorTabs::TreeEditorID(TEXT("MissionEditor_Tree"));

// Document tab identifiers
const FName FPDMissionEditorTabs::GraphEditorID(TEXT("Document"));


FPDMissionNodeData::FPDMissionNodeData(UStruct* InStruct, const FString& InDeprecatedMessage) :
	bIsHidden(0),
	bHideParent(0),
	Struct(InStruct),
	DeprecatedMessage(InDeprecatedMessage)
{
	Category = GetCategory();
	if (InStruct) { ClassName = InStruct->GetName(); }
}

FPDMissionNodeData::FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UStruct* InStruct) :
	bIsHidden(0),
	bHideParent(0),
	Struct(InStruct),
	AssetName(InGeneratedClassPath.GetAssetName().ToString()),
	GeneratedPackage(InGeneratedClassPath.GetPackageName().ToString()),
	ClassName(InGeneratedClassPath.GetAssetName().ToString())
{
	Category = GetCategory();
}

FPDMissionNodeData::FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UStruct* InStruct) :
	bIsHidden(0),
	bHideParent(0),
	Struct(InStruct),
	AssetName(InAssetName),
	GeneratedPackage(InGeneratedClassPackage),
	ClassName(InClassName)
{
	Category = GetCategory();
}

FString FPDMissionNodeData::ToString() const
{
	FString ShortName = GetDisplayName();
	if (!ShortName.IsEmpty())
	{
		return ShortName;
	}

	UStruct* MyClass = Struct.Get();
	if (MyClass)
	{
		FString ClassDesc = MyClass->GetName();

		// if (MyClass->HasAnyFlags(EObjectFlags:: CLASS_CompiledFromBlueprint))
		// {
		// 	return ClassDesc.LeftChop(2);
		// }

		const int32 ShortNameIdx = ClassDesc.Find(TEXT("_"), ESearchCase::CaseSensitive);
		if (ShortNameIdx != INDEX_NONE)
		{
			ClassDesc.MidInline(ShortNameIdx + 1, MAX_int32, EAllowShrinking::No);
		}

		return ClassDesc;
	}

	return AssetName;
}

FString FPDMissionNodeData::GetDataEntryName() const
{
	return Struct.IsValid() ? Struct->GetName() : ClassName;
}

FString FPDMissionNodeData::GetDisplayName() const
{
	return Struct.IsValid() ? Struct->GetMetaData(TEXT("DisplayName")) : FString();
}

FText FPDMissionNodeData::GetTooltip() const
{
	return Struct.IsValid() ? Struct->GetToolTipText() : FText::GetEmpty();
}

FText FPDMissionNodeData::GetCategory() const
{
	return Struct.IsValid() ? FObjectEditorUtils::GetCategoryText(Struct.Get()) : Category;
}


UStruct* FPDMissionNodeData::GetStruct(bool bSilent)
{
	UStruct* RetStruct = Struct.Get();
	if (RetStruct == NULL && GeneratedPackage.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(NULL, *GeneratedPackage, LOAD_NoRedirects);
		if (Package)
		{
			Package->FullyLoad();

			RetStruct = FindObject<UClass>(Package, *ClassName);

			GWarn->EndSlowTask();
			Struct = RetStruct;
		}
		else
		{
			GWarn->EndSlowTask();

			if (!bSilent)
			{
				FMessageLog EditorErrors("EditorErrors");
				EditorErrors.Error(LOCTEXT("PackageLoadFail", "Package Load Failed"));
				EditorErrors.Info(FText::FromString(GeneratedPackage));
				EditorErrors.Notify(LOCTEXT("PackageLoadFail", "Package Load Failed"));
			}
		}
	}

	return RetStruct;
}

//////////////////////////////////////////////////////////////////////////
TArray<FName> FPDMissionDataNodeHelper::UnknownPackages;
TMap<UClass*, int32> FPDMissionDataNodeHelper::BlueprintClassCount;
FPDMissionDataNodeHelper::FOnPackageListUpdated FPDMissionDataNodeHelper::OnPackageListUpdated;

FPDMissionDataNodeHelper::FPDMissionDataNodeHelper(UClass* InRootClass)
{
	RootNodeClass = InRootClass;

	// Register with the Asset Registry to be informed when it is done loading up files.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	AssetRegistryModule.Get().OnFilesLoaded().AddRaw(this, &FPDMissionDataNodeHelper::InvalidateCache);
	AssetRegistryModule.Get().OnAssetAdded().AddRaw(this, &FPDMissionDataNodeHelper::OnAssetAdded);
	AssetRegistryModule.Get().OnAssetRemoved().AddRaw(this, &FPDMissionDataNodeHelper::OnAssetRemoved);

	// Register to have Populate called when doing a Reload.
	FCoreUObjectDelegates::ReloadCompleteDelegate.AddRaw(this, &FPDMissionDataNodeHelper::OnReloadComplete);

	// Register to have Populate called when a Blueprint is compiled.
	GEditor->OnBlueprintCompiled().AddRaw(this, &FPDMissionDataNodeHelper::InvalidateCache);
	GEditor->OnClassPackageLoadedOrUnloaded().AddRaw(this, &FPDMissionDataNodeHelper::InvalidateCache);

	UpdateAvailableBlueprintClasses();
}

FPDMissionDataNodeHelper::~FPDMissionDataNodeHelper()
{
	// Unregister with the Asset Registry to be informed when it is done loading up files.
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		IAssetRegistry* AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).TryGet();
		if (AssetRegistry)
		{
			AssetRegistry->OnFilesLoaded().RemoveAll(this);
			AssetRegistry->OnAssetAdded().RemoveAll(this);
			AssetRegistry->OnAssetRemoved().RemoveAll(this);
		}

		// Unregister to have Populate called when doing a Reload.
		FCoreUObjectDelegates::ReloadCompleteDelegate.RemoveAll(this);

		// Unregister to have Populate called when a Blueprint is compiled.
		if (UObjectInitialized())
		{
			// GEditor can't have been destructed before we call this or we'll crash.
			GEditor->OnBlueprintCompiled().RemoveAll(this);
			GEditor->OnClassPackageLoadedOrUnloaded().RemoveAll(this);
		}
	}
}

void FPDMissionDataNode::AddUniqueSubNode(TSharedPtr<FPDMissionDataNode> SubNode)
{
	for (int32 Idx = 0; Idx < SubNodes.Num(); Idx++)
	{
		if (SubNode->Data.GetDataEntryName() == SubNodes[Idx]->Data.GetDataEntryName())
		{
			return;
		}
	}

	SubNodes.Add(SubNode);
}

void FPDMissionDataNodeHelper::GatherClasses(const UClass* BaseClass, TArray<FPDMissionNodeData>& AvailableClasses)
{
	const FString BaseClassName = BaseClass->GetName();
	if (!RootNode.IsValid())
	{
		BuildClassGraph();
	}

	TSharedPtr<FPDMissionDataNode> BaseNode = FindBaseClassNode(RootNode, BaseClassName);
	FindAllSubClasses(BaseNode, AvailableClasses);
}

FString FPDMissionDataNodeHelper::GetDeprecationMessage(const UClass* Class)
{
	static FName MetaDeprecated = TEXT("DeprecatedNode");
	static FName MetaDeprecatedMessage = TEXT("DeprecationMessage");
	FString DefDeprecatedMessage("Please remove it!");
	FString DeprecatedPrefix("DEPRECATED");
	FString DeprecatedMessage;

	if (Class && Class->HasAnyClassFlags(CLASS_Native) && Class->HasMetaData(MetaDeprecated))
	{
		DeprecatedMessage = DeprecatedPrefix + TEXT(": ");
		DeprecatedMessage += Class->HasMetaData(MetaDeprecatedMessage) ? Class->GetMetaData(MetaDeprecatedMessage) : DefDeprecatedMessage;
	}

	return DeprecatedMessage;
}

bool FPDMissionDataNodeHelper::IsClassKnown(const FPDMissionNodeData& ClassData)
{
	return !ClassData.IsBlueprint() || !UnknownPackages.Contains(*ClassData.GetPackageName());
}

void FPDMissionDataNodeHelper::AddUnknownClass(const FPDMissionNodeData& ClassData)
{
	if (ClassData.IsBlueprint())
	{
		UnknownPackages.AddUnique(*ClassData.GetPackageName());
	}
}

bool FPDMissionDataNodeHelper::IsHidingParentClass(UClass* Class)
{
	static FName MetaHideParent = TEXT("HideParentNode");
	return Class && Class->HasAnyClassFlags(CLASS_Native) && Class->HasMetaData(MetaHideParent);
}

bool FPDMissionDataNodeHelper::IsHidingClass(UClass* Class)
{
	static FName MetaHideInEditor = TEXT("HiddenNode");

	return 
		Class && 
		((Class->HasAnyClassFlags(CLASS_Native) && Class->HasMetaData(MetaHideInEditor))
		|| ForcedHiddenClasses.Contains(Class));
}

bool FPDMissionDataNodeHelper::IsPackageSaved(FName PackageName)
{
	const bool bFound = FPackageName::SearchForPackageOnDisk(PackageName.ToString());
	return bFound;
}

void FPDMissionDataNodeHelper::OnAssetAdded(const struct FAssetData& AssetData)
{
	TSharedPtr<FPDMissionDataNode> Node = CreateClassDataNode(AssetData);

	TSharedPtr<FPDMissionDataNode> ParentNode;
	if (Node.IsValid())
	{
		ParentNode = FindBaseClassNode(RootNode, Node->ParentClassName);

		if (!IsPackageSaved(AssetData.PackageName))
		{
			UnknownPackages.AddUnique(AssetData.PackageName);
		}
		else
		{
			const int32 PrevListCount = UnknownPackages.Num();
			UnknownPackages.RemoveSingleSwap(AssetData.PackageName);

			if (UnknownPackages.Num() != PrevListCount)
			{
				OnPackageListUpdated.Broadcast();
			}
		}
	}

	if (ParentNode.IsValid())
	{
		ParentNode->AddUniqueSubNode(Node);
		Node->ParentNode = ParentNode;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if (!AssetRegistryModule.Get().IsLoadingAssets())
	{
		UpdateAvailableBlueprintClasses();
	}
}

void FPDMissionDataNodeHelper::OnAssetRemoved(const struct FAssetData& AssetData)
{
	FString AssetClassName;
	if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, AssetClassName))
	{
		ConstructorHelpers::StripObjectClass(AssetClassName);
		AssetClassName = FPackageName::ObjectPathToObjectName(AssetClassName);

		TSharedPtr<FPDMissionDataNode> Node = FindBaseClassNode(RootNode, AssetClassName);
		if (Node.IsValid() && Node->ParentNode.IsValid())
		{
			Node->ParentNode->SubNodes.RemoveSingleSwap(Node);
		}
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if (!AssetRegistryModule.Get().IsLoadingAssets())
	{
		UpdateAvailableBlueprintClasses();
	}
}

void FPDMissionDataNodeHelper::InvalidateCache()
{
	RootNode.Reset();

	UpdateAvailableBlueprintClasses();
}

void FPDMissionDataNodeHelper::OnReloadComplete(EReloadCompleteReason Reason)
{
	InvalidateCache();
}

TSharedPtr<FPDMissionDataNode> FPDMissionDataNodeHelper::CreateClassDataNode(const struct FAssetData& AssetData)
{
	TSharedPtr<FPDMissionDataNode> Node;

	FString AssetClassName;
	FString AssetParentClassName;
	if (AssetData.GetTagValue(FBlueprintTags::GeneratedClassPath, AssetClassName) && AssetData.GetTagValue(FBlueprintTags::ParentClassPath, AssetParentClassName))
	{
		UObject* Outer1(NULL);
		ResolveName(Outer1, AssetClassName, false, false);

		UObject* Outer2(NULL);
		ResolveName(Outer2, AssetParentClassName, false, false);

		Node = MakeShareable(new FPDMissionDataNode);
		Node->ParentClassName = AssetParentClassName;

		FPDMissionNodeData NewData(AssetData.AssetName.ToString(), AssetData.PackageName.ToString(), AssetClassName, nullptr);
		Node->Data = NewData;
	}

	return Node;
}

TSharedPtr<FPDMissionDataNode> FPDMissionDataNodeHelper::FindBaseClassNode(TSharedPtr<FPDMissionDataNode> Node, const FString& ClassName)
{
	TSharedPtr<FPDMissionDataNode> RetNode;
	if (Node.IsValid())
	{
		if (Node->Data.GetDataEntryName() == ClassName)
		{
			return Node;
		}

		for (int32 i = 0; i < Node->SubNodes.Num(); i++)
		{
			RetNode = FindBaseClassNode(Node->SubNodes[i], ClassName);
			if (RetNode.IsValid())
			{
				break;
			}
		}
	}

	return RetNode;
}

void FPDMissionDataNodeHelper::FindAllSubClasses(TSharedPtr<FPDMissionDataNode> Node, TArray<FPDMissionNodeData>& AvailableClasses)
{
	if (Node.IsValid())
	{
		if (!!Node->Data.IsDeprecated() && !Node->Data.bIsHidden)
		{
			AvailableClasses.Add(Node->Data);
		}

		for (int32 i = 0; i < Node->SubNodes.Num(); i++)
		{
			FindAllSubClasses(Node->SubNodes[i], AvailableClasses);
		}
	}
}

void FPDMissionDataNodeHelper::BuildClassGraph()
{
	TArray<TSharedPtr<FPDMissionDataNode> > NodeList;
	TArray<UClass*> HideParentList;
	RootNode.Reset();

	// gather all native classes
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* TestClass = *It;
		if (TestClass->HasAnyClassFlags(CLASS_Native) && TestClass->IsChildOf(RootNodeClass))
		{
			TSharedPtr<FPDMissionDataNode> NewNode = MakeShareable(new FPDMissionDataNode);
			NewNode->ParentClassName = TestClass->GetSuperClass()->GetName();

			FString DeprecatedMessage = GetDeprecationMessage(TestClass);
			FPDMissionNodeData NewData(TestClass, DeprecatedMessage);

			NewData.bHideParent = IsHidingParentClass(TestClass);
			if (NewData.bHideParent)
			{
				HideParentList.Add(TestClass->GetSuperClass());
			}

			NewData.bIsHidden = IsHidingClass(TestClass);

			NewNode->Data = NewData;

			if (TestClass == RootNodeClass)
			{
				RootNode = NewNode;
			}

			NodeList.Add(NewNode);
		}
	}

	// find all hidden parent classes
	for (int32 i = 0; i < NodeList.Num(); i++)
	{
		TSharedPtr<FPDMissionDataNode> TestNode = NodeList[i];
		if (HideParentList.Contains(TestNode->Data.GetStruct()))
		{
			TestNode->Data.bIsHidden = true;
		}
	}

	// gather all blueprints
	if (bGatherBlueprints)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> BlueprintList;

		FARFilter Filter;
		Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		AssetRegistryModule.Get().GetAssets(Filter, BlueprintList, false);

		for (int32 i = 0; i < BlueprintList.Num(); i++)
		{
			TSharedPtr<FPDMissionDataNode> NewNode = CreateClassDataNode(BlueprintList[i]);
			NodeList.Add(NewNode);
		}
	}

	// build class tree
	AddClassGraphChildren(RootNode, NodeList);
}

void FPDMissionDataNodeHelper::AddClassGraphChildren(TSharedPtr<FPDMissionDataNode> Node, TArray<TSharedPtr<FPDMissionDataNode> >& NodeList)
{
	if (!Node.IsValid())
	{
		return;
	}

	const FString NodeClassName = Node->Data.GetDataEntryName();
	for (int32 i = NodeList.Num() - 1; i >= 0; i--)
	{
		if (NodeList[i]->ParentClassName == NodeClassName)
		{
			TSharedPtr<FPDMissionDataNode> MatchingNode = NodeList[i];
			NodeList.RemoveAt(i);

			MatchingNode->ParentNode = Node;
			Node->SubNodes.Add(MatchingNode);

			AddClassGraphChildren(MatchingNode, NodeList);
		}
	}
}

int32 FPDMissionDataNodeHelper::GetObservedBlueprintClassCount(UClass* BaseNativeClass)
{
	return BlueprintClassCount.FindRef(BaseNativeClass);
}

void FPDMissionDataNodeHelper::AddObservedBlueprintClasses(UClass* BaseNativeClass)
{
	BlueprintClassCount.Add(BaseNativeClass, 0);
}

void FPDMissionDataNodeHelper::UpdateAvailableBlueprintClasses()
{
	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		IAssetRegistry& AssetRegistry = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		const bool bSearchSubClasses = true;

		TArray<FTopLevelAssetPath> ClassNames;
		TSet<FTopLevelAssetPath> DerivedClassNames;

		for (TMap<UClass*, int32>::TIterator It(BlueprintClassCount); It; ++It)
		{
			ClassNames.Reset();
			ClassNames.Add(It.Key()->GetClassPathName());

			DerivedClassNames.Empty(DerivedClassNames.Num());
			AssetRegistry.GetDerivedClassNames(ClassNames, TSet<FTopLevelAssetPath>(), DerivedClassNames);

			int32& Count = It.Value();
			Count = DerivedClassNames.Num();
		}
	}
}

void FPDMissionDataNodeHelper::AddForcedHiddenClass(UClass* Class)
{
	if (Class)
	{
		ForcedHiddenClasses.Add(Class);
	}
}

void FPDMissionDataNodeHelper::SetForcedHiddenClasses(const TSet<UClass*>& Classes)
{
	ForcedHiddenClasses = Classes;
}

void FPDMissionDataNodeHelper::SetGatherBlueprints(const bool bGather)
{
	bGatherBlueprints = bGather;
}


void FPDMissionDebuggerHandler::BindDebuggerToolbarCommands()
{
	// @todo
}

void FPDMissionDebuggerHandler::RefreshDebugger()
{
	// @todo
}

FText FPDMissionDebuggerHandler::HandleGetDebugKeyValue(const FName& InKeyName, bool bUseCurrentState) const
{
	// @todo
	return FText::GetEmpty();
}

float FPDMissionDebuggerHandler::HandleGetDebugTimeStamp(bool bUseCurrentState) const
{
	// @todo
	return 0.0f;
}

void FPDMissionDebuggerHandler::OnEnableBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnToggleBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnDisableBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnAddBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::OnRemoveBreakpoint()
{
	// @todo
}

void FPDMissionDebuggerHandler::SearchMissionDatabase()
{
	// @todo
}

void FPDMissionDebuggerHandler::JumpToNode(const UEdGraphNode* Node)
{
	// @todo
}
void FPDMissionDebuggerHandler::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	// @todo
}

void FPDMissionDebuggerHandler::UpdateToolbar()
{
	// @todo
}


bool FPDMissionDebuggerHandler::CanEnableBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanToggleBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanDisableBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanAddBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanRemoveBreakpoint() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::CanSearchMissionDatabase() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsPropertyEditable() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsDebuggerReady() const
{
	// @todo
	return false;
}

bool FPDMissionDebuggerHandler::IsDebuggerPaused() const
{
	// @todo
	return false;
}

TSharedRef<SWidget> FPDMissionDebuggerHandler::OnGetDebuggerActorsMenu()
{
	// @todo
	return SNew(SGraphEditor);
}

FText FPDMissionDebuggerHandler::GetDebuggerActorDesc() const
{
	// @todo
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
