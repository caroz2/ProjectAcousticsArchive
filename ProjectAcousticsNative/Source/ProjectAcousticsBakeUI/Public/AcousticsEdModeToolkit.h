// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/BaseToolkit.h"

class FAcousticsEdModeToolkit : public FModeToolkit
{
public:
    FAcousticsEdModeToolkit();

    /** FModeToolkit interface */
    virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

    /** IToolkit interface */
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual class FEdMode* GetEditorMode() const override;
    virtual TSharedPtr<class SWidget> GetInlineContent() const override
    {
        return m_AcousticsEdWidget;
    }

private:
    TSharedPtr<SWidget> m_AcousticsEdWidget;
};
