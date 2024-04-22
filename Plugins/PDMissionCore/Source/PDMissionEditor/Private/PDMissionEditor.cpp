/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

// Mission editor
#include "PDMissionEditor.h"
#include "PDMissionCommon.h"
#include "PDMissionGraphTypes.h"
#include "Mission/FPDMissionEditor.h"
#include "Mission/PDMissionBuilder.h"
#include "Mission/Graph/PDMissionGraph.h"
#include "Mission/Graph/PDMissionGraphSchema.h"
#include "Mission/Slate/PDMissionView.h"
#include "PDMissionEditorStyle.h"
#include "PDMissionEditorCommands.h"

// Engine
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/DataTableFactory.h"
#include "Subsystems/PDMissionSubsystem.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

static const FName PDMissionEditorTabName("PDMissionEditor");

#define LOCTEXT_NAMESPACE "FPDMissionEditorModule"

void FPDMissionEditorModule::StartupModule()
{
	//
	//
	// Register pinfactory for editor
	TSharedPtr<FPDAttributeGraphPinFactory> AttributeGraphPinFactory = MakeShareable(new FPDAttributeGraphPinFactory());
	FEdGraphUtilities::RegisterVisualPinFactory(AttributeGraphPinFactory);

	
	//
	//
	// This Asset action applies a new default editor of assets of type: UPDMissionDataTable
	ItemDataAssetTypeActions.Add(MakeShared<FAssetTypeActions_MissionEditor>());
	IAssetTools& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	for (TSharedPtr<FAssetTypeActions_Base> AssetActionPtr : ItemDataAssetTypeActions)
	{
		AssetToolsModule.RegisterAssetTypeActions(AssetActionPtr.ToSharedRef());
	}

	FPDMissionEditorStyle::Initialize();
	FPDMissionEditorStyle::ReloadTextures();
	FPDMissionEditorCommands::Register();
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPDMissionEditorCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FPDMissionEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPDMissionEditorModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PDMissionEditorTabName, FOnSpawnTab::CreateRaw(this, &FPDMissionEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPDMissionEditorTabTitle", "PDMissionEditor"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FPDMissionEditorModule::ShutdownModule()
{
	if (GraphObj != nullptr && GraphObj->IsValidLowLevelFast())
	{
		GraphObj->RemoveFromRoot();
		GraphObj->BeginDestroy();
	}
	
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& AssetTypeAction : ItemDataAssetTypeActions)
		{
			if (AssetTypeAction.IsValid())
			{
				AssetToolsModule.UnregisterAssetTypeActions(AssetTypeAction.ToSharedRef());
			}
		}
	}
	ItemDataAssetTypeActions.Empty();
	

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FPDMissionEditorStyle::Shutdown();
	FPDMissionEditorCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PDMissionEditorTabName);
}

TSharedRef<SDockTab> FPDMissionEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	if (GEngine == nullptr)
	{
		const FText WidgetText = FText::Format(
			LOCTEXT("WindowWidgetText", "GEngine is invalid: in function: {0} in file: {1}"),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);

		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center) [ SNew(STextBlock).Text(WidgetText) ]
		];		
	}

	UPDMissionSubsystem* MissionSubsystem = UPDMissionStatics::GetMissionSubsystem();
	if (MissionSubsystem == nullptr)
	{
		const FText WidgetText = FText::Format(
			LOCTEXT("WindowWidgetText", "MissionSubsystem is invalid: in function: {0} in file: {1}"),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);

		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center) [ SNew(STextBlock).Text(WidgetText) ]
		];		
	}
	
	TSharedPtr<FFPDMissionGraphEditor> MissionEditor;
	MissionSubsystem->Utility.InitializeMissionSubsystem(); // @test
	const TArray<UDataTable*>& Tables = MissionSubsystem->Utility.GetAllTables();
	for (const UDataTable* Databank : Tables)
	{
		if (Databank == nullptr) { continue; }

		TArray<FName> SlowRandomKeyList{};
		
		Databank->GetRowMap().GetKeys(SlowRandomKeyList);
		
		FPDMissionNodeHandle ConstructedNodeData;
		ConstructedNodeData.DataTarget.DataTable = Databank;
		ConstructedNodeData.DataTarget.RowName = SlowRandomKeyList.IsEmpty() ? NAME_None : SlowRandomKeyList[0];
		MissionEditor = FFPDMissionGraphEditor::CreateMissionEditor(ConstructedNodeData);

		break;
	}

	if (Tables.IsEmpty())
	{
		const FText WidgetText = FText::Format(
			LOCTEXT("WindowWidgetText", "UPDMissionSubsystem 'MissionTables' array is empty: in function: {0} in file: {1}"),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);

		const FText WidgetTextWarning = FText::Format(
			LOCTEXT("WindowWidgetTextWarning", "Assign tables via config variable. 'TArray<UDataTable*> MissionTables' "),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);		

		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(WidgetText)
			]

			+ SHorizontalBox::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(WidgetTextWarning).ColorAndOpacity(FLinearColor::Yellow)
			]
		];
	}

	const TWeakPtr<FDocumentTabFactory> GraphEditorFactory = MissionEditor != nullptr ? MissionEditor->GetGraphEditorTabFactoryPtr() : nullptr;
	if (GraphEditorFactory == nullptr)
	{
		const FText WidgetText = FText::Format(
			LOCTEXT("WindowWidgetText", "MissionEditor or GraphEditorFactory is invalid: in function: {0} in file: {1}"),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);

		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center) [ SNew(STextBlock).Text(WidgetText) ]
		];
	}

	const TWeakPtr<SGraphEditor> FocusedGraphEditor = MissionEditor->GetFocusedGraphPtr();
	if (FocusedGraphEditor.IsValid() && FocusedGraphEditor.Pin()->GetCurrentGraph() != nullptr)
	{
		GraphObj = Cast<UPDMissionGraph>(FocusedGraphEditor.Pin()->GetCurrentGraph());
	}
	else
	{
		GraphObj = FPDMissionBuilder::CreateNewGraph(Tables[0], "MissionEditor");
		// GraphObj =  NewObject<UPDMissionGraph>();
	}
	
	if (GraphObj == nullptr)
	{
		const FText WidgetText = FText::Format(
			LOCTEXT("WindowWidgetText", "Expected UEdGraph is invalid: in function: {0} in file: {1}"),
			FText::FromString(TEXT("FPDMissionEditorModule::OnSpawnPluginTab")),
			FText::FromString(TEXT("PDMissionEditor.cpp"))
			);
	
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)
		[
			SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center) [ SNew(STextBlock).Text(WidgetText) ]
		];
	}

	if (EditingTable == nullptr) // && EditingTable->GetRowMap().Num() > 0)
	{
		EditingTable = GetIntermediaryEditingTable(true);
	}

	// Copy any data if necessary
	if (EditingTable->GetRowMap().Num() >= 0)
	{
		for (const UDataTable* TableToCopy : Tables)
		{
			CopyMissionTable(TableToCopy, true);
		}
	}
	
	// Add schema to the graph and add the graph to the root
	GraphObj->Schema = UPDMissionGraphSchema::StaticClass();
	GraphObj->AddToRoot();

	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
	[
		MissionEditor->CreateGraphEditorWidget(GraphObj)
	];
	

	return MajorTab;
}

void FPDMissionEditorModule::CopyMissionTable(const UDataTable* const TableToCopy, bool bAccumulateTables)
{
	if (TableToCopy == nullptr) { return; }


	TArray<FName> RowNameArray = TableToCopy->GetRowNames();
	TMap<FName, const FPDMissionRow*> OutRows;
	OutRows.Reserve(RowNameArray.Num());
	
	for (const FName& RowName : RowNameArray)
	{
		const FPDMissionRow* Row = TableToCopy->FindRow<FPDMissionRow>(RowName, "");
		if (Row == nullptr) { return; }
		
		OutRows.Emplace(RowName, Row);
	}
	
	if (EditingTable == nullptr)
	{
		EditingTable = ConstructEditingTable();
		EditingTable->AddToRoot(); // Keep GC from destroying it
	}
	else if (EditingTable->IsValidLowLevelFast() == false)
	{
		EditingTable->RemoveFromRoot(); // Allow GC to destroy it
		EditingTable->BeginDestroy();

		EditingTable = ConstructEditingTable(); // reload new pointer to it
		EditingTable->AddToRoot(); // Keep GC from destroying the new pointer
	}
	else if (bAccumulateTables == false)
	{
		EditingTable->EmptyTable(); // clear the associative datatable, might have clutter from last 
	}

	if (OutRows.IsEmpty() == false)
	{
		EditingTable->MarkPackageDirty();
	}	

	// Create/add the associative rows
	for (const TPair<FName, const FPDMissionRow*>& RowNameRowData : OutRows)
	{
		FPDAssociativeMissionEditingRow AssociativeRow;
		AssociativeRow.TargetRow.DataTable = TableToCopy;
		AssociativeRow.TargetRow.RowName = RowNameRowData.Key;
		AssociativeRow.UpdateRowData = *RowNameRowData.Value; // Copy data, so states are properly represented in the graph
		
		EditingTable->AddRow(RowNameRowData.Key, AssociativeRow);
		EditingTable->HandleDataTableChanged(RowNameRowData.Key);
	}

	if (OutRows.IsEmpty() == false)
	{
		// let the package update itself if necessary
		EditingTable->PreEditChange(nullptr);
		EditingTable->PostEditChange();
	}
}

