// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/SDialogViewerWindow.h"
#include "UI/SDialogTreeView.h"
#include "UI/SDialogWheel.h"
#include "Data/DialogDataManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"

void SDialogViewerWindow::Construct(const FArguments& InArgs, TSharedPtr<FDialogDataManager> InDataManager)
{
	DataManager = InDataManager;
	CurrentStatus = FText::FromString(TEXT("Ready"));

	// Create tree view (replaces graph panel)
	TreeView = SNew(SDialogTreeView, DataManager);

	// Create dialog wheel
	DialogWheel = SNew(SDialogWheel, DataManager);

	// Connect tree and wheel
	TreeView->SetDialogWheel(DialogWheel);
	DialogWheel->SetTreeView(TreeView);

	// Gender options
	TArray<TSharedPtr<FString>> GenderOptions;
	GenderOptions.Add(MakeShared<FString>(TEXT("Male")));
	GenderOptions.Add(MakeShared<FString>(TEXT("Female")));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(4.0f)
		[
			SNew(SVerticalBox)

			// Top toolbar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)

				// Load conversation button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Load Conversation")))
					.OnClicked(this, &SDialogViewerWindow::OnLoadConversationClicked)
				]

				// Reset plot state button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Reset Plot State")))
					.OnClicked(this, &SDialogViewerWindow::OnResetPlotStateClicked)
				]

				// Spacer
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)

				// Gender label
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Player Gender:")))
				]

				// Gender selection toggle button
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.Text(this, &SDialogViewerWindow::GetGenderButtonText)
					.OnClicked(this, &SDialogViewerWindow::OnGenderButtonClicked)
				]
			]

			// Conversation name
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SNew(STextBlock)
				.Text(this, &SDialogViewerWindow::GetConversationName)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
			]

			// Main content area
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(4.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				// Tree view panel (top, 60%)
				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(2.0f)
					[
						TreeView.ToSharedRef()
					]
				]

				// Dialog wheel (bottom, 40%)
				+ SSplitter::Slot()
				.Value(0.4f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
					.Padding(2.0f)
					[
						SNew(SBox)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							DialogWheel.ToSharedRef()
						]
					]
				]
			]

			// Status bar
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.0f)
			[
				SAssignNew(StatusText, STextBlock)
				.Text(this, &SDialogViewerWindow::GetStatusText)
			]
		]
	];
}

FReply SDialogViewerWindow::OnLoadConversationClicked()
{
	// Open file dialog
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return FReply::Handled();
	}

	TArray<FString> OutFiles;
	FString DefaultPath = FPaths::Combine(DataManager->GetDataDirectory(), TEXT("DLG/cnv"));

	const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindowHandle,
		TEXT("Select Conversation XML"),
		DefaultPath,
		TEXT(""),
		TEXT("XML Files (*.xml)|*.xml"),
		EFileDialogFlags::None,
		OutFiles
	);

	if (bOpened && OutFiles.Num() > 0)
	{
		FString ConversationPath = OutFiles[0];

		// Load conversation
		if (DataManager->LoadConversation(ConversationPath))
		{
			// Update tree view
			TreeView->LoadConversation(DataManager->GetCurrentConversation());

			// Navigate to first entry node
			TSharedPtr<FConversation> Conv = DataManager->GetCurrentConversation();
			if (Conv.IsValid() && Conv->EntryLinks.Num() > 0)
			{
				int32 FirstNodeIndex = Conv->EntryLinks[0].TargetNodeIndex;
				TreeView->NavigateToNode(FirstNodeIndex);
			}

			CurrentStatus = FText::FromString(FString::Printf(TEXT("Loaded: %s"), *FPaths::GetBaseFilename(ConversationPath)));
		}
		else
		{
			CurrentStatus = FText::FromString(TEXT("Failed to load conversation"));
		}
	}

	return FReply::Handled();
}

FReply SDialogViewerWindow::OnResetPlotStateClicked()
{
	if (DataManager.IsValid())
	{
		DataManager->ResetPlotState();
		CurrentStatus = FText::FromString(TEXT("Plot state reset"));
	}

	return FReply::Handled();
}

void SDialogViewerWindow::OnGenderChanged(int32 NewSelection, ESelectInfo::Type SelectInfo)
{
	if (DataManager.IsValid())
	{
		DataManager->SetPlayerGender(NewSelection == 0 ? EPlayerGender::Male : EPlayerGender::Female);
	}
}

void SDialogViewerWindow::UpdateStatusText()
{
	// Status text is updated via GetStatusText()
}

FText SDialogViewerWindow::GetStatusText() const
{
	return CurrentStatus;
}

FText SDialogViewerWindow::GetConversationName() const
{
	if (DataManager.IsValid())
	{
		TSharedPtr<FConversation> Conv = DataManager->GetCurrentConversation();
		if (Conv.IsValid())
		{
			// Display conversation name and owner as static header information
			if (!Conv->OwnerTag.IsEmpty())
			{
				return FText::FromString(FString::Printf(TEXT("Conversation: %s | Owner: %s"),
					*Conv->ConversationName, *Conv->OwnerTag));
			}
			else
			{
				return FText::FromString(FString::Printf(TEXT("Conversation: %s"), *Conv->ConversationName));
			}
		}
	}

	return FText::FromString(TEXT("No conversation loaded"));
}

FReply SDialogViewerWindow::OnGenderButtonClicked()
{
	if (DataManager.IsValid())
	{
		// Toggle gender
		EPlayerGender Current = DataManager->GetPlayerGender();
		DataManager->SetPlayerGender(Current == EPlayerGender::Male ? EPlayerGender::Female : EPlayerGender::Male);

		// Log the change
		FString NewGender = (DataManager->GetPlayerGender() == EPlayerGender::Male) ? TEXT("Male") : TEXT("Female");
		UE_LOG(LogTemp, Log, TEXT("DialogViewer: Player gender changed to %s"), *NewGender);
	}

	return FReply::Handled();
}

FText SDialogViewerWindow::GetGenderButtonText() const
{
	if (DataManager.IsValid())
	{
		return FText::FromString(DataManager->GetPlayerGender() == EPlayerGender::Male ? TEXT("Male") : TEXT("Female"));
	}

	return FText::FromString(TEXT("Male"));
}
