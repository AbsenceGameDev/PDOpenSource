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
#include "..\Public\MissionGraph\FPDMissionEditor.h"
#include "Engine/CompositeDataTable.h"
#include "MissionGraph/PDMissionGraphNode.h"

#define LOCTEXT_NAMESPACE "MissionGraph"


// Tab identifiers
const FName FPDMissionEditorTabs::GraphDetailsID(TEXT("MissionEditor_Properties"));
const FName FPDMissionEditorTabs::SearchID(TEXT("MissionEditor_Search"));
const FName FPDMissionEditorTabs::TreeEditorID(TEXT("MissionEditor_Tree"));

// Document tab identifiers
const FName FPDMissionEditorTabs::GraphEditorID(TEXT("Document"));


const FName FPDMissionGraphTypes::PinCategory_MultipleNodes("MultipleNodes");
const FName FPDMissionGraphTypes::PinCategory_SingleComposite("SingleComposite");
const FName FPDMissionGraphTypes::PinCategory_SingleTask("SingleTask");
const FName FPDMissionGraphTypes::PinCategory_SingleNode("SingleNode");


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

	const UStruct* MyClass = Struct.Get();
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
	if (RetStruct == nullptr && GeneratedPackage.Len())
	{
		GWarn->BeginSlowTask(LOCTEXT("LoadPackage", "Loading Package..."), true);

		UPackage* Package = LoadPackage(nullptr, *GeneratedPackage, LOAD_NoRedirects);
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