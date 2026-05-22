// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "AcousticsDataFactory.generated.h"

UCLASS(hidecategories = Object)
class UAcousticsDataFactory : public UFactory, public FReimportHandler
{
    GENERATED_UCLASS_BODY()

    virtual bool FactoryCanImport(const FString& Filename) override;

    static UObject* ImportFromFile(const FString& filename);

    /**
     * Create a new object by importing it from a file name.
     *
     * The default implementation of this method will load the contents of the entire
     * file into a byte buffer and call FactoryCreateBinary. User defined factories
     * may override this behavior to process the provided file name on their own.
     *
     */
    virtual UObject* FactoryCreateFile(
        UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename,
        const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    // Reimport interfaces. Used instead of the above if uasset already exists
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    virtual int32 GetPriority() const override;

};
