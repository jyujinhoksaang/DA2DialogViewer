// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogFlow/DialogNode.h"

class FPlotState;

/**
 * Executes plot actions when dialog nodes are triggered
 */
class FActionExecutor
{
public:
	/**
	 * Execute plot action
	 * @param Action Plot action to execute
	 * @param PlotState Plot state to modify
	 */
	static void ExecuteAction(const FPlotReference& Action, FPlotState& PlotState);

	/**
	 * Execute node's plot action
	 * @param Node Dialog node whose action to execute
	 * @param PlotState Plot state to modify
	 */
	static void ExecuteNodeAction(const FDialogNode& Node, FPlotState& PlotState);
};
