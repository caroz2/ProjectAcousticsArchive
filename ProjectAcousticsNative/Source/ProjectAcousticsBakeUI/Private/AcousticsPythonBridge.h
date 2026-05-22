// Copyright (c) 2022 Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once
#include "UObject/Object.h"
#include "AcousticsPythonBridge.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Azure Account"))
struct FAzureCredentials
{
    GENERATED_BODY()

    // clang-format off
    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta = (DisplayName = "Batch Account Name", tooltip = "Name of Azure Batch account from Azure Portal"),
        Category = "Azure Account")
    FString batch_name;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta = (DisplayName = "Batch Account Endpoint", tooltip = "Endpoint for the Azure Batch account from Azure Portal"),
        Category = "Azure Account")
    FString batch_url;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta = (DisplayName = "Batch Access Key", tooltip = "Batch account access key from Azure Portal"),
        Category = "Azure Account")
    FString batch_key;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Storage Account Name",
             tooltip = "Name of the Azure storage account associated with the Azure Batch account on Azure Portal"),
        Category = "Azure Account")
    FString storage_name;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Storage Key",
             tooltip = "Key for the Azure storage account associated with the Azure Batch account on Azure Portal"),
        Category = "Azure Account")
    FString storage_key;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Project Acoustics Toolset",
             tooltip = "Specific version of Microsoft Project Acoustics used for simulation processing"),
        Category = "Azure Account")
    FString toolset_version;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Custom Azure Container Login Server",
             tooltip = "If using non-default Project Acoustics Toolset, specify the login server used to log into the Azure Container where it is hosted. Otherwise, leave blank"),
        Category = "Azure Account")
    FString acr_server;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Custom Azure Container Username",
             tooltip = "If using non-default Project Acoustics Toolset, the username to use for authentication to the custom container registry. Otherwise, leave blank"),
        Category = "Azure Account")
    FString acr_account;

    UPROPERTY(
        EditAnywhere, BlueprintReadWrite, NonTransactional, Transient,
        meta =
            (DisplayName = "Custom Azure Container Password",
             tooltip = "If using non-default Project Acoustics Toolset, the password to use for authentication to the custom container registry. Otherwise, leave blank"),
        Category = "Azure Account")
    FString acr_key;
    // clang-format on
};

USTRUCT(BlueprintType)
struct FProbeSampling
{
    GENERATED_BODY()

    /**
    * Minimum horizontal distance allowed between probes in centimeters.
    * 
    * This should be modified with care and not made very large. For example, you could perhaps increase 
    * this to 100cm if you are confident that your map will be lacking narrower features that the player can walk through.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 25, UIMin = 50, UIMax = 200, ClampMax = 2000, DisplayName = "Min Horizontal Probe Spacing (cm)"))
    float horizontal_spacing_min = 0.0f;

    /**
    * Maximum horizontal distance allowed between probes in centimeters.
    * 
    * This is the primary control for bake cost and data size, both of which scale linearly with  
    * number of probes; each probe yields a simulation task, whose data is concatenated in 
    * the final ACE file. We have tested this parameter increased to ~10m in big outdoor areas.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 25, UIMin = 50, UIMax = 1000, ClampMax = 2000, DisplayName = "Max Horizontal Probe Spacing (cm)"))
    float horizontal_spacing_max = 0.0f;

    /**
    * Vertical distance separating probes in centimeters
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 0, UIMin = 10, UIMax = 10000, ClampMax = 100000, DisplayName = "Vertical Probe Spacing (cm)"))
    float vertical_spacing = 0.0f;

    /**
    * Minimum distance from the ground at which probes should be placed in centimeters
    * 
    * This is typically chosen to range around human height, but if you are say, modeling a mech, this can be increased correspondingly.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 0, UIMin = 10, UIMax = 10000, ClampMax = 100000, DisplayName = "Min Probe Height Above Ground (cm)"))
    float min_height_above_ground = 0.0f;
};

USTRUCT(BlueprintType)
struct FSimulationParameters
{
    GENERATED_BODY()

    /**
    * Multiplicative adjustment to scale the mesh's unit system to meters. This applies in addition to the automatic 
    * conversion from Unreal performed by PA.
    */
    UPROPERTY(BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 0, UIMin = 0, DisplayName = "Mesh Unit Adjustment"))
    float mesh_unit_adjustment = 0.0f;

    /** 
    * This is the top frequency in Hertz used for wave simulation. It is used to determine the voxel resolution. For a value of 
    * 500Hz, the voxel size is 25 centimeters, with linear scaling. That is, at 250Hz, the voxel size is 51cm and so on. This is
    * also a factor in determing when the simulation terminates.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 50, UIMin = 250, UIMax = 1000, ClampMax = 10000,  DisplayName = "Max Frequency (Hz)"))
    int max_frequency = 0;

    /**
    * Sampling resolution of parameter field in centimeters
    * 
    * For each probe, the volumetric simulated data is sampled on a regularly-spaced 3D grid. This is the spacing of that grid, which
    * is internally discretized to an integer number of simulation voxels. It is the spatial resolution of acoustic interpolation for
    * sound sources at runtime.
    *
    * Default is 1500cm as a practical compromise. Smaller values increase ACE size, with some impact on CPU/RAM as more data must be
    * decompressed at runtime for moving sources. There is no impact on bake time. Larger values can yield a smaller dataset with the
    * risk of undersampling, such as the "fireplace bug" - where a sound source inside a cavity happens to have receiver samples straddle
    * the cavity with no sample inside. With geometry-aware interpolation, all samples may be discarded, failing the query.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (ClampMin = 25, UIMin = 50, UIMax = 200, ClampMax = 2000, DisplayName = "Receiver Spacing (cm)"))
    float receiver_spacing = 0.0f;

    /**
    * These settings inform the global adaptive probe placement algorithm. The algorithm adapts the probe spacing based on how cramped or
    * open the surroundings are in various regions of the map. The rails on these resolution variations are provided by 
    * HorizontalSpacingMin and HorizontalSpacingMax respectively. Set the former based on the most cramped areas such as width of narrow 
    * corridors you wish to resolve. Set the latter based on how coarse you can go in wide open areas. Vertically, the layout algorithm will 
    * create a single layer of probes that are within the height range (ProbeMinHeightAboveGround, ProbeMinHeightAboveGround + VerticalSpacing) 
    * above the nav mesh.
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (DisplayName = "Probe Spacing"))
    FProbeSampling probe_spacing;

    /**
    * Bounding box to control simulation region around a probe (in centimeters).
    * 
    * Each probe performs a simulation on a limited volume of space. Thus runtime sources' acoustics are accurately modeled only up to some 
    * distance. Beyond this distance, the runtime extrapolates assuming that everything is air. The "Min" and "Max" values specify the corners 
    * of this simulation box, relative to the probe location
    */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, NonTransactional, Transient, Category = "Acoustics", meta =
        (DisplayName = "Simulation Region"))
    FBox simulation_region = FBox(ForceInitToZero);
};

