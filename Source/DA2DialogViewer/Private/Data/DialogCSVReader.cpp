// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/DialogCSVReader.h"
#include "Misc/FileHelper.h"

bool FDialogCSVReader::ReadCSV(const FString& FilePath, TArray<TArray<FString>>& OutRows)
{
	OutRows.Empty();

	// Read file into string
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to read CSV file: %s"), *FilePath);
		return false;
	}

	// Split into lines
	TArray<FString> Lines;
	FileContent.ParseIntoArrayLines(Lines);

	// Parse each line
	for (const FString& Line : Lines)
	{
		if (Line.IsEmpty())
		{
			continue;
		}

		TArray<FString> Columns;
		ParseCSVLine(Line, Columns);
		OutRows.Add(Columns);
	}

	return OutRows.Num() > 0;
}

void FDialogCSVReader::ParseCSVLine(const FString& Line, TArray<FString>& OutColumns)
{
	OutColumns.Empty();

	FString CurrentColumn;
	bool bInQuotes = false;

	for (int32 i = 0; i < Line.Len(); ++i)
	{
		TCHAR Char = Line[i];

		if (Char == TEXT('"'))
		{
			bInQuotes = !bInQuotes;
		}
		else if (Char == TEXT(',') && !bInQuotes)
		{
			// End of column
			OutColumns.Add(CurrentColumn.TrimStartAndEnd());
			CurrentColumn.Empty();
		}
		else
		{
			CurrentColumn.AppendChar(Char);
		}
	}

	// Add last column
	if (!CurrentColumn.IsEmpty() || OutColumns.Num() > 0)
	{
		OutColumns.Add(CurrentColumn.TrimStartAndEnd());
	}
}

bool FDialogCSVReader::ReadPlotsCSV(const FString& FilePath, TMap<FString, FString>& OutPlotMap)
{
	OutPlotMap.Empty();

	TArray<TArray<FString>> Rows;
	if (!ReadCSV(FilePath, Rows))
	{
		return false;
	}

	// Each row: plot_name, GUID
	for (const TArray<FString>& Row : Rows)
	{
		if (Row.Num() >= 2)
		{
			FString PlotName = Row[0].TrimStartAndEnd();
			FString GUID = Row[1].TrimStartAndEnd();

			if (!PlotName.IsEmpty() && !GUID.IsEmpty())
			{
				OutPlotMap.Add(PlotName, GUID);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d plot mappings from %s"), OutPlotMap.Num(), *FilePath);
	return OutPlotMap.Num() > 0;
}

bool FDialogCSVReader::ReadPlotsCSVReverse(const FString& FilePath, TMap<FString, FString>& OutGUIDMap)
{
	OutGUIDMap.Empty();

	TArray<TArray<FString>> Rows;
	if (!ReadCSV(FilePath, Rows))
	{
		return false;
	}

	// Each row: plot_name, GUID
	// Reverse mapping: GUID -> plot_name
	for (const TArray<FString>& Row : Rows)
	{
		if (Row.Num() >= 2)
		{
			FString PlotName = Row[0].TrimStartAndEnd();
			FString GUID = Row[1].TrimStartAndEnd();

			if (!PlotName.IsEmpty() && !GUID.IsEmpty())
			{
				OutGUIDMap.Add(GUID, PlotName);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d GUID->plot mappings from %s"), OutGUIDMap.Num(), *FilePath);
	return OutGUIDMap.Num() > 0;
}

bool FDialogCSVReader::ReadTLKStringsCSV(const FString& FilePath, TMap<int32, FString>& OutTLKMap)
{
	OutTLKMap.Empty();

	TArray<TArray<FString>> Rows;
	if (!ReadCSV(FilePath, Rows))
	{
		return false;
	}

	// TableTalk.csv format: TLK_ID, localized_text
	// Example: 6090301,"Hey! We heard you in there. Asking about the healer."
	for (const TArray<FString>& Row : Rows)
	{
		if (Row.Num() >= 2)
		{
			FString TLKIDString = Row[0].TrimStartAndEnd();
			FString LocalizedText = Row[1].TrimStartAndEnd();

			// Convert TLK ID string to integer
			if (!TLKIDString.IsEmpty() && !LocalizedText.IsEmpty())
			{
				int32 TLKID = FCString::Atoi(*TLKIDString);
				if (TLKID > 0)
				{
					OutTLKMap.Add(TLKID, LocalizedText);
				}
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Loaded %d TLK strings from %s"), OutTLKMap.Num(), *FilePath);
	return OutTLKMap.Num() > 0;
}
