/* @author: Ario Amin @ Permafrost Development. @copyright: Full BSL(1.1) License included at bottom of the file  */

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