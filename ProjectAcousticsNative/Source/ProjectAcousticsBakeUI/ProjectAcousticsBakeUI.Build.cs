// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

// NOTE: Uncomment the define below to enable PhysX collision mesh support
// #define ENABLE_COLLISION_SUPPORT

using UnrealBuildTool;
using System.IO;

public class ProjectAcousticsBakeUI : ModuleRules
{
    public ProjectAcousticsBakeUI(ReadOnlyTargetRules Target) : base(Target)
    {
        //PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PCHUsage = ModuleRules.PCHUsageMode.Default;
        OptimizeCode = CodeOptimization.InShippingBuildsOnly;
        PrivatePCHHeaderFile = "Public/AcousticsEditorModule.h";

        PublicIncludePaths.AddRange(
            new string[] {
                // ... add public include paths required here ...
            }
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "ThirdParty/Include",
                "../Source/ProjectAcoustics/Public"
                // ... add other private include paths required here ...
            }
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
#if UE_4_22_OR_LATER
                "MeshDescription",
#endif
                "ProjectAcoustics",
                "UnrealEd",
#if UE_5_0_OR_LATER
                "EditorFramework",
#endif
                // ... add other public dependencies that you statically link with here ...
                // Adding Source Control module to list of public dependencies to use
                // source control operations during the prebake and bake process.
                "SourceControl"
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "RenderCore",
                "Slate",
                "SlateCore",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                "EditorStyle",
                "Projects",
#if UE_5_0_OR_LATER
                "DeveloperToolSettings",
#endif
#if UE_4_20_OR_LATER
                "NavigationSystem",
#else
                "NavMesh",
#endif
                "RawMesh",
                "StaticMeshDescription",
                "Landscape",
                "DesktopWidgets",
                // Dependencies for converting landscape to static mesh
                "PropertyEditor",
                "MeshUtilitiesCommon",
                "MeshDescription",
                // ... add private dependencies that you statically link with here ...
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                // ... add any modules that your module loads dynamically here ...
            }
            );

        const string DllName = "Triton.Preprocessor.dll";
        const string LibName = "Triton.Preprocessor.lib";

        var configuration = "Release";
        var arch = "Win64";
        var thirdPartyDir = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Source/ThirdParty/"));
        var thirdPartyLibPath = Path.Combine(Path.Combine(thirdPartyDir, arch), configuration);
        var tritonPreprocessorDllPath = Path.Combine(thirdPartyLibPath, DllName);
        var tritonPreprocessorLibPath = Path.Combine(thirdPartyLibPath, LibName);

        PublicAdditionalLibraries.Add(tritonPreprocessorLibPath);

        PublicDelayLoadDLLs.Add(DllName);
#if UE_4_20_OR_LATER
        RuntimeDependencies.Add(tritonPreprocessorDllPath);
#else
        RuntimeDependencies.Add(new RuntimeDependency(tritonPreprocessorDllPath));
#endif

#if ENABLE_COLLISION_SUPPORT
        PublicDefinitions.Add("ENABLE_COLLISION_SUPPORT=1");
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string FbxSdkDir = Target.UEThirdPartySourceDirectory + "FBX/2018.1.1/";
            PublicSystemIncludePaths.AddRange(
                new string[] {
                    FbxSdkDir + "include",
                    FbxSdkDir + "include/fbxsdk",
                    }
                );

            string FbxLibPath = FbxSdkDir + "lib/vs" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";

            FbxLibPath += "x64/release/";
            PublicLibraryPaths.Add(FbxLibPath);

            if (Target.LinkType != TargetLinkType.Monolithic)
            {
                PublicAdditionalLibraries.Add("libfbxsdk.lib");

                // We are using DLL versions of the FBX libraries
                PublicDefinitions.Add("FBXSDK_SHARED");

                RuntimeDependencies.Add("$(TargetOutputDir)/libfbxsdk.dll", FbxLibPath + "libfbxsdk.dll");
            }
            else
            {
                if (Target.bUseStaticCRT)
                {
                    PublicAdditionalLibraries.Add("libfbxsdk-mt.lib");
                }
                else
                {
                    PublicAdditionalLibraries.Add("libfbxsdk-md.lib");
                }
            }
        }
#endif // ENABLE_COLLISION_SUPPORT
    }
}
