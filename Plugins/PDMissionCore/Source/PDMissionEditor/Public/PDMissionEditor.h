/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

#pragma once

#include "CoreMinimal.h"
#include "HistoryManager.h"
#include "Modules/ModuleManager.h"

struct FPDMissionRow;
class UPDMissionGraph;
class FAssetTypeActions_Base;
class FToolBarBuilder;
class FMenuBuilder;

class FPDMissionEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	
	/** @brief Copies a given table to a table constructed at editor time (the working set) */
	void CopyMissionTable(const UDataTable* const TableToCopy, bool bAccumulateTables = false);
	/** @brief Saves the working set to it's associated tables. @otod turn TabelToSaveIn into an optional parameter and make a slight adjustment in the function defintion */
	void SaveToMissionTable(UDataTable* TableToSaveIn);
	
	/** @brief Marks an associated tablerow for removal */
	bool RemoveFromEditingTable(const FName& RowNameToRemove);
	/** @brief Edit row copy and mark as updated */
	bool EditRowInEditingTable(const FName& RowNameToRemove, const FPDMissionRow& NewData);
	/** @brief Add row copy and mark as updated, will be added to an associated table, if an associated table not available will be added to 'TableToSaveIn' */
	void AddRowToEditingTable(const FName& RowNameToAdd, const FPDMissionRow& NewData);
	/** @brief Finalize any changes which hasn't been saved */
	void FinalizeRowChanges(UDataTable* TargetTable, const TArray<FName>& RowNames) const;

	/** @brief Marks an associated tablerow for removal, marks table dirty and saves changes immediately */
	bool RemoveFromEditingTable_MarkDirty(const FName& RowNameToRemove);
	/** @brief Edit row copy and mark as updated, marks 'EditingTable' dirty and saves changes immediately */
	bool EditRowInEditingTable_MarkDirty(const FName& RowNameToRemove, const FPDMissionRow& NewData);
	/** @brief Add row copy and mark as updated, marks 'EditingTable' dirty and saves changes immediately  */
	bool AddRowToEditingTable_MarkDirty(const FName& RowNameToAdd, const FPDMissionRow& NewData);

	UDataTable* GetIntermediaryEditingTable(const bool bReconstruct);

	UDataTable* ConstructEditingTable();
	
private:
	
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	UPDMissionGraph* GraphObj = nullptr;
	TSharedPtr<class FUICommandList> PluginCommands;
	TArray<TSharedPtr<class FAssetTypeActions_Base>> ItemDataAssetTypeActions;

	/** @brief When opening the graph, create a full copy of the table. That copy is what we should be editing, then upon saving we move the changes over to the correct tables */
	UDataTable* EditingTable = nullptr; 
	bool bEditTableParity = true;
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