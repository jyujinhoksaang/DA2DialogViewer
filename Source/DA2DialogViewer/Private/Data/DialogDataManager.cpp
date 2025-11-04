// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/DialogDataManager.h"
#include "Data/ConversationParser.h"
#include "Data/DialogCSVReader.h"
#include "Misc/Paths.h"
#include "XmlFile.h"

FDialogDataManager::FDialogDataManager()
	: PlayerGender(EPlayerGender::Male)
	  , bIsInitialized(false)
{
}

FDialogDataManager::~FDialogDataManager()
{
}

bool FDialogDataManager::Initialize(const FString& InDataDirectory)
{
	DataDirectory = InDataDirectory;

	// Load plots.csv
	FString PlotsCSVPath = FPaths::Combine(DataDirectory, TEXT("plo_727/plots.csv"));
	if (!PlotDatabase.LoadPlotsCSV(PlotsCSVPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load plots.csv from: %s"), *PlotsCSVPath);
	}

	// Load dialog.csv
	FString DialogCSVPath = FPaths::Combine(DataDirectory, TEXT("DLG/dialog.csv"));
	if (!AudioMapper.LoadDialogCSV(DialogCSVPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load dialog.csv from: %s"), *DialogCSVPath);
	}

	// Load TableTalk.csv (TLK strings)
	FString TableTalkPath = FPaths::Combine(DataDirectory, TEXT("DLG/csv/TableTalk.csv"));
	if (!FDialogCSVReader::ReadTLKStringsCSV(TableTalkPath, TLKStrings))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load TableTalk.csv from: %s"), *TableTalkPath);
	}

	bIsInitialized = true;

	UE_LOG(LogTemp, Log, TEXT("DialogDataManager initialized with data directory: %s"), *DataDirectory);
	UE_LOG(LogTemp, Log, TEXT("  - Plots loaded: %d"), PlotDatabase.GetPlotCount());
	UE_LOG(LogTemp, Log, TEXT("  - TLK strings loaded: %d"), TLKStrings.Num());

	return true;
}

bool FDialogDataManager::LoadConversation(const FString& ConversationPath)
{
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Error, TEXT("DialogDataManager not initialized"));
		return false;
	}

	// Create new conversation
	TSharedPtr<FConversation> NewConversation = MakeShared<FConversation>();

	// Parse conversation XML
	if (!FConversationParser::ParseConversation(ConversationPath, *NewConversation))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse conversation: %s"), *ConversationPath);
		return false;
	}

	// Set as current conversation
	CurrentConversation = NewConversation;

	// Find and set owner tag from UTC files
	CurrentConversation->OwnerTag = FindOwnerTagForConversation(CurrentConversation->ConversationName);

	// Reset plot state for new conversation
	ResetPlotState();

	UE_LOG(LogTemp, Log, TEXT("Loaded conversation: %s (Owner: %s)"), *CurrentConversation->ConversationName, *CurrentConversation->OwnerTag);

	return true;
}

FString FDialogDataManager::GetAudioDirectory() const
{
	return FPaths::Combine(DataDirectory, TEXT("all_conv_wav"));
}

void FDialogDataManager::ResetPlotState()
{
	PlotState.Reset();
}

FString FDialogDataManager::GetTLKString(int32 TLKID) const
{
	// Handle special case: 4294967295 (unsigned -1) is a placeholder
	if (TLKID == -1 || static_cast<uint32>(TLKID) == 4294967295)
	{
		return TEXT("-1");
	}

	// Handle invalid/zero TLK IDs
	if (TLKID <= 0)
	{
		return TEXT("");
	}

	const FString* FoundString = TLKStrings.Find(TLKID);
	if (FoundString)
	{
		// Return empty string if the TLK content is empty
		if (FoundString->IsEmpty())
		{
			return TEXT("");
		}

		// Process rich text and special markers
		return ProcessTLKString(*FoundString);
	}

	// Return fallback text with TLK ID if not found
	return FString::Printf(TEXT("[TLK %d - Not Found]"), TLKID);
}