bool FPDMissionEditorModule::RemoveFromEditingTable(const FName& RowNameToRemove)
{
	FPDAssociativeMissionEditingRow* EditingRow = EditingTable->FindRow<FPDAssociativeMissionEditingRow>(RowNameToRemove, "");
	if (EditingRow == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("FPDMissionEditorModule::RemoveFromEditingTable -- Did not find editing row of name: %s"), *RowNameToRemove.ToString());
		return false;
	}

	bEditTableParity = false;
	EditingRow->bMarkForRemoval = true;
	EditingRow->bWasUpdated     = true;
	return true;
}

bool FPDMissionEditorModule::EditRowInEditingTable(const FName& RowNameToRemove, const FPDMissionRow& NewData)
{
	FPDAssociativeMissionEditingRow* EditingRow = EditingTable->FindRow<FPDAssociativeMissionEditingRow>(RowNameToRemove, "");
	if (EditingRow == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("FPDMissionEditorModule::RemoveFromEditingTable -- Did not find editing row of name: %s"), *RowNameToRemove.ToString());
		return false;
	}

	bEditTableParity = false;	
	EditingRow->bMarkForRemoval = false;
	EditingRow->UpdateRowData   = NewData;
	EditingRow->bWasUpdated     = true;
	return true;
}

void FPDMissionEditorModule::AddRowToEditingTable(const FName& RowNameToAdd, const FPDMissionRow& NewData)
{
	FPDAssociativeMissionEditingRow NewRow;
	bEditTableParity = false;	
	NewRow.bMarkForRemoval = false;
	NewRow.UpdateRowData   = NewData;
	NewRow.bWasUpdated     = true; 
	EditingTable->AddRow(RowNameToAdd, NewRow);
}


//
// @todo need to remake this slightly, as the associated row table entries can be targeting different tables,
// @todo cont: so I need to source it from the target and 'TableToSaveIn' will be an optional target if not nullptr
//
void FPDMissionEditorModule::SaveToMissionTable(UDataTable* TableToSaveIn)
{
	if (bEditTableParity == true) { return; } // no changes made, no need to save

	TableToSaveIn->PreEditChange(nullptr);
	const bool bMarkedDirty = TableToSaveIn->MarkPackageDirty();

	TArray<FPDAssociativeMissionEditingRow*> AssociativeRows;
	EditingTable->GetAllRows<FPDAssociativeMissionEditingRow>("", AssociativeRows);

	TArray<FName> AssociativeRowNames = EditingTable->GetRowNames();

	TArray<FName> RowsToRemove;
	TMap<FName, FPDMissionRow> RowsToAdd;
	
	for (const FName& AssociatedRowName : AssociativeRowNames)
	{
		const FPDAssociativeMissionEditingRow* Association = EditingTable->FindRow<FPDAssociativeMissionEditingRow>(AssociatedRowName, "");
		check(Association != nullptr)

		if (Association->bWasUpdated == false) { continue; }

		if (Association->bMarkForRemoval)
		{
			if (Association->TargetRow.RowName != NAME_None)
			{
				RowsToRemove.Emplace(Association->TargetRow.RowName);
			}
			continue;
		}

		const bool bShouldAddNewRow = Association->TargetRow.DataTable == nullptr || Association->TargetRow.RowName == NAME_None;
		if (bShouldAddNewRow)
		{
			RowsToAdd.Emplace(AssociatedRowName, Association->UpdateRowData);
			continue;
		}

		// Update existing rows that we are not removing
		FPDMissionRow* TargetRowPtr = Association->TargetRow.GetRow<FPDMissionRow>("");
		*TargetRowPtr = Association->UpdateRowData;
	}
	
	for (const FName& RemoveName : RowsToRemove)
	{
		TableToSaveIn->RemoveRow(RemoveName);
	}	
	
	for (const TPair<FName, FPDMissionRow>& AddRowPair : RowsToAdd)
	{
		TableToSaveIn->AddRow(AddRowPair.Key, AddRowPair.Value);
	}

	TArray<FName> AddKeys;
	RowsToAdd.GetKeys(AddKeys);
	RowsToRemove.Append(AddKeys);
	FinalizeRowChanges(TableToSaveIn, RowsToRemove);
	
	bEditTableParity = true;
}

