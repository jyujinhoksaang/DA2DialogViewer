// Copyright Epic Games, Inc. All Rights Reserved.

#include "Plot/ActionExecutor.h"
#include "Plot/PlotState.h"

void FActionExecutor::ExecuteAction(const FPlotReference& Action, FPlotState& PlotState)
{
	// Empty plot name = no action
	if (Action.PlotName.IsEmpty())
	{
		return;
	}

	// Flag index -1 = no specific flag to set
	if (Action.FlagIndex < 0)
	{
		return;
	}

	// For now, we'll set the flag to 1 (true)
	// In a full implementation, we'd decode the comparison type to determine the value
	int32 ValueToSet = 1;

	// Set the flag
	PlotState.SetFlag(Action.PlotName, Action.FlagIndex, ValueToSet);

	UE_LOG(LogTemp, Log, TEXT("ActionExecutor: Set %s[%d] = %d"),
		*Action.PlotName, Action.FlagIndex, ValueToSet);
}

void FActionExecutor::ExecuteNodeAction(const FDialogNode& Node, FPlotState& PlotState)
{
	ExecuteAction(Node.Action, PlotState);
}
