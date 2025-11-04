// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DialogFlow/DialogNode.h"

class FPlotState;

/**
 * Evaluates plot conditions for dialog visibility
 */
class FConditionEvaluator
{
public:
	/**
	 * Evaluate if plot condition is met
	 * @param Condition Plot condition to evaluate
	 * @param PlotState Current plot state
	 * @return True if condition is satisfied
	 */
	static bool EvaluateCondition(const FPlotReference& Condition, const FPlotState& PlotState);

	/**
	 * Evaluate if dialog link should be visible
	 * @param Link Dialog link to check
	 * @param PlotState Current plot state
	 * @return True if link should be shown
	 */
	static bool EvaluateLinkCondition(const FDialogLink& Link, const FPlotState& PlotState);

	/**
	 * Evaluate if dialog node should be accessible
	 * @param Node Dialog node to check
	 * @param PlotState Current plot state
	 * @return True if node should be accessible
	 */
	static bool EvaluateNodeCondition(const FDialogNode& Node, const FPlotState& PlotState);

private:
	// Compare flag value based on comparison type
	static bool CompareFlagValue(int32 ActualValue, int32 ExpectedValue, uint8 ComparisonType);
};
