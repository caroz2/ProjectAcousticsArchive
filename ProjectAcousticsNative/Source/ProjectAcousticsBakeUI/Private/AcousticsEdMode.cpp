// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//   Created from SFoliageEdit class code

#include "AcousticsEdMode.h"
#include "Runtime/Launch/Resources/Version.h"
#include "AcousticsEdModeToolkit.h"
#include "AcousticsEditActions.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "Landscape.h"
#include "Engine/StaticMeshActor.h"
#include "Interfaces/IPluginManager.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Navmesh/RecastNavMesh.h"
#include "AcousticsShared.h"

const FEditorModeID FAcousticsEdMode::EM_AcousticsEdModeId = TEXT("EM_AcousticsEdMode");

DEFINE_LOG_CATEGORY(LogAcoustics)

FAcousticsEdMode::FAcousticsEdMode()
{
    FAcousticsEditCommands::Register();

    UICommandList = MakeShareable(new FUICommandList);
    BindCommands();

    AcousticsUISettings.CurrentTab = AcousticsActiveTab::ObjectTag;
    AcousticsUISettings.ObjectsTabSettings.IsNavMeshChecked = false;
    AcousticsUISettings.ObjectsTabSettings.IsLandscapeChecked = false;
    AcousticsUISettings.ObjectsTabSettings.IsStaticMeshChecked = false;
    AcousticsUISettings.ObjectsTabSettings.IsMovableStaticMeshChecked = false;
    AcousticsUISettings.ObjectsTabSettings.IsAcousticsRadioButtonChecked = true;
    AcousticsUISettings.ObjectsTabSettings.IsNavigationRadioButtonChecked = false;
}

FAcousticsEdMode::~FAcousticsEdMode()
{
}

void FAcousticsEdMode::Enter()
{
    FEdMode::Enter();

    if (!Toolkit.IsValid() && UsesToolkits())
    {
        Toolkit = MakeShareable(new FAcousticsEdModeToolkit);
        Toolkit->Init(Owner->GetToolkitHost());
    }
}

void FAcousticsEdMode::Exit()
{
    if (Toolkit.IsValid())
    {
        FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
        Toolkit.Reset();
    }

    // Call base Exit method to ensure proper cleanup
    FEdMode::Exit();
}

bool FAcousticsEdMode::UsesToolkits() const
{
    return true;
}

void FAcousticsEdMode::BindCommands()
{
    const FAcousticsEditCommands& Commands = FAcousticsEditCommands::Get();

    UICommandList->MapAction(
        Commands.SetObjectTag,
        FExecuteAction::CreateRaw(this, &FAcousticsEdMode::OnClickObjectTab),
        FCanExecuteAction(),
        FIsActionChecked::CreateLambda(
            [this] { return AcousticsUISettings.CurrentTab == AcousticsActiveTab::ObjectTag; }));

    UICommandList->MapAction(
        Commands.SetMaterials,
        FExecuteAction::CreateRaw(this, &FAcousticsEdMode::OnClickMaterialsTab),
        FCanExecuteAction(),
        FIsActionChecked::CreateLambda(
            [this] { return AcousticsUISettings.CurrentTab == AcousticsActiveTab::Materials; }));

    UICommandList->MapAction(
        Commands.SetProbes,
        FExecuteAction::CreateRaw(this, &FAcousticsEdMode::OnClickProbesTab),
        FCanExecuteAction(),
        FIsActionChecked::CreateLambda([this] { return AcousticsUISettings.CurrentTab == AcousticsActiveTab::Probes; }));

    UICommandList->MapAction(
        Commands.SetBake,
        FExecuteAction::CreateRaw(this, &FAcousticsEdMode::OnClickBakeTab),
        FCanExecuteAction(),
        FIsActionChecked::CreateLambda([this] { return AcousticsUISettings.CurrentTab == AcousticsActiveTab::Bake; }));
}

void FAcousticsEdMode::SelectObjects()
{
    GEditor->SelectNone(true, true, false);

    UWorld* world = GEditor->GetEditorWorldContext().World();
    ULevel* currentLevel = world->GetCurrentLevel();
    const int32 numLevels = world->GetNumLevels();

    for (int32 LevelIdx = 0; LevelIdx < numLevels; ++LevelIdx)
    {
        ULevel* level = world->GetLevel(LevelIdx);
        if (level != nullptr && level->bIsVisible)
        {
            // Add all objects that fit a filter to some internal list
            auto actors = level->Actors;
            for (const auto& actor : actors)
            {
                // Sometimes actors don't exist for some reason. Just skip them
                if (actor == nullptr)
                {
                    continue;
                }
                bool shouldSelectActor = false;
                if (AcousticsUISettings.ObjectsTabSettings.IsStaticMeshChecked)
                {
                    // Instead of checking if the actor is a static mesh actor, loop through all the
                    // static mesh components to check if they have static mobility and select the actor if it does.
                    TArray<UStaticMeshComponent*> staticMeshComponents;
                    actor->GetComponents<UStaticMeshComponent>(staticMeshComponents, true);
                    for (UStaticMeshComponent* const& meshComponent : staticMeshComponents)
                    {
                        if (meshComponent != nullptr)
                        {
                            // If MovableStaticMeshChecked, then we accept everything. Otherwise, we only accept !Movable (Static and Stationary)
                            if (AcousticsUISettings.ObjectsTabSettings.IsMovableStaticMeshChecked || meshComponent->Mobility != EComponentMobility::Movable)
                            {
                                shouldSelectActor = true;
                                break;
                            }
                        }
                    }
                }
                if (AcousticsUISettings.ObjectsTabSettings.IsLandscapeChecked)
                {
                    if (actor->IsA<ALandscapeProxy>())
                    {
                        shouldSelectActor = true;
                    }
                }
                if (AcousticsUISettings.ObjectsTabSettings.IsNavMeshChecked)
                {
                    if (actor->IsA<ARecastNavMesh>())
                    {
                        shouldSelectActor = true;
                    }
                }

                if (shouldSelectActor)
                {
                    GEditor->SelectActor(actor, true, false, true, false);
                }
            }
        }
    }

    GEditor->NoteSelectionChange();
}

// Returns true if all tags were able to be set, false if there was a problem with one or more tags
bool FAcousticsEdMode::TagGeometry(bool tag)
{
    bool allTagsSet = true;
    for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
    {
        AActor* actor = *ActorItr;
        if (actor->IsSelected())
        {
            // Always remove any previously set tags. This prevents multiple-tagging
            actor->Tags.Remove(c_AcousticsGeometryTag);
            actor->Modify();
            if (tag)
            {
                if (actor->IsA(ARecastNavMesh::StaticClass()))
                {
                    allTagsSet = false;
                    UE_LOG(
                        LogAcoustics,
                        Error,
                        TEXT("Attempted to add Geometry tag to %s, which is a Nav Mesh. This is not "
                             "supported. Skipping tag."),
                        *(actor->GetName()));
                    continue;
                }
                actor->Tags.Add(c_AcousticsGeometryTag);
            }
        }
    }

    return allTagsSet;
}

// Returns true if all tags were able to be set, false if there was a problem with one or more tags
bool FAcousticsEdMode::TagNavigation(bool tag)
{
    bool allTagsSet = true;
    for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
    {
        AActor* actor = *ActorItr;
        if (actor->IsSelected())
        {
            // Always remove any previously set tags. This prevents multiple-tagging
            actor->Tags.Remove(c_AcousticsNavigationTag);
            actor->Modify();
            if (tag)
            {
                // Do a safety check for the user to make sure they don't bake a ridiculously large mesh.
                // Use the magnitude of the size of the bounding box, because this helps handle the case
                // where we have a 2D plane, and one of the dimensions is 0.
                auto actorSize = actor->GetComponentsBoundingBox(true, true).GetSize().Size();
                if (actorSize > c_NavigationActorSizeWarning)
                {
                    auto consent = EAppReturnType::Ok;

                    UE_LOG(
                        LogAcoustics,
                        Warning,
                        TEXT("Warning: A very large mesh (%s) was tagged for Acoustic Navigation. This may result in a "
                             "very long probe calculation time. Make sure "
                             "you haven't accidentally tagged a huge mesh like SkySphere."),
                        *actor->GetName());
                }

                if (actor->IsA<ANavMeshBoundsVolume>())
                {
                    allTagsSet = false;
                    UE_LOG(
                        LogAcoustics,
                        Error,
                        TEXT("Attempted to tag a NavMeshBoundsVolume as Navigation, which is not supported. Please tag "
                             "the RecastNavMesh actor as Navigation."),
                        *(actor->GetName()));
                    continue;
                }

                actor->Tags.Add(c_AcousticsNavigationTag);
            }
        }
    }

    return allTagsSet;
}

// Gets the FConfigFile and FString assocaited with the ProjectAcoustics config file
// The config file stores material properties
// On success, returns true and both parameters are populated
// On failure, returns false and neither parameter is changed
bool FAcousticsEdMode::GetConfigFile(FConfigFile** configFile, FString& configFilePath)
{
    // If the config file is not initialized, deserialize it from the plugin's config directly
    // If it has already been read into memory, just return it below
    if (!m_ConfigFile.Name.IsValid() || m_ConfigFilePath.IsEmpty())
    {
        static TSharedPtr<IPlugin> ProjectAcousticsPlugin = IPluginManager::Get().FindPlugin(c_PluginName);
        if (!ProjectAcousticsPlugin.IsValid())
        {
            return false;
        }
        m_ConfigFilePath = GConfig->GetDestIniFilename(
            *c_PluginName, nullptr, *FPaths::Combine(ProjectAcousticsPlugin->GetBaseDir(), TEXT("Config/")));
        m_ConfigFile.Read(m_ConfigFilePath);
    }
    *configFile = &m_ConfigFile;
    configFilePath = m_ConfigFilePath;
    return true;
}

bool FAcousticsEdMode::ShouldUsePhysicalMaterial(const class UPhysicalMaterial* physicalMaterial) const
{
	if (physicalMaterial != nullptr)
	{
		return UsePhysicalMaterials && physicalMaterial != GEngine->DefaultPhysMaterial;
	}
	return false;
}