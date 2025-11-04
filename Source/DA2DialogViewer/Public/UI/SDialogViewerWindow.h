// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FDialogDataManager;
class SDialogTreeView;
class SDialogWheel;
class STextBlock;

/**
 * Main dialog viewer window
 * Contains graph panel, dialog wheel, and controls
 */
class SDialogViewerWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogViewerWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager);

private:
	// Load conversation button clicked
	FReply OnLoadConversationClicked();

	// Reset plot state button clicked
	FReply OnResetPlotStateClicked();

	// Gender button clicked
	FReply OnGenderButtonClicked();

	// Gender toggle changed
	void OnGenderChanged(int32 NewSelection, ESelectInfo::Type SelectInfo);

	// Update status text
	void UpdateStatusText();

	// Get current status text
	FText GetStatusText() const;

	// Get current conversation name
	FText GetConversationName() const;

	// Get gender button text (dynamic)
	FText GetGenderButtonText() const;

private:
	// Data manager reference
	TSharedPtr<FDialogDataManager> DataManager;

	// Tree view widget (replaces graph panel)
	TSharedPtr<SDialogTreeView> TreeView;

	// Dialog wheel widget
	TSharedPtr<SDialogWheel> DialogWheel;

	// Status text block
	TSharedPtr<STextBlock> StatusText;

	// Current status message
	FText CurrentStatus;
};
