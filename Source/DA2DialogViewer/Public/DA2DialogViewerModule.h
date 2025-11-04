// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FDialogDataManager;

/**
 * Main module for DA2 Dialog Viewer plugin
 */
class FDA2DialogViewerModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Get singleton instance */
	static FDA2DialogViewerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FDA2DialogViewerModule>("DA2DialogViewer");
	}

	/** Check if module is loaded */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DA2DialogViewer");
	}

	/** Get data manager */
	TSharedPtr<FDialogDataManager> GetDataManager() const { return DataManager; }

private:
	/** Register menu commands */
	void RegisterMenus();

	/** Register menu extensions */
	void RegisterMenuExtensions();

	/** Spawn dialog viewer tab */
	TSharedRef<class SDockTab> OnSpawnDialogViewerTab(const class FSpawnTabArgs& Args);

	/** Create dialog viewer window */
	void CreateDialogViewerWindow();

private:
	/** Singleton data manager */
	TSharedPtr<FDialogDataManager> DataManager;
};
