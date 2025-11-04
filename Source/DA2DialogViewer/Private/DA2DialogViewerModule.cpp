// Copyright Epic Games, Inc. All Rights Reserved.

#include "DA2DialogViewerModule.h"
#include "Data/DialogDataManager.h"
#include "UI/SDialogViewerWindow.h"
#include "ToolMenus.h"
#include "Misc/Paths.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "FDA2DialogViewerModule"

static const FName DialogViewerTabName("DA2DialogViewer");

void FDA2DialogViewerModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("DA2DialogViewer: Module starting up"));

	// Create data manager
	DataManager = MakeShared<FDialogDataManager>();

	// Initialize data manager with project's Data directory
	FString ProjectDir = FPaths::ProjectDir();
	FString DataDir = FPaths::Combine(ProjectDir, TEXT("Data"));

	if (FPaths::DirectoryExists(DataDir))
	{
		DataManager->Initialize(DataDir);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DA2DialogViewer: Data directory not found: %s"), *DataDir);
	}

	// Register menus
	RegisterMenus();

	UE_LOG(LogTemp, Log, TEXT("DA2DialogViewer: Module started successfully"));
}

void FDA2DialogViewerModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("DA2DialogViewer: Module shutting down"));

	// Clean up data manager
	DataManager.Reset();

	// Unregister menus
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FDA2DialogViewerModule::RegisterMenus()
{
	// Register tab spawner
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DialogViewerTabName,
		FOnSpawnTab::CreateRaw(this, &FDA2DialogViewerModule::OnSpawnDialogViewerTab))
		.SetDisplayName(LOCTEXT("DialogViewerTabTitle", "Dragon Age 2 Dialog Viewer"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	// Add menu entry
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDA2DialogViewerModule::RegisterMenuExtensions));
}

void FDA2DialogViewerModule::RegisterMenuExtensions()
{
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (!Menu)
	{
		return;
	}

	FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
	Section.AddMenuEntry(
		"OpenDA2DialogViewer",
		LOCTEXT("OpenDA2DialogViewer", "Dragon Age 2 Dialog Viewer"),
		LOCTEXT("OpenDA2DialogViewerTooltip", "Open the Dragon Age 2 Dialog Viewer window"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FDA2DialogViewerModule::CreateDialogViewerWindow))
	);
}

TSharedRef<SDockTab> FDA2DialogViewerModule::OnSpawnDialogViewerTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDialogViewerWindow, DataManager)
		];
}

void FDA2DialogViewerModule::CreateDialogViewerWindow()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DialogViewerTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDA2DialogViewerModule, DA2DialogViewer)
