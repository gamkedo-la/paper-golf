// Copyright 2024 Game Salutes and HomeTeam GameDev contributors under GPL-3.0-only.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataTable.h"

#include <concepts>

namespace PG::DataTableUtils
{
	template<std::derived_from<FTableRowBase> T>
	bool ValidateDataTableRowType(UDataTable* DataTable);

	template<std::derived_from<FTableRowBase> T>
	TArray<T> ReadAll(UDataTable* DataTable, bool bValidate = true, const char* Context = "DataTableUtils::ReadAll");
}


#pragma region Definitions


template<std::derived_from<FTableRowBase> T>
inline bool PG::DataTableUtils::ValidateDataTableRowType(UDataTable* DataTable)
{
    if (!ensureAlwaysMsgf(DataTable, TEXT("DataTable was NULL for expected row type=%s"), *T::StaticStruct()->GetFName().ToString()))
    {
        return false;
    }

    // GetRowStructPathName is only available in editor builds
#if WITH_EDITOR
    return ensureAlwaysMsgf(DataTable->GetRowStruct()->IsChildOf(T::StaticStruct()), TEXT("DataTable=%s has row type %s where %s was expected: AssetPath=%s"),
        *DataTable->GetName(), *DataTable->GetRowStructPathName().ToString(), *T::StaticStruct()->GetFName().ToString(), *DataTable->GetPathName());
#else
    return ensureAlwaysMsgf(DataTable->GetRowStruct()->IsChildOf(T::StaticStruct()), TEXT("DataTable=%s expected row type=%s for AssetPath=%s"),
        *DataTable->GetName(), *T::StaticStruct()->GetFName().ToString(), *DataTable->GetPathName());
#endif
}

template<std::derived_from<FTableRowBase> T>
TArray<T> PG::DataTableUtils::ReadAll(UDataTable* DataTable, bool bValidate, const char* Context)
{
	if (!ValidateDataTableRowType<T>(DataTable))
	{
		return {};
	}

	TArray<T*> Data;
	DataTable->GetAllRows(Context, Data);

	TArray<T> OutputData;
	OutputData.Reserve(Data.Num());

	for (auto DataRowPtr : Data)
	{
		if (DataRowPtr)
		{
			OutputData.Add(*DataRowPtr);
		}
	}

	return OutputData;
}

#pragma endregion Definitions
