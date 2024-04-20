/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

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
#include "MissionGraph/FPDMissionEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/CompositeDataTable.h"
#include "MissionGraph/PDMissionGraphNode.h"

#define LOCTEXT_NAMESPACE "MissionGraph"


// Tab identifiers
const FName FPDMissionEditorTabs::GraphDetailsID(TEXT("MissionEditor_Properties"));
const FName FPDMissionEditorTabs::SearchID(TEXT("MissionEditor_Search"));
const FName FPDMissionEditorTabs::TreeEditorID(TEXT("MissionEditor_Tree"));

// Document tab identifiers
const FName FPDMissionEditorTabs::GraphEditorID(TEXT("Document"));

const FName FPDMissionGraphTypes::PinCategory_Name("Name");
const FName FPDMissionGraphTypes::PinCategory_String("String");
const FName FPDMissionGraphTypes::PinCategory_Text("Text");

const FName FPDMissionGraphTypes::PinCategory_MissionRow("MissionRow");
const FName FPDMissionGraphTypes::PinCategory_MissionDataRef("MissionDataRef");
const FName FPDMissionGraphTypes::PinCategory_MissionRowKeyBuilder("MissionRowKeyBuilder");

const FName FPDMissionGraphTypes::PinCategory_MultipleNodes("MultipleNodes");
const FName FPDMissionGraphTypes::PinCategory_SingleComposite("SingleComposite");
const FName FPDMissionGraphTypes::PinCategory_SingleTask("SingleTask");
const FName FPDMissionGraphTypes::PinCategory_SingleNode("SingleNode");


FPDMissionNodeData::FPDMissionNodeData(UClass* InClass, const FString& InDeprecatedMessage) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass),
	DeprecatedMessage(InDeprecatedMessage)
{
	Category = GetCategory();
	if (InClass) { ClassName = InClass->GetName(); }
}

FPDMissionNodeData::FPDMissionNodeData(const FTopLevelAssetPath& InGeneratedClassPath, UClass* InClass) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass),
	AssetName(InGeneratedClassPath.GetAssetName().ToString()),
	GeneratedPackage(InGeneratedClassPath.GetPackageName().ToString()),
	ClassName(InGeneratedClassPath.GetAssetName().ToString())
{
	Category = GetCategory();
}

FPDMissionNodeData::FPDMissionNodeData(const FString& InAssetName, const FString& InGeneratedClassPackage, const FString& InClassName, UClass* InClass) :
	bIsHidden(0),
	bHideParent(0),
	Class(InClass),
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

	const UClass* MyClass = Class.Get();
	if (MyClass)
	{
		FString ClassDesc = MyClass->GetName();
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
	return Class.IsValid() ? Class->GetName() : ClassName;
}

FString FPDMissionNodeData::GetDisplayName() const
{
	return Class.IsValid() ? Class->GetMetaData(TEXT("DisplayName")) : FString();
}

FText FPDMissionNodeData::GetTooltip() const
{
	return Class.IsValid() ? Class->GetToolTipText() : FText::GetEmpty();
}

FText FPDMissionNodeData::GetCategory() const
{
	return Class.IsValid() ? FObjectEditorUtils::GetCategoryText(Class.Get()) : Category;
}


UClass* FPDMissionNodeData::GetClass(bool bSilent)
{
	UClass* RetClass = Class.Get();
	if (RetClass == nullptr && GeneratedPackage.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(nullptr, *GeneratedPackage, LOAD_NoRedirects);
		if (Package)
		{
			Package->FullyLoad();

			RetClass = FindObject<UClass>(Package, *ClassName);

			GWarn->EndSlowTask();
			Class = RetClass;
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

	return RetClass;
}

bool FPDMissionNodeData::operator==(const FPDMissionNodeData& Other) const
{
	return
		bHideParent      == Other.bHideParent      &&
		bIsHidden        == Other.bIsHidden        &&
		Class.Get()      == Other.Class.Get()      &&
		AssetName        == Other.AssetName        &&
		GeneratedPackage == Other.GeneratedPackage &&
		ClassName        == Other.ClassName        &&
		Category.EqualTo(Other.Category)           &&
		DeprecatedMessage == Other.DeprecatedMessage;
}

bool FPDMissionNodeData::operator!=(const FPDMissionNodeData& Other) const
{
	return (Other == *this) == false;
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

void FPDMissionDebuggerHandler::OnSearchMissionDatabase()
{
	// @todo
}

FReply FPDMissionDebuggerHandler::JumpToNode(const UEdGraphNode* Node)
{
	// @todo

	return FReply::Handled(); 
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


FPDMissionGraphConnectionDrawingPolicy::FPDMissionGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements)
	, GraphObj(InGraphObj)
{
}

void FPDMissionGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	Params.AssociatedPin1 = OutputPin;
	Params.AssociatedPin2 = InputPin;
	Params.WireThickness = 1.5f;

	Params.WireColor = MissionTreeColors::Connection::Default;

	UPDMissionGraphNode* FromNode = OutputPin ? Cast<UPDMissionGraphNode>(OutputPin->GetOwningNode()) : nullptr;
	UPDMissionGraphNode* ToNode = InputPin ? Cast<UPDMissionGraphNode>(InputPin->GetOwningNode()) : nullptr;
	if (ToNode && FromNode)
	{
#ifdef TODO
		// @todo 
#endif
	}

	const bool bDeemphasizeUnhoveredPins = HoveredPins.Num() > 0;
	if (bDeemphasizeUnhoveredPins)
	{
		ApplyHoverDeemphasis(OutputPin, InputPin, /*inout*/ Params.WireThickness, /*inout*/ Params.WireColor);
	}
}

void FPDMissionGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	// Build an acceleration structure to quickly find geometry for the nodes
	NodeWidgetMap.Empty();
	for (int32 NodeIndex = 0; NodeIndex < ArrangedNodes.Num(); ++NodeIndex)
	{
		FArrangedWidget& CurWidget = ArrangedNodes[NodeIndex];
		const TSharedRef<SGraphNode> ChildNode = StaticCastSharedRef<SGraphNode>(CurWidget.Widget);
		NodeWidgetMap.Add(ChildNode->GetNodeObj(), NodeIndex);
	}

	// Now draw
	FConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
}

void FPDMissionGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	FConnectionParams Params;
	DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

	if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, EndPoint), EndPoint, Params);
	}
	else
	{
		DrawSplineWithArrow(FGeometryHelper::FindClosestPointOnGeom(PinGeometry, StartPoint), StartPoint, Params);
	}
}

void FPDMissionGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	// bUserFlag1 indicates that we need to reverse the direction of connection (used by debugger)
	const FVector2D& P0 = Params.bUserFlag1 ? EndAnchorPoint : StartAnchorPoint;
	const FVector2D& P1 = Params.bUserFlag1 ? StartAnchorPoint : EndAnchorPoint;

	Internal_DrawLineWithArrow(P0, P1, Params);
}

void FPDMissionGraphConnectionDrawingPolicy::Internal_DrawLineWithArrow(const FVector2D& StartAnchorPoint, const FVector2D& EndAnchorPoint, const FConnectionParams& Params)
{
	//@TODO: Should this be scaled by zoom factor?
	constexpr float LineSeparationAmount = 4.5f;

	const FVector2D DeltaPos = EndAnchorPoint - StartAnchorPoint;
	const FVector2D UnitDelta = DeltaPos.GetSafeNormal();
	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	// Come up with the final start/end points
	const FVector2D DirectionBias = Normal * LineSeparationAmount;
	const FVector2D LengthBias = ArrowRadius.X * UnitDelta;
	const FVector2D StartPoint = StartAnchorPoint + DirectionBias + LengthBias;
	const FVector2D EndPoint = EndAnchorPoint + DirectionBias - LengthBias;

	// Draw a line/spline
	DrawConnection(WireLayerID, StartPoint, EndPoint, Params);

	// Draw the arrow
	const FVector2D ArrowDrawPos = EndPoint - ArrowRadius;
	const float AngleInRadians = FMath::Atan2(DeltaPos.Y, DeltaPos.X);

	FSlateDrawElement::MakeRotatedBox(
		DrawElementsList,
		ArrowLayerID,
		FPaintGeometry(ArrowDrawPos, ArrowImage->ImageSize * ZoomFactor, ZoomFactor),
		ArrowImage,
		ESlateDrawEffect::None,
		AngleInRadians,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		Params.WireColor
		);
}

void FPDMissionGraphConnectionDrawingPolicy::DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params)
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	DrawSplineWithArrow(StartAnchorPoint, EndAnchorPoint, Params);
}

FVector2D FPDMissionGraphConnectionDrawingPolicy::ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const
{
	const FVector2D Delta = End - Start;
	const FVector2D NormDelta = Delta.GetSafeNormal();

	return NormDelta;
}


#define LOCTEXT_NAMESPACE "AssetTypeActions"

uint32 FAssetTypeActions_MissionEditor::GetCategories()
{ 
	return EAssetTypeCategories::Gameplay;
}

void FAssetTypeActions_MissionEditor::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (UObject* Object : InObjects)
	{
		UDataTable* Bank = Cast<UDataTable>(Object);
		if (Bank == nullptr) { continue; }

		TArray<FName> SlowRandomKeyList{};
		Bank->GetRowMap().GetKeys(SlowRandomKeyList);
		FPDMissionNodeHandle ConstructedNodeData;
		ConstructedNodeData.DataTarget.DataTable = Bank;
		ConstructedNodeData.DataTarget.RowName = SlowRandomKeyList.IsEmpty() ? NAME_None : SlowRandomKeyList[0];
		FFPDMissionGraphEditor::CreateMissionEditor(Mode, EditWithinLevelEditor, ConstructedNodeData);
	}
}

UClass* FAssetTypeActions_MissionEditor::GetSupportedClass() const
{ 
	return UPDMissionDataTable::StaticClass();
}


void FAssetTypeActions_MissionEditor::OpenInDefaults(UDataTable* OldBank, UDataTable* NewBank) const
{
	const FString OldTextFilename = DumpAssetToTempFile(OldBank);
	const FString NewTextFilename = DumpAssetToTempFile(NewBank);

	// Get diff program to use
	const FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	// AssetToolsModule.Get().CreateDiffProcess(DiffCommand, OldTextFilename, NewTextFilename); // @todo
}

#undef LOCTEXT_NAMESPACE

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