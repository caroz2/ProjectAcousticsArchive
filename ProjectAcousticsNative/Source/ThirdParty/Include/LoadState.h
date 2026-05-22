// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

namespace TritonRuntime
{
    enum class LoadState
    {
        Loaded,
        NotLoaded,
        LoadFailed,
        LoadInProgress,
        DoesNotExist,
        Invalid
    };
}