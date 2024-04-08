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

#include "MissionGraph/PDMissionBuilder.h"
#include "MissionGraph/PDMissionGraph.h"
#include "MissionGraph/PDMissionGraphNode.h"
#include "MissionGraph/PDMissionGraphSchema.h"
#include "PDMissionGraphTypes.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Stats/StatsMisc.h"

#include "Misc/MessageDialog.h"


#define LOCTEXT_NAMESPACE "FPDMissionBuilder"

UPDMissionGraph* FPDMissionBuilder::CreateNewGraph(const FPDMissionNodeHandle& NodeData, FName GraphName)
{
	UPDMissionGraph* NewGraph = CastChecked<UPDMissionGraph>(FBlueprintEditorUtils::CreateNewGraph(static_cast<UDataTable*>(NodeData.DataTarget.DataTable), GraphName, UPDMissionGraph::StaticClass(), UPDMissionGraphSchema::StaticClass()));

	const UEdGraphSchema* Schema = NewGraph->GetSchema();
	Schema->CreateDefaultNodesForGraph(*NewGraph);

	NewGraph->OnCreated();

	return NewGraph;
}


int32 FPDMissionBuilder::GetNumGraphs(const FPDMissionNodeHandle& NodeData)
{
	check(NodeData.DataTarget.DataTable);
	return NodeData.SourceGraphs.Num();
}

UPDMissionGraph* FPDMissionBuilder::GetGraphFromBank(FPDMissionNodeHandle& NodeData, const int32 Index)
{
	check(NodeData.DataTarget.DataTable);
	return NodeData.SourceGraphs.IsValidIndex(Index) ? CastChecked<UPDMissionGraph>(NodeData.SourceGraphs[Index]) : nullptr;
}

void FPDMissionBuilder::RebuildData(FPDMissionNodeHandle& NodeData)
{
	SCOPE_LOG_TIME_IN_SECONDS(TEXT("FPDMissionBuilder::RebuildData"), nullptr);

	check(NodeData.DataTarget.DataTable);

	// Merge all the graphs
	TArray<UPDMissionGraphNode*> AllGraphNodes;
	for (const UEdGraph* Graph : NodeData.SourceGraphs)
	{
		Graph->GetNodesOfClass<UPDMissionGraphNode>(/*inout*/ AllGraphNodes);
	}

	// 1.  Read table rows from the given mission-row handles
	// 2.  Walk different the paths in them until a loop is found
	// 3.  build up pin association between then amd update data representation

}

void FPDMissionBuilder::ForeachConnectedOutgoingMissionNode(UEdGraphPin* Pin, TFunctionRef<void(UPDMissionGraphNode*)> Predicate)
{
	for (const UEdGraphPin* RemotePin : Pin->LinkedTo)
	{
		if (const UPDMissionGraphNode_Knot* Knot = Cast<UPDMissionGraphNode_Knot>(RemotePin->GetOwningNode()))
		{
			ForeachConnectedOutgoingMissionNode(Knot->GetOutputPin(), Predicate);
		}
		else if (UPDMissionGraphNode* RemoteNode = Cast<UPDMissionGraphNode>(RemotePin->GetOwningNode()))
		{
			Predicate(RemoteNode);
		}
	}
}

#undef LOCTEXT_NAMESPACE
