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

#include "MissionGraph/PDMissionEditorModes.h"
#include "..\..\Public\MissionGraph\FPDMissionEditor.h"
#include "PDMissionGraphTypes.h"
#include "MissionGraph/PDMissionEditorToolbar.h"
#include "MissionGraph/PDMissionTabFactories.h"

FMissionEditorApplicationMode_GraphView::FMissionEditorApplicationMode_GraphView(TSharedPtr<FFPDMissionGraphEditor> InMissionEditor)
	: FApplicationMode(FFPDMissionGraphEditor::GraphViewMode, FFPDMissionGraphEditor::GetLocalizedMode)
{
	MissionEditor = InMissionEditor;

	MissionEditorTabFactories.RegisterFactory(MakeShareable(new FPDMissionDetailsFactory(InMissionEditor)));
	MissionEditorTabFactories.RegisterFactory(MakeShareable(new FPDMissionSearchFactory(InMissionEditor)));
	MissionEditorTabFactories.RegisterFactory(MakeShareable(new FPDMissionTreeEditorFactory(InMissionEditor)));

	TabLayout = FTabManager::NewLayout( "Standalone_MissionEditor_GraphView_Layout_v4" )
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.7f)
			->AddTab(FPDMissionEditorTabs::GraphEditorID, ETabState::ClosedTab)
		)
		->Split
		(
			FTabManager::NewSplitter()
			->SetOrientation(Orient_Vertical)
			->SetSizeCoefficient(0.3f)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.6f)
				->AddTab(FPDMissionEditorTabs::GraphDetailsID, ETabState::OpenedTab)
				->AddTab(FPDMissionEditorTabs::SearchID, ETabState::ClosedTab)
			)
 			->Split
 			(
 				FTabManager::NewStack()
 				->SetSizeCoefficient(0.4f)
 				->AddTab(FPDMissionEditorTabs::TreeEditorID, ETabState::OpenedTab)
 			)
		)
	);
	
	// InMissionEditor->GetToolbarBuilder()->AddDebuggerToolbar(ToolbarExtender); // @todo: debugger
	InMissionEditor->GetToolbarBuilder()->AddMissionEditorToolbar(ToolbarExtender);
}

void FMissionEditorApplicationMode_GraphView::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	check(MissionEditor.IsValid());
	const TSharedPtr<FFPDMissionGraphEditor> MissionEditorPtr = MissionEditor.Pin();
	
	MissionEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	MissionEditorPtr->PushTabFactories(MissionEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FMissionEditorApplicationMode_GraphView::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(MissionEditor.IsValid());
	const TSharedPtr<FFPDMissionGraphEditor> MissionEditorPtr = MissionEditor.Pin();
	
	MissionEditorPtr->SaveEditedObjectState();
}

void FMissionEditorApplicationMode_GraphView::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	check(MissionEditor.IsValid());
	const TSharedPtr<FFPDMissionGraphEditor> MissionEditorPtr = MissionEditor.Pin();
	MissionEditorPtr->RegenerateMissionGraph();

	FApplicationMode::PostActivateMode();
}
