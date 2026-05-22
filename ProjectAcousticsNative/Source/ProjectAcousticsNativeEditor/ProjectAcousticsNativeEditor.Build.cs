// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using UnrealBuildTool;
using System.IO;

public class ProjectAcousticsNativeEditor : ModuleRules
{
    public ProjectAcousticsNativeEditor(ReadOnlyTargetRules Target) : base(Target)
    {

        PCHUsage = ModuleRules.PCHUsageMode.Default;
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        PrivatePCHHeaderFile = "Public/AcousticsNativeEditorModule.h";

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                // ... add other private include paths required here ...
                "../Source/ProjectAcousticsNative/Public",
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "ProjectAcoustics",
                "ProjectAcousticsNative",
                // ... add other public dependencies that you statically link with here ...
                // Adding Source Control module to list of public dependencies to use
                // source control operations during the prebake and bake process.
                "SourceControl"
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd"
                // Dependencies for converting landscape to static mesh
                // ... add private dependencies that you statically link with here ...
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );
    }
}
