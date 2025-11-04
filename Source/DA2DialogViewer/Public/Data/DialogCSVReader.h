// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * CSV file reader utility
 */
class FDialogCSVReader
{
public:
	/**
	 * Read CSV file into rows and columns
	 * @param FilePath Path to CSV file
	 * @param OutRows Output array of rows (each row is array of columns)
	 * @return True if file was read successfully
	 */
	static bool ReadCSV(const FString& FilePath, TArray<TArray<FString>>& OutRows);

	/**
	 * Parse a single CSV line into columns
	 * @param Line Input line
	 * @param OutColumns Output columns
	 */
	static void ParseCSVLine(const FString& Line, TArray<FString>& OutColumns);

	/**
	 * Read plots.csv into plot name -> GUID map
	 * @param FilePath Path to plots.csv
	 * @param OutPlotMap Output map from plot name to GUID
	 * @return True if successful
	 */
	static bool ReadPlotsCSV(const FString& FilePath, TMap<FString, FString>& OutPlotMap);

	/**
	 * Read plots.csv into GUID -> plot name map (reverse mapping)
	 * @param FilePath Path to plots.csv
	 * @param OutGUIDMap Output map from GUID to plot name
	 * @return True if successful
	 */
	static bool ReadPlotsCSVReverse(const FString& FilePath, TMap<FString, FString>& OutGUIDMap);

	/**
	 * Read TableTalk.csv into TLK ID -> text string map
	 * @param FilePath Path to TableTalk.csv
	 * @param OutTLKMap Output map from TLK string ID to localized text
	 * @return True if successful
	 */
	static bool ReadTLKStringsCSV(const FString& FilePath, TMap<int32, FString>& OutTLKMap);
};