bool FPDMissionEditorModule::RemoveFromEditingTable_MarkDirty(const FName& RowNameToRemove)
{
	EditingTable->PreEditChange(nullptr);
	const bool bMarkedDirty         = EditingTable->MarkPackageDirty();
	const bool bMarkedRowForRemoval = RemoveFromEditingTable(RowNameToRemove);
	FinalizeRowChanges(EditingTable, TArray<FName>{RowNameToRemove});
	
	return bMarkedDirty && bMarkedRowForRemoval;
}

bool FPDMissionEditorModule::EditRowInEditingTable_MarkDirty(const FName& RowNameToRemove, const FPDMissionRow& NewData)
{
	EditingTable->PreEditChange(nullptr);
	const bool bMarkedDirty = EditingTable->MarkPackageDirty();
	const bool bEditedRow = EditRowInEditingTable(RowNameToRemove, NewData);
	FinalizeRowChanges(EditingTable, TArray<FName>{RowNameToRemove});
	
	return bMarkedDirty && bEditedRow;
}

bool FPDMissionEditorModule::AddRowToEditingTable_MarkDirty(const FName& RowNameToAdd, const FPDMissionRow& NewData)
{
	EditingTable->PreEditChange(nullptr);
	const bool bMarkedDirty = EditingTable->MarkPackageDirty();
	AddRowToEditingTable(RowNameToAdd, NewData);
	FinalizeRowChanges(EditingTable, TArray<FName>{RowNameToAdd});
	
	return bMarkedDirty;
}

UDataTable* FPDMissionEditorModule::GetIntermediaryEditingTable(const bool bReconstruct)
{
	if (bReconstruct == false && EditingTable != nullptr)
	{
		return EditingTable;
	}
	
	if (EditingTable == nullptr)
	{
		EditingTable = ConstructEditingTable();
		EditingTable->AddToRoot(); // Keep GC from destroying it
	}
	else if (EditingTable->IsValidLowLevelFast() == false)
	{
		EditingTable->RemoveFromRoot(); // Allow GC to destroy it
		EditingTable->BeginDestroy();
		
		EditingTable = ConstructEditingTable(); // reload new pointer to it
		EditingTable->AddToRoot(); // Keep GC from destroying the new pointer
	}
	
	return EditingTable;
};

void FPDMissionEditorModule::FinalizeRowChanges(UDataTable* TargetTable, const TArray<FName>& RowNames) const
{
	for (const FName& RowName : RowNames)
	{
		EditingTable->HandleDataTableChanged(RowName);
	}
	EditingTable->PostEditChange();	
}

void FPDMissionEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PDMissionEditorTabName);
}

UDataTable* FPDMissionEditorModule::ConstructEditingTable()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		
	// Construct a table from a package, or load one if one already exists in target path
	FString AssetBaseName = "EditTable";
	FString PackageName = "/Game/__Mission/";
	PackageName += AssetBaseName;
	UPackage* Package = CreatePackage(*PackageName);

	UDataTable* ConstructedTable = NewObject<UDataTable>(Package, *AssetBaseName, RF_Standalone | RF_Public);
	ConstructedTable->RowStruct = FPDAssociativeMissionEditingRow::StaticStruct();
	
	AssetRegistryModule.AssetCreated(ConstructedTable);
	Package->FullyLoad();
	Package->SetDirtyFlag(true);
	ConstructedTable->EmptyTable(); // clear the associative datatable, might have clutter from last load if the packages was not created anew 

	return ConstructedTable;
} 
void FPDMissionEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FPDMissionEditorCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FPDMissionEditorCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FPDMissionEditorModule, PDMissionEditor)

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