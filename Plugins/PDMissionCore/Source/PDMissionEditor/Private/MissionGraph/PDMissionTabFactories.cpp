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

#include "MissionGraph/PDMissionTabFactories.h"
#include "..\..\Public\MissionGraph\FPDMissionEditor.h"

#include "Engine/Blueprint.h"

#include "Widgets/Docking/SDockTab.h"


#define LOCTEXT_NAMESPACE "MissionEditorFactories"

//////////////////////////////////////////////////////////////////////
//

FPDMissionDetailsFactory::FPDMissionDetailsFactory(const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditorPtr)
	: FWorkflowTabFactory(FPDMissionEditorTabs::GraphDetailsID, InMissionEditorPtr)
	, MissionEditorPtr(InMissionEditorPtr)
{
	TabLabel = LOCTEXT("MissionDetailsLabel", "Details");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Components");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MissionDetailsView", "Details");
	ViewMenuTooltip = LOCTEXT("MissionDetailsView_ToolTip", "Show the details view");
}

TSharedRef<SWidget> FPDMissionDetailsFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	check(MissionEditorPtr.IsValid());
	return MissionEditorPtr.Pin()->SpawnProperties();
}

FText FPDMissionDetailsFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("MissionDetailsTabTooltip", "The behavior tree details tab allows editing of the properties of behavior tree nodes");
}

//////////////////////////////////////////////////////////////////////
//

FPDMissionSearchFactory::FPDMissionSearchFactory(const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditorPtr)
	: FWorkflowTabFactory(FPDMissionEditorTabs::SearchID, InMissionEditorPtr)
	, MissionEditorPtr(InMissionEditorPtr)
{
	TabLabel = LOCTEXT("MissionSearchLabel", "Search");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.SearchResults");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MissionSearchView", "Search");
	ViewMenuTooltip = LOCTEXT("MissionSearchView_ToolTip", "Show the mission tree search tab");
}

TSharedRef<SWidget> FPDMissionSearchFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return MissionEditorPtr.Pin()->SpawnSearch();
}

FText FPDMissionSearchFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("MissionSearchTabTooltip", "The behavior tree search tab allows searching within behavior tree nodes");
}

//////////////////////////////////////////////////////////////////////
//

FPDMissionTreeEditorFactory::FPDMissionTreeEditorFactory(const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditorPtr)
	: FWorkflowTabFactory(FPDMissionEditorTabs::TreeEditorID, InMissionEditorPtr)
	, MissionEditorPtr(InMissionEditorPtr)
{
	TabLabel = LOCTEXT("MissionTreeEditorLabel", "Mission Tree");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "Kismet.Tabs.Tree");

	bIsSingleton = true;

	ViewMenuDescription = LOCTEXT("MissionTreeEditorView", "Mission Tree");
	ViewMenuTooltip = LOCTEXT("MissionTreeEditorView_ToolTip", "Mission Tree");
}

TSharedRef<SWidget> FPDMissionTreeEditorFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return MissionEditorPtr.Pin()->SpawnMissionTree();
}

FText FPDMissionTreeEditorFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return LOCTEXT("MissionTreeEditorTabTooltip", "");
}

//////////////////////////////////////////////////////////////////////
//

FPDMissionGraphEditorFactory::FPDMissionGraphEditorFactory(const TSharedPtr<FFPDMissionGraphEditor>& InMissionEditorPtr, const FOnCreateGraphEditorWidget& CreateGraphEditorWidgetCallback)
	: FDocumentTabFactoryForObjects<UEdGraph>(FPDMissionEditorTabs::GraphEditorID, InMissionEditorPtr)
	, MissionEditorPtr(InMissionEditorPtr)
	, OnCreateGraphEditorWidget(CreateGraphEditorWidgetCallback)
{
}

void FPDMissionGraphEditorFactory::OnTabActivated(TSharedPtr<SDockTab> Tab) const
{
	check(MissionEditorPtr.IsValid());
	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	MissionEditorPtr.Pin()->OnGraphEditorFocused(GraphEditor);
}

void FPDMissionGraphEditorFactory::OnTabRefreshed(TSharedPtr<SDockTab> Tab) const
{
	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());
	GraphEditor->NotifyGraphChanged();
}

TAttribute<FText> FPDMissionGraphEditorFactory::ConstructTabNameForObject(UEdGraph* DocumentID) const
{
	return TAttribute<FText>( FText::FromString( DocumentID->GetName() ) );
}

TSharedRef<SWidget> FPDMissionGraphEditorFactory::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return OnCreateGraphEditorWidget.Execute(DocumentID);
}

const FSlateBrush* FPDMissionGraphEditorFactory::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* DocumentID) const
{
	return FAppStyle::GetBrush("NoBrush");
}

void FPDMissionGraphEditorFactory::SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const
{
	check(MissionEditorPtr.IsValid());
	check(MissionEditorPtr.Pin()->GetMissionData().DataTarget.DataTable);

	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Tab->GetContent());

	FVector2D ViewLocation;
	float ZoomAmount;
	GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);

	UEdGraph* Graph = FTabPayload_UObject::CastChecked<UEdGraph>(Payload);
	MissionEditorPtr.Pin()->GetMissionData().LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
	// MissionEditorPtr.Pin()->GetMissionData()->LastEditedDocuments.Add(FEditedDocumentInfo(Graph, ViewLocation, ZoomAmount));
}

#undef LOCTEXT_NAMESPACE
