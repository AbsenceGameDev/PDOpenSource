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