FString FDialogDataManager::ProcessTLKString(const FString& RawString) const
{
	if (RawString.IsEmpty())
	{
		return TEXT("");
	}

	FString Processed = RawString.TrimStartAndEnd();

	// Check if entire string is a special metadata marker like [Character] or [Action]
	// These should be treated as empty/invisible
	if (Processed.StartsWith(TEXT("[")) && Processed.EndsWith(TEXT("]")))
	{
		// Check if it's ALL metadata (no closing tag paired with opening tag)
		// e.g. "[Character]" or "[SpecialMarker]" but NOT "[bold]text[/bold]"
		if (!Processed.Contains(TEXT("[/")))
		{
			// This is a metadata marker, treat as empty
			return TEXT("");
		}
	}

	// Process Dragon Age 2's actual markup tags
	// Based on analysis of TableTalk.csv, DA2 uses XML-style tags
	FString Result = Processed;

	// === TEXT FORMATTING TAGS ===
	// <emp>text</emp> - Emphasis (italic/bold)
	Result = Result.Replace(TEXT("<emp>"), TEXT(""));
	Result = Result.Replace(TEXT("</emp>"), TEXT(""));

	// <title>text</title> - Book/document titles (italic)
	Result = Result.Replace(TEXT("<title>"), TEXT(""));
	Result = Result.Replace(TEXT("</title>"), TEXT(""));

	// <attrib>text</attrib> - Attribution/citation
	Result = Result.Replace(TEXT("<attrib>"), TEXT(""));
	Result = Result.Replace(TEXT("</attrib>"), TEXT(""));

	// === DYNAMIC PLACEHOLDERS (self-closing tags) ===
	// These are replaced at runtime by the game with actual values

	// Character name placeholders
	// Default names: Garrett (male), Marian (female)
	const TCHAR* PlayerName = (PlayerGender == EPlayerGender::Male) ? TEXT("Garrett") : TEXT("Marian");
	Result = Result.Replace(TEXT("<FirstName/>"), PlayerName);
	Result = Result.Replace(TEXT("<A/>"), PlayerName);

	// Numeric value placeholders (stats, damage, etc.)
	Result = Result.Replace(TEXT("<powervalue/>"), TEXT("[Value]"));
	Result = Result.Replace(TEXT("<damage/>"), TEXT("[Damage]"));
	Result = Result.Replace(TEXT("<force/>"), TEXT("[Force]"));
	Result = Result.Replace(TEXT("<duration/>"), TEXT("[Duration]"));
	Result = Result.Replace(TEXT("<value/>"), TEXT("[Value]"));
	Result = Result.Replace(TEXT("<procchance/>"), TEXT("[Chance]"));

	// Float multiplier placeholders
	Result = Result.Replace(TEXT("<float5/>"), TEXT("[x]"));
	Result = Result.Replace(TEXT("<float6/>"), TEXT("[x]"));
	Result = Result.Replace(TEXT("<float7/>"), TEXT("[x]"));
	Result = Result.Replace(TEXT("<float5x100/>"), TEXT("[%]"));
	Result = Result.Replace(TEXT("<float6x100/>"), TEXT("[%]"));
	Result = Result.Replace(TEXT("<float7x100/>"), TEXT("[%]"));

	// Status effect icons (appear before status text)
	Result = Result.Replace(TEXT("<brittleicon/>"), TEXT("[BRITTLE] "));
	Result = Result.Replace(TEXT("<staggericon/>"), TEXT("[STAGGER] "));
	Result = Result.Replace(TEXT("<disorienticon/>"), TEXT("[DISORIENT] "));

	// Control/button placeholders
	Result = Result.Replace(TEXT("<theleftstick/>"), TEXT("[Left Stick]"));
	Result = Result.Replace(TEXT("<Y/>"), TEXT("[Y]"));
	Result = Result.Replace(TEXT("<LT/>"), TEXT("[LT]"));
	Result = Result.Replace(TEXT("<GUIInteractionEnter/>"), TEXT("[Enter]"));

	// Item/ability requirement placeholders
	Result = Result.Replace(TEXT("<itemrequirements/>"), TEXT("[Requirements]"));
	Result = Result.Replace(TEXT("<passive1/>"), TEXT("[Passive]"));
	Result = Result.Replace(TEXT("<upgrade1/>"), TEXT("[Upgrade]"));
	Result = Result.Replace(TEXT("<nocopy/>"), TEXT(""));

	// TODO: Convert to proper Slate RichText format when we implement SRichTextBlock
	// For now, we strip/replace tags with plain text equivalents

	return Result;
}

FString FDialogDataManager::FindOwnerTagForConversation(const FString& ConversationName) const
{
	// Search UTC files for one that references this conversation
	FString UTCDirectory = FPaths::Combine(DataDirectory, TEXT("utc"));

	TArray<FString> UTCFiles;
	IFileManager::Get().FindFiles(UTCFiles, *(UTCDirectory / TEXT("*.xml")), true, false);

	for (const FString& UTCFile : UTCFiles)
	{
		FString FullPath = FPaths::Combine(UTCDirectory, UTCFile);

		// Load and parse UTC XML file
		TSharedPtr<FXmlFile> XmlFile = MakeShared<FXmlFile>(FullPath, EConstructMethod::ConstructFromFile);
		if (!XmlFile->IsValid())
		{
			continue; // Skip invalid XML files
		}

		// Get root node
		const FXmlNode* RootNode = XmlFile->GetRootNode();
		if (!RootNode)
		{
			continue;
		}

		// Find the struct node (should be first child of root)
		const FXmlNode* StructNode = nullptr;
		for (const FXmlNode* Child : RootNode->GetChildrenNodes())
		{
			if (Child->GetTag() == TEXT("struct"))
			{
				StructNode = Child;
				break;
			}
		}

		if (!StructNode)
		{
			continue;
		}

		// Look for ConversationResR and Tag nodes
		FString ConversationResR;
		FString Tag;

		for (const FXmlNode* Child : StructNode->GetChildrenNodes())
		{
			if (Child->GetTag() == TEXT("resref") && Child->GetAttribute(TEXT("label")) == TEXT("ConversationResR"))
			{
				ConversationResR = Child->GetContent();
				UE_LOG(LogTemp, Log, TEXT("DEBUG: Found ConversationResR = '%s'"), *ConversationResR);
			}
			else if (Child->GetTag() == TEXT("exostring") && Child->GetAttribute(TEXT("label")) == TEXT("Tag"))
			{
				Tag = Child->GetContent();
				UE_LOG(LogTemp, Log, TEXT("DEBUG: Found Tag = '%s' (Length: %d)"), *Tag, Tag.Len());
			}

			// Early exit if we found both
			if (!ConversationResR.IsEmpty() && !Tag.IsEmpty())
			{
				break;
			}
		}

		// Check if this UTC file references our conversation
		if (ConversationResR == ConversationName)
		{
			UE_LOG(LogTemp, Log, TEXT("Found owner tag '%s' for conversation '%s' in UTC file '%s'"),
				*Tag, *ConversationName, *UTCFile);
			return Tag;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Could not find UTC file for conversation: %s"), *ConversationName);
	return TEXT("");
}