USTRUCT(BlueprintType)
struct FComputePoolConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString vm_size;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    int nodes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool use_low_priority_nodes = false;
};

USTRUCT(BlueprintType)
struct FJobConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    int probe_count = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString vox_file;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString config_file;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString prefix;
};

USTRUCT(BlueprintType)
struct FProjectConfiguration
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    TMap<FString, FString> level_prefix_map;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString plugins_dir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString content_dir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString game_content_dir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString config_dir;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString log_dir;
};

USTRUCT(BlueprintType)
struct FActiveJobInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString job_id;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString job_id_prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString submit_time;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool submit_pending = false;
};

USTRUCT(BlueprintType)
struct FAzureCallStatus
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    bool succeeded = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FString message;
};

UCLASS(Blueprintable)
class UAcousticsPythonBridge : public UObject
{
    GENERATED_BODY()

    // Added edit mode for source control access.
private:
    class FAcousticsEdMode* m_AcousticsEditMode;

public:
    UFUNCTION(BlueprintCallable, Category = Python)
    static UAcousticsPythonBridge* Get();

    void Initialize();

    // These methods are projected up to Python.
    // Please use snake_case for all projected members.
    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void initialize_projection() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void load_configuration() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void save_configuration() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void update_azure_credentials() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    float estimate_processing_time() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void submit_for_processing() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void cancel_job() const;

    UFUNCTION(BlueprintImplementableEvent, Category = Python)
    void update_job_status() const;

    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    void create_ace_asset(FString acePath);

    UFUNCTION(BlueprintCallable, Category = "Acoustics")
    void show_readonly_ace_dialog();

    void SetProjectConfiguration(const FProjectConfiguration& config);
    const FProjectConfiguration& GetProjectConfiguration() const
    {
        return project_config;
    }

    void SetAzureCredentials(const FAzureCredentials& creds);
    const FAzureCredentials& GetAzureCredentials() const
    {
        return azure_credentials;
    }

    void SetSimulationParameters(const FSimulationParameters& config);
    const FSimulationParameters& GetSimulationParameters() const
    {
        return simulation_parameters;
    }

    const FSimulationParameters& GetDefaultSimulationParameters() const
    {
        return default_simulation_parameters;
    }

    void SetComputePoolConfiguration(const FComputePoolConfiguration& config);
    const FComputePoolConfiguration& GetComputePoolConfiguration() const
    {
        return compute_pool_configuration;
    }

    void SetJobConfiguration(const FJobConfiguration& config);
    const FJobConfiguration& GetJobConfiguration() const
    {
        return job_configuration;
    }

    const FActiveJobInfo& GetActiveJobInfo() const
    {
        return active_job_info;
    }

    const FAzureCallStatus& GetCurrentStatus()
    {
        // Update and return latest status
        update_job_status();
        return current_status;
    }

    // Unreal's Python reflection will crash if passing parameters to reflected functions.
    // Workaround is to setup public reflected properties instead.
    // Please use Accessors/Mutators instead of direct access, if/when UE fixes
    // this issue the only change required would be to make these properties private.
    // Python reflection also requires the member names to be in lower_snake_case format.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FProjectConfiguration project_config;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FAzureCredentials azure_credentials;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FSimulationParameters simulation_parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FSimulationParameters default_simulation_parameters;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FComputePoolConfiguration compute_pool_configuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FJobConfiguration job_configuration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FActiveJobInfo active_job_info;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
    FAzureCallStatus current_status;
};