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

#include "MissionGraph/PDMissionEditorToolbar.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SComboButton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

#include "PDMissionCommon.h"
#include "..\..\Public\MissionGraph\FPDMissionEditor.h"


#define LOCTEXT_NAMESPACE "MissionEditorToolbar"

class SMissionModeSeparator : public SBorder
{
public:
	SLATE_BEGIN_ARGS(SMissionModeSeparator) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArg)
	{
		SBorder::Construct(
			SBorder::FArguments()
			.BorderImage(FAppStyle::GetBrush("BlueprintEditor.PipelineSeparator"))
			.Padding(0.0f)
			);
	}

	// SWidget interface
	virtual FVector2D ComputeDesiredSize(float) const override
	{
		constexpr float Height = 20.0f;
		constexpr float Thickness = 16.0f;
		return FVector2D(Thickness, Height);
	}
	// End of SWidget interface
};


void FPDMissionEditorToolbar::AddMissionEditorToolbar(TSharedPtr<FExtender> Extender)
{
	check(MissionEditor.IsValid());
	const TSharedPtr<FFPDMissionGraphEditor> MissionEditorPtr = MissionEditor.Pin();

	const TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension("Asset", EExtensionHook::After, MissionEditorPtr->GetToolkitCommands(), FToolBarExtensionDelegate::CreateSP( this, &FPDMissionEditorToolbar::FillMissionEditorToolbar ));
	MissionEditorPtr->AddToolbarExtender(ToolbarExtender);
}

void FPDMissionEditorToolbar::FillMissionEditorToolbar(FToolBarBuilder& ToolbarBuilder)
{
	check(MissionEditor.IsValid());
	const TSharedPtr<FFPDMissionGraphEditor> MissionEditorPtr = MissionEditor.Pin();

	if (MissionEditorPtr->DebugHandler.IsDebuggerReady() == false && MissionEditorPtr->GetCurrentMode() == FFPDMissionGraphEditor::GraphViewMode)
	{
		ToolbarBuilder.BeginSection("Mission");
		{
			ToolbarBuilder.AddComboButton(
				FUIAction(
					FExecuteAction(),
					FCanExecuteAction::CreateSP(MissionEditorPtr.Get(), &FFPDMissionGraphEditor::CanCreateNewMissionNodes),
					FIsActionChecked()
					), 
				FOnGetContent::CreateSP(MissionEditorPtr.Get(), &FFPDMissionGraphEditor::HandleCreateNewStructMenu, FPDMissionState::StaticStruct()),
				LOCTEXT("NewMission_Label", "New Mission"),
				LOCTEXT("NewMission_ToolTip", "Create a new mission node from a given mission state"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "BTEditor.Graph.NewDecorator")
			);
		}
		ToolbarBuilder.EndSection();
	}
}

#undef LOCTEXT_NAMESPACE
