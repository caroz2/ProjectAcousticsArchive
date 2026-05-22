# Copyright (c) 2022 Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import unreal
import os
import sys
import datetime
import tempfile
import json
import copy
try:
    # Python 2 support
    import ConfigParser as configparser
except ImportError:
    # Python 3 support
    import configparser
import shutil
import threading
from subprocess import call

search_path = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)), "..\..\Source\ThirdParty\Win64\Release"))
sys.path.append(search_path)

# Make sure clr is available
try:
    import clr
except ImportError:
    # clr not available. Go get it
    dialogText = "Project Acoustics is performing first run setup for required python dependencies. This may take a minute."
    with unreal.ScopedSlowTask(4, dialogText) as slow_task:
        slow_task.make_dialog(True)
        pythonDir = os.__file__ + "/../.."
        pythonExe = pythonDir + "/python.exe"
        CREATE_NO_WINDOW = 0x08000000
        call([pythonExe, '-m', 'ensurepip'], creationflags=CREATE_NO_WINDOW)
        slow_task.enter_progress_frame(1)
        call([pythonExe, '-m', 'pip', 'install', '--upgrade', 'pip'], creationflags=CREATE_NO_WINDOW)
        slow_task.enter_progress_frame(1)
        call([pythonExe, '-m', 'pip', 'install', '--upgrade', 'wheel'], creationflags=CREATE_NO_WINDOW)
        slow_task.enter_progress_frame(1)
        call([pythonExe, '-m', 'pip', 'install', '--upgrade', 'pythonnet'], creationflags=CREATE_NO_WINDOW)
        slow_task.enter_progress_frame(1)
    import clr

# CLR imports
from System import AppDomain, Convert, String 
from System.Text import Encoding

clr.AddReference("System.Security")
from System.Security.Cryptography import ProtectedData, DataProtectionScope

clr.AddReference("Microsoft.Azure.Batch")
from Microsoft.Azure.Batch import ContainerRegistry

clr.AddReference("Triton.BakeService")
from Microsoft.Cloud.Acoustics import *


def decrypt_azure_creds(creds_key):
    # decrypt creds
    azure_creds_bytes = Convert.FromBase64String(creds_key)
    azure_creds_bytes_unprotected = ProtectedData.Unprotect(azure_creds_bytes, None, DataProtectionScope.CurrentUser)
    azure_creds_unprotected = Encoding.Unicode.GetString(azure_creds_bytes_unprotected)
    json_decrypted = json.loads(azure_creds_unprotected)
    return json_decrypted


def encrypt_azure_creds(batch_key, storage_key, registry_key):
    # encrypt creds before saving
    json_encoded = json.dumps({'batch_key':batch_key, 'storage_key': storage_key, 'registry_key':registry_key})
    new_creds_bytes = Encoding.Unicode.GetBytes(json_encoded)
    new_creds_bytes_protected = ProtectedData.Protect(new_creds_bytes, None, DataProtectionScope.CurrentUser)
    new_creds_protected = Convert.ToBase64String(new_creds_bytes_protected)
    return new_creds_protected


def get_formatted_time(duration):
    total_time = ""
    if (duration.Days > 0):
        total_time = "{0} day{1} ".format(duration.Days, "" if (duration.Days == 1) else "s")
    if (duration.Hours > 0 or duration.Days > 0):
        total_time += "{0} hour{1} ".format(duration.Hours, "" if (duration.Hours == 1) else "s")
    total_time += "{0} minute{1}".format(duration.Minutes, "" if (duration.Minutes == 1) else "s")
    return total_time


class AcousticsConfigSection(object):
    def __init__(self):
        self.values = dict()

    def __getitem__(self, key):
        if key not in self.values:
            return ''
        return self.values[key]

    def __setitem__(self, key, value):
        self.values[key] = value

    def get_as_float(self, key, default):
        value = self.__getitem__(key)
        if not value:
            value = default
        return float(value)

class AzureInformationConfig(AcousticsConfigSection):
    azure_account_config_section = 'azure_account'
    azure_account_config_url = 'url'
    azure_account_config_batch_name = 'batch_name'
    azure_account_config_batch_key = 'batch_key'
    azure_account_config_storage_key = 'storage_key'
    azure_account_config_storage_name = 'storage_name'
    azure_account_config_secret = 'secret'
    azure_account_config_toolset_version = 'toolset_version'
    default_toolset_version = 'mcr.microsoft.com/acoustics/baketools:2022.1.Linux'
    azure_container_registry_server = 'registry_server'
    azure_container_registry_account = 'registry_account'
    azure_container_registry_key = 'registry_key'

    def __init__(self, config):
        super(AzureInformationConfig, self).__init__()
        self.load(config)

    def load(self, config):
        if config.has_section(AzureInformationConfig.azure_account_config_section):
            azure_values = dict(config.items(AzureInformationConfig.azure_account_config_section))

            for key in azure_values.keys():
                if key == AzureInformationConfig.azure_account_config_secret:
                    encrypted_keys = azure_values[key]
                    try:
                        json_decrypted = decrypt_azure_creds(encrypted_keys)
                        self.values[AzureInformationConfig.azure_account_config_batch_key] = json_decrypted[AzureInformationConfig.azure_account_config_batch_key]
                        self.values[AzureInformationConfig.azure_account_config_storage_key] = json_decrypted[AzureInformationConfig.azure_account_config_storage_key]
                        self.values[AzureInformationConfig.azure_container_registry_key] = json_decrypted[AzureInformationConfig.azure_container_registry_key]
                    except Exception as error:
                        unreal.log_error("The saved Project Acoustics Bake Azure Account are invalid and must be re-entererd.")
                        unreal.log_error(error)
                else:
                    self.values[key] = azure_values[key]


    def save(self, config):
        config.add_section(AzureInformationConfig.azure_account_config_section)
        values_copy = copy.deepcopy(self.values)

        # don't persist clear text secrets
        if AzureInformationConfig.azure_account_config_batch_key in values_copy:
            values_copy.pop(AzureInformationConfig.azure_account_config_batch_key, None)
        if AzureInformationConfig.azure_account_config_storage_key in values_copy:
            values_copy.pop(AzureInformationConfig.azure_account_config_storage_key, None)
        if AzureInformationConfig.azure_container_registry_key in values_copy:
            values_copy.pop(AzureInformationConfig.azure_container_registry_key, None)

        if AzureInformationConfig.azure_account_config_batch_key in self.values and AzureInformationConfig.azure_account_config_storage_key in self.values:
            if self.values[AzureInformationConfig.azure_account_config_batch_key] and self.values[AzureInformationConfig.azure_account_config_storage_key]:
                encrypted_keys = encrypt_azure_creds(self.values[AzureInformationConfig.azure_account_config_batch_key], self.values[AzureInformationConfig.azure_account_config_storage_key], self.values[AzureInformationConfig.azure_container_registry_key])
                values_copy[AzureInformationConfig.azure_account_config_secret] = encrypted_keys
        
        for key in values_copy.keys():
            config.set(AzureInformationConfig.azure_account_config_section, key, values_copy[key])

    @property
    def url(self):
        return self.__getitem__(AzureInformationConfig.azure_account_config_url)

    @url.setter
    def url(self, value):
        self.values[AzureInformationConfig.azure_account_config_url] = value

    @property
    def batch_name(self):
        return self.__getitem__(AzureInformationConfig.azure_account_config_batch_name)

    @batch_name.setter
    def batch_name(self, value):
        self.values[AzureInformationConfig.azure_account_config_batch_name] = value

    @property
    def batch_key(self):
        return self.__getitem__(AzureInformationConfig.azure_account_config_batch_key)

    @batch_key.setter
    def batch_key(self, value):
        self.values[AzureInformationConfig.azure_account_config_batch_key] = value

    @property
    def storage_name(self):
        return self.__getitem__(AzureInformationConfig.azure_account_config_storage_name)

    @storage_name.setter
    def storage_name(self, value):
        self.values[AzureInformationConfig.azure_account_config_storage_name] = value

    @property
    def storage_key(self):
        return self.__getitem__(AzureInformationConfig.azure_account_config_storage_key)

    @storage_key.setter
    def storage_key(self, value):
        self.values[AzureInformationConfig.azure_account_config_storage_key] = value

    @property
    def toolset_version(self):
        value = self.__getitem__(AzureInformationConfig.azure_account_config_toolset_version)
        # Prefer the default toolset version if preferred is not specified
        if (not value):
            value = self.default_toolset_version
        return value

    @toolset_version.setter
    def toolset_version(self, value):
        self.values[AzureInformationConfig.azure_account_config_toolset_version] = value

    @property
    def acr_server(self):
        return self.__getitem__(AzureInformationConfig.azure_container_registry_server)

    @acr_server.setter
    def acr_server(self, value):
        self.values[AzureInformationConfig.azure_container_registry_server] = value

    @property
    def acr_account(self):
        return self.__getitem__(AzureInformationConfig.azure_container_registry_account)

    @acr_account.setter
    def acr_account(self, value):
        self.values[AzureInformationConfig.azure_container_registry_account] = value

    @property
    def acr_key(self):
        return self.__getitem__(AzureInformationConfig.azure_container_registry_key)

    @acr_key.setter
    def acr_key(self, value):
        self.values[AzureInformationConfig.azure_container_registry_key] = value

class SimulationParametersConfig(AcousticsConfigSection):
    simulation_parameters_section = 'simulation_parameters'
    simulation_parameters_mesh_unit_adjustment = 'mesh_unit_adjustment'
    simulation_parameters_max_frequency = 'max_frequency'
    simulation_parameters_receiver_spacing = 'receiver_spacing'
    simulation_parameters_probe_horizontal_spacing_min = 'probe_horizontal_spacing_min'
    simulation_parameters_probe_horizontal_spacing_max = 'probe_horizontal_spacing_max'
    simulation_parameters_probe_vertical_spacing = 'probe_vertical_spacing'
    simulation_parameters_probe_min_height_above_ground = 'probe_min_height_above_ground'
    simulation_parameters_probe_region_lower = 'simulation_region_lower'
    simulation_parameters_probe_region_upper = 'simulation_region_upper'
    default_float_value = '-1.0'

    def __init__(self, config):
        super(SimulationParametersConfig, self).__init__()
        self.load(config)

    def load(self, config):
        if config.has_section(SimulationParametersConfig.simulation_parameters_section):
            self.values = dict(config.items(SimulationParametersConfig.simulation_parameters_section))

    def save(self, config):
        config.add_section(SimulationParametersConfig.simulation_parameters_section)
        for key in self.values.keys():
            config.set(SimulationParametersConfig.simulation_parameters_section, key, self.values[key])

    def vector_from_list(self, list):
        values = [float(token) for token in str(list).strip('[]').split(',')]
        return unreal.Vector(values[0], values[1], values[2])

    @property
    def mesh_unit_adjustment(self):
        return self.get_as_float(SimulationParametersConfig.simulation_parameters_mesh_unit_adjustment, SimulationParametersConfig.default_float_value)

    @mesh_unit_adjustment.setter
    def mesh_unit_adjustment(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_mesh_unit_adjustment] = str(value)

    @property
    def max_frequency(self):
        return self.get_as_float(SimulationParametersConfig.simulation_parameters_max_frequency, SimulationParametersConfig.default_float_value)

    @max_frequency.setter
    def max_frequency(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_max_frequency] = str(value)

    @property
    def receiver_spacing(self):
        return self.get_as_float(SimulationParametersConfig.simulation_parameters_receiver_spacing, SimulationParametersConfig.default_float_value)

    @receiver_spacing.setter
    def receiver_spacing(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_receiver_spacing] = str(value)

    @property
    def probe_horizontal_spacing_min(self):
        return float(self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_horizontal_spacing_min))

    @probe_horizontal_spacing_min.setter
    def probe_horizontal_spacing_min(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_horizontal_spacing_min] = str(value)

    @property
    def probe_horizontal_spacing_max(self):
        return float(self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_horizontal_spacing_max))

    @probe_horizontal_spacing_max.setter
    def probe_horizontal_spacing_max(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_horizontal_spacing_max] = str(value)

    @property
    def probe_vertical_spacing(self):
        return float(self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_vertical_spacing))

    @probe_vertical_spacing.setter
    def probe_vertical_spacing(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_vertical_spacing] = str(value)

    @property
    def probe_min_height_above_ground(self):
        return float(self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_min_height_above_ground))

    @probe_min_height_above_ground.setter
    def probe_min_height_above_ground(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_min_height_above_ground] = str(value)

    @property
    def simulation_region_lower(self):
        list = self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_region_lower)
        return self.vector_from_list(list)

    @simulation_region_lower.setter
    def simulation_region_lower(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_region_lower] = [value.x, value.y, value.z]

    @property
    def simulation_region_upper(self):
        list = self.__getitem__(SimulationParametersConfig.simulation_parameters_probe_region_upper)
        return self.vector_from_list(list)

    @simulation_region_upper.setter
    def simulation_region_upper(self, value):
        self.values[SimulationParametersConfig.simulation_parameters_probe_region_upper] = [value.x, value.y, value.z]

class ComputePoolConfig(AcousticsConfigSection):
    compute_pool_config_section = 'compute_pool'
    compute_pool_config_vm_size = 'vm_size'
    compute_pool_config_nodes = 'nodes'
    compute_pool_config_use_lowpri_nodes = 'use_lowpri_nodes'

    def __init__(self, config):
        super(ComputePoolConfig, self).__init__()
        self.load(config)

    def load(self, config):
        if config.has_section(ComputePoolConfig.compute_pool_config_section):
            self.values = dict(config.items(ComputePoolConfig.compute_pool_config_section))

    def save(self, config):
        config.add_section(ComputePoolConfig.compute_pool_config_section)
        for key in self.values.keys():
            config.set(ComputePoolConfig.compute_pool_config_section, key, self.values[key])

    @property
    def vm_size(self):
        return self.__getitem__(ComputePoolConfig.compute_pool_config_vm_size)

    @vm_size.setter
    def vm_size(self, value):
        self.values[ComputePoolConfig.compute_pool_config_vm_size] = value

    @property
    def nodes(self):
        return float(self.__getitem__(ComputePoolConfig.compute_pool_config_nodes))

    @nodes.setter
    def nodes(self, value):
        self.values[ComputePoolConfig.compute_pool_config_nodes] = str(value)

    @property
    def use_lowpri_nodes(self):
        return (self.__getitem__(ComputePoolConfig.compute_pool_config_use_lowpri_nodes) == 'True')

    @use_lowpri_nodes.setter
    def use_lowpri_nodes(self, value):
        self.values[ComputePoolConfig.compute_pool_config_use_lowpri_nodes] = str(value)


class OperationParametersConfig(AcousticsConfigSection):
    operational_parameters_section = 'operational_parameters'
    operational_parameters_level_prefix_map = 'level_prefix_map'
    operational_parameters_data_path = 'acoustics_data_path'
    operational_parameters_active_job_id = 'job_id'
    operational_parameters_active_job_prefix = 'job_prefix'
    operational_parameters_active_job_submit_time = 'job_submit_time'

    def __init__(self, config):
        super(OperationParametersConfig, self).__init__()
        self.load(config)

    def load(self, config):
        if config.has_section(OperationParametersConfig.operational_parameters_section):
            self.values = dict(config.items(OperationParametersConfig.operational_parameters_section))

    def save(self, config):
        config.add_section(OperationParametersConfig.operational_parameters_section)
        for key in self.values.keys():
            config.set(OperationParametersConfig.operational_parameters_section, key, self.values[key])

    @property
    def level_prefix_map(self):
        value = self.__getitem__(OperationParametersConfig.operational_parameters_level_prefix_map)
        if (value):
            return json.loads(value)
        # Return empty dictionary if missing config value 
        else:
            return dict()

    @level_prefix_map.setter
    def level_prefix_map(self, value):
        self.values[OperationParametersConfig.operational_parameters_level_prefix_map] = value

    @property
    def data_folder(self):
        return self.__getitem__(OperationParametersConfig.operational_parameters_data_path)

    @data_folder.setter
    def data_folder(self, value):
        self.values[OperationParametersConfig.operational_parameters_data_path] = value

    @property
    def job_id(self):
        return self.__getitem__(OperationParametersConfig.operational_parameters_active_job_id)

    @job_id.setter
    def job_id(self, value):
        self.values[OperationParametersConfig.operational_parameters_active_job_id] = value

    @property
    def job_prefix(self):
        return self.__getitem__(OperationParametersConfig.operational_parameters_active_job_prefix)

    @job_prefix.setter
    def job_prefix(self, value):
        self.values[OperationParametersConfig.operational_parameters_active_job_prefix] = value

    @property
    def job_submit_time(self):
        return self.__getitem__(OperationParametersConfig.operational_parameters_active_job_submit_time)

    @job_submit_time.setter
    def job_submit_time(self, value):
        self.values[OperationParametersConfig.operational_parameters_active_job_submit_time] = value


class AcousticsConfiguration:

    def __init__(self, config_filepath):
        self.config_filepath = config_filepath
        self.load()

    def load(self):
        config = configparser.RawConfigParser()
        if (os.path.isfile(self.config_filepath)):
            # if the config file exists, read info from each section
            config.read(self.config_filepath)
            
        self.azure_creds = AzureInformationConfig(config)
        self.simulation_parameters = SimulationParametersConfig(config)
        self.compute_pool = ComputePoolConfig(config)
        self.operational_parameters = OperationParametersConfig(config)

    def save(self):
        # create config
        config = configparser.RawConfigParser()
        # add sections
        self.azure_creds.save(config)
        self.simulation_parameters.save(config)
        self.compute_pool.save(config)
        self.operational_parameters.save(config)
        # save file
        with open(self.config_filepath, 'w') as config_file:
            config.write(config_file)


@unreal.uclass()
class AcousticsPythonBridgeImplmentation(unreal.AcousticsPythonBridge):
    project_acoustics_config_file = 'ProjectAcoustics.cfg'
    project_acoustics_default_config_file = 'ProjectAcousticsDefault.cfg'

    fine_bake_cost_sheet = 'FineBakeCostSheet.xml'
    coarse_bake_cost_sheet = 'CoarseBakeCostSheet.xml'
    config_path = ''
    lock = threading.Lock()
    submit_monitor = None
    download_ace_monitor = None

    @unreal.ufunction(override=True)
    def initialize_projection(self):
        configPath = os.path.join(self.project_config.config_dir, AcousticsPythonBridgeImplmentation.project_acoustics_config_file)

        defaultConfig = os.path.join(os.path.dirname(os.path.realpath(__file__)),
            "../../Resources",
            AcousticsPythonBridgeImplmentation.project_acoustics_default_config_file)
        if not os.path.isfile(defaultConfig):
            unreal.log_error("Default configuration missing. Check your Project Acoustics plugin installation. Expected to find: " + defaultConfig)
            return

        # Save the default simulation parameters so that we can restore them at will
        default_acoustics_configuration =  AcousticsConfiguration(defaultConfig)
        self.default_simulation_parameters.mesh_unit_adjustment = default_acoustics_configuration.simulation_parameters.mesh_unit_adjustment
        self.default_simulation_parameters.max_frequency = default_acoustics_configuration.simulation_parameters.max_frequency
        self.default_simulation_parameters.receiver_spacing = default_acoustics_configuration.simulation_parameters.receiver_spacing 
        self.default_simulation_parameters.probe_spacing.horizontal_spacing_min = default_acoustics_configuration.simulation_parameters.probe_horizontal_spacing_min
        self.default_simulation_parameters.probe_spacing.horizontal_spacing_max = default_acoustics_configuration.simulation_parameters.probe_horizontal_spacing_max 
        self.default_simulation_parameters.probe_spacing.vertical_spacing = default_acoustics_configuration.simulation_parameters.probe_vertical_spacing
        self.default_simulation_parameters.probe_spacing.min_height_above_ground = default_acoustics_configuration.simulation_parameters.probe_min_height_above_ground
        self.default_simulation_parameters.simulation_region.min = default_acoustics_configuration.simulation_parameters.simulation_region_lower 
        self.default_simulation_parameters.simulation_region.max = default_acoustics_configuration.simulation_parameters.simulation_region_upper 

        if os.path.isfile(configPath):
            AcousticsPythonBridgeImplmentation.config_path = configPath
        else:
            AcousticsPythonBridgeImplmentation.config_path = defaultConfig
        self.load_configuration()


    @unreal.ufunction(override=True)
    def save_configuration(self):
        # If we weren't able to successfully load a configuration, do not try to save a configuration.
        # This most likely means all internal state is invalid
        if not os.path.isfile(self.config_path):
            unreal.log_error("Configuration missing. Check your Project Acoustics plugin installation")
            return
        # Always save the configuration to the project's Config folder
        configPath = os.path.join(self.project_config.config_dir, AcousticsPythonBridgeImplmentation.project_acoustics_config_file)
        self.acoustics_configuration =  AcousticsConfiguration(configPath)

        # save azure creds
        self.acoustics_configuration.azure_creds.url = self.azure_credentials.batch_url
        self.acoustics_configuration.azure_creds.batch_key = self.azure_credentials.batch_key
        self.acoustics_configuration.azure_creds.batch_name = self.azure_credentials.batch_name
        self.acoustics_configuration.azure_creds.storage_name = self.azure_credentials.storage_name
        self.acoustics_configuration.azure_creds.storage_key = self.azure_credentials.storage_key
        self.acoustics_configuration.azure_creds.toolset_version = self.azure_credentials.toolset_version
        self.acoustics_configuration.azure_creds.acr_server = self.azure_credentials.acr_server
        self.acoustics_configuration.azure_creds.acr_account = self.azure_credentials.acr_account
        self.acoustics_configuration.azure_creds.acr_key = self.azure_credentials.acr_key

        # save compute pool configuration
        self.acoustics_configuration.compute_pool.vm_size = self.compute_pool_configuration.vm_size
        self.acoustics_configuration.compute_pool.nodes = self.compute_pool_configuration.nodes
        self.acoustics_configuration.compute_pool.use_lowpri_nodes = self.compute_pool_configuration.use_low_priority_nodes

        # Save simulation params
        self.acoustics_configuration.simulation_parameters.mesh_unit_adjustment = self.simulation_parameters.mesh_unit_adjustment
        self.acoustics_configuration.simulation_parameters.max_frequency = self.simulation_parameters.max_frequency
        self.acoustics_configuration.simulation_parameters.receiver_spacing = self.simulation_parameters.receiver_spacing
        self.acoustics_configuration.simulation_parameters.probe_horizontal_spacing_min = self.simulation_parameters.probe_spacing.horizontal_spacing_min
        self.acoustics_configuration.simulation_parameters.probe_horizontal_spacing_max = self.simulation_parameters.probe_spacing.horizontal_spacing_max
        self.acoustics_configuration.simulation_parameters.probe_vertical_spacing = self.simulation_parameters.probe_spacing.vertical_spacing
        self.acoustics_configuration.simulation_parameters.probe_min_height_above_ground = self.simulation_parameters.probe_spacing.min_height_above_ground
        self.acoustics_configuration.simulation_parameters.simulation_region_lower = self.simulation_parameters.simulation_region.min
        self.acoustics_configuration.simulation_parameters.simulation_region_upper = self.simulation_parameters.simulation_region.max

        # save operational parameters
        self.acoustics_configuration.operational_parameters.level_prefix_map = self.project_config.level_prefix_map
        self.acoustics_configuration.operational_parameters.data_folder = self.project_config.content_dir
        self.acoustics_configuration.operational_parameters.job_id = self.active_job_info.job_id
        self.acoustics_configuration.operational_parameters.job_prefix = self.active_job_info.prefix
        self.acoustics_configuration.operational_parameters.job_submit_time = self.active_job_info.submit_time

        self.acoustics_configuration.save()


    @unreal.ufunction(override=True)
    def load_configuration(self):
        self.acoustics_configuration =  AcousticsConfiguration(AcousticsPythonBridgeImplmentation.config_path)

        # Read azure account info
        self.azure_credentials.batch_url = self.acoustics_configuration.azure_creds.url
        self.azure_credentials.batch_name = self.acoustics_configuration.azure_creds.batch_name
        self.azure_credentials.storage_name = self.acoustics_configuration.azure_creds.storage_name
        self.azure_credentials.batch_key = self.acoustics_configuration.azure_creds.batch_key
        self.azure_credentials.storage_key = self.acoustics_configuration.azure_creds.storage_key
        # in order to avoid a mismatch of tools, we will always use the toolset version that is bundled with the plugin
        # this version can always be overwritten for a single bake, but will be reset to the default upon reloading the configuration
        self.azure_credentials.toolset_version = AzureInformationConfig.default_toolset_version
        self.azure_credentials.acr_server = self.acoustics_configuration.azure_creds.acr_server
        self.azure_credentials.acr_account = self.acoustics_configuration.azure_creds.acr_account
        self.azure_credentials.acr_key = self.acoustics_configuration.azure_creds.acr_key

        # Read compute pool info
        self.compute_pool_configuration.vm_size = self.acoustics_configuration.compute_pool.vm_size
        self.compute_pool_configuration.nodes = self.acoustics_configuration.compute_pool.nodes
        self.compute_pool_configuration.use_low_priority_nodes = self.acoustics_configuration.compute_pool.use_lowpri_nodes

        # read sim cofiguration
        self.simulation_parameters.mesh_unit_adjustment = self.acoustics_configuration.simulation_parameters.mesh_unit_adjustment
        self.simulation_parameters.max_frequency = self.acoustics_configuration.simulation_parameters.max_frequency
        self.simulation_parameters.receiver_spacing = self.acoustics_configuration.simulation_parameters.receiver_spacing 
        self.simulation_parameters.probe_spacing.horizontal_spacing_min = self.acoustics_configuration.simulation_parameters.probe_horizontal_spacing_min
        self.simulation_parameters.probe_spacing.horizontal_spacing_max = self.acoustics_configuration.simulation_parameters.probe_horizontal_spacing_max 
        self.simulation_parameters.probe_spacing.vertical_spacing = self.acoustics_configuration.simulation_parameters.probe_vertical_spacing
        self.simulation_parameters.probe_spacing.min_height_above_ground = self.acoustics_configuration.simulation_parameters.probe_min_height_above_ground
        self.simulation_parameters.simulation_region.min = self.acoustics_configuration.simulation_parameters.simulation_region_lower 
        self.simulation_parameters.simulation_region.max = self.acoustics_configuration.simulation_parameters.simulation_region_upper 

        # load operational parameters
        self.project_config.level_prefix_map = self.acoustics_configuration.operational_parameters.level_prefix_map
        self.project_config.content_dir = self.acoustics_configuration.operational_parameters.data_folder
        self.active_job_info.job_id = self.acoustics_configuration.operational_parameters.job_id
        self.active_job_info.prefix = self.acoustics_configuration.operational_parameters.job_prefix
        self.active_job_info.submit_time = self.acoustics_configuration.operational_parameters.job_submit_time

        self.update_azure_credentials()


    @unreal.ufunction(override=True)
    def update_azure_credentials(self):
        # Sanitize values first
        if not self.azure_credentials.toolset_version or self.azure_credentials.toolset_version == "":
            self.azure_credentials.toolset_version = AzureInformationConfig.default_toolset_version
        CloudProcessor.TritonToolsetPath = AppDomain.CurrentDomain.BaseDirectory
        CloudProcessor.BatchAccount.Url = self.azure_credentials.batch_url
        CloudProcessor.BatchAccount.Name = self.azure_credentials.batch_name
        CloudProcessor.BatchAccount.Key = self.azure_credentials.batch_key
        CloudProcessor.StorageAccount.Name = self.azure_credentials.storage_name
        CloudProcessor.StorageAccount.Key = self.azure_credentials.storage_key
        CloudProcessor.AcousticsDockerImageName = self.azure_credentials.toolset_version
        if not self.azure_credentials.acr_server or self.azure_credentials.acr_server == "":
            CloudProcessor.CustomContainerRegistry = None
        else:
            CloudProcessor.CustomContainerRegistry = ContainerRegistry(self.azure_credentials.acr_account, self.azure_credentials.acr_key, self.azure_credentials.acr_server)


    def get_latest_compute_pool_config(self):
        # Use latest compute pool config
        pool_config = ComputePoolConfiguration()
        pool_config.VirtualMachineSize = self.compute_pool_configuration.vm_size
        if (self.compute_pool_configuration.use_low_priority_nodes):
            pool_config.DedicatedNodes = 0
            pool_config.LowPriorityNodes = self.compute_pool_configuration.nodes
        else:
            pool_config.DedicatedNodes = self.compute_pool_configuration.nodes
            pool_config.LowPriorityNodes = 0
        return pool_config


    @unreal.ufunction(override=True)
    def estimate_processing_time(self):
        try:
            # Use latest simulation config
            sim_config = SimulationConfiguration()
            sim_config.Frequency = self.simulation_parameters.max_frequency
            sim_config.ProbeCount = self.job_configuration.probe_count
            sim_config.ProbeSpacing = self.simulation_parameters.probe_spacing.horizontal_spacing_max / 100
            sim_config.ReceiverSpacing = self.simulation_parameters.receiver_spacing / 100
            sim_config.SimulationVolume = \
                (self.simulation_parameters.simulation_region.max.x / 100 - self.simulation_parameters.simulation_region.min.x / 100) * \
                (self.simulation_parameters.simulation_region.max.y / 100 - self.simulation_parameters.simulation_region.min.y / 100) * \
                (self.simulation_parameters.simulation_region.max.z / 100 - self.simulation_parameters.simulation_region.min.z / 100)

            # Use latest compute pool config
            pool_config = self.get_latest_compute_pool_config()

            # Select cost sheet based on simulation frequency
            if (sim_config.Frequency == 250):
                cost_sheet = os.path.join(os.path.dirname(os.path.realpath(__file__)), '../../Resources', AcousticsPythonBridgeImplmentation.coarse_bake_cost_sheet)
            elif (sim_config.Frequency == 500):
                cost_sheet = os.path.join(os.path.dirname(os.path.realpath(__file__)), '../../Resources', AcousticsPythonBridgeImplmentation.fine_bake_cost_sheet)
            else:
                unreal.log_error('Cost sheet is not available for the configured simulation frequency')
                return 0

            duration = CloudProcessor.EstimateProcessingTime(cost_sheet, sim_config, pool_config)
            return duration.TotalMinutes
        except:
            unreal.log_error('Cost estimation failed')
            unreal.log_error(sys.exc_info()[0])
            return 0

    def reset_active_job_info(self):
        self.active_job_info.job_id = ''
        self.active_job_info.job_id_prefix = ''
        self.active_job_info.submit_time = ''
        self.active_job_info.prefix = ''
        self.active_job_info.submit_pending = False
        self.save_configuration()

    def get_active_job_ace_filename(self):
        if (not self.active_job_info):
            return None
        return self.active_job_info.prefix + ".ace"

    def get_active_job_ace_filepath(self):
        if (not self.active_job_info):
            return None
        return os.path.join(self.project_config.game_content_dir, self.get_active_job_ace_filename())

    @unreal.ufunction(override=True)
    def submit_for_processing(self):
        try:
            # Setup job configuration
            job_config = JobConfiguration()
            job_config.Prefix = "uepa" + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
            self.active_job_info.job_id_prefix = job_config.Prefix
            job_config.VoxFilepath = "{0}".format(self.job_configuration.vox_file)
            job_config.ConfigurationFilepath = "{0}".format(self.job_configuration.config_file)

            # Use latest compute pool config
            pool_config = self.get_latest_compute_pool_config()

            # Submit for processing
            self.active_job_info.submit_pending = True
            self.current_status.succeeded = True
            self.current_status.message = ''
            AcousticsPythonBridgeImplmentation.submit_monitor = CloudProcessor.SubmitForAnalysisOnThreadPool(pool_config, job_config)
        except Exception as e:
            unreal.log_error(e.Message)
            unreal.log_error(e.Source)
            self.current_status.succeeded = False
            self.current_status.message = 'Job submission failed, please see Output Log for more details and check your Azure account credentials'
            self.reset_active_job_info()
        except:
            unreal.log_error("Job submission failed")
            unreal.log_error(sys.exc_info()[0])
            self.reset_active_job_info()
            self.current_status.succeeded = False
            self.current_status.message = 'Job submission failed, please see Output Log for more details and check your Azure account credentials'


    @unreal.ufunction(override=True)
    def cancel_job(self):
        try:
            self.current_status.succeeded = True
            self.current_status.message = ''
            CloudProcessor.DeleteJobAsync(self.active_job_info.job_id).Wait()
        except Exception as error:
            unreal.log_error("Job deletion failed, job may be already deleted.")
            unreal.log_error(error)
            self.current_status.succeeded = False
            self.current_status.message = 'Job cancellation failed, please see Output Log for more details and check your Azure account credentials'
        finally:
            self.reset_active_job_info()


    def check_for_submission_status(self):
        # Are we actively monitoring submit operation?
        if (AcousticsPythonBridgeImplmentation.submit_monitor):
            # Job submission still in progress
            if (AcousticsPythonBridgeImplmentation.submit_monitor.Status == ThreadPoolOperationStatus.InProgress):
                self.active_job_info.submit_pending = True
                self.current_status.succeeded = True
                self.current_status.message = 'Job submission in progress'
            # If submit failed, clear the monitor and set failure status
            elif (AcousticsPythonBridgeImplmentation.submit_monitor.Status == ThreadPoolOperationStatus.Failed):
                self.current_status.succeeded = False
                self.current_status.message = 'Job submission failed, see Output Log for more details'
                unreal.log_error(self.submit_monitor.Result)
                AcousticsPythonBridgeImplmentation.submit_monitor = None
                self.reset_active_job_info()
            # Succeeded, save config and stop monitoring
            else:
                self.active_job_info.prefix = self.job_configuration.prefix
                self.active_job_info.job_id = self.submit_monitor.Result
                self.active_job_info.submit_time = str(datetime.datetime.now())
                self.save_configuration()
                self.active_job_info.submit_pending = False
                AcousticsPythonBridgeImplmentation.submit_monitor = None
                self.current_status.succeeded = True
                self.current_status.message = 'Job submission succeeded'
            # Return true to inform submission monitoring is active
            return True
        else:
            # Not monitoring job submission
            return False


    def check_for_download_ace_status(self):
        # Are we actively monitoring submit operation?
        if (not AcousticsPythonBridgeImplmentation.download_ace_monitor):
            return False
        else:
            # Ace download is in progress
            if (AcousticsPythonBridgeImplmentation.download_ace_monitor.Status == ThreadPoolOperationStatus.InProgress):
                self.current_status.succeeded = True
                self.current_status.message = 'Downloading results'
            # Ace download failed, clear the monitor and set failure status
            elif (AcousticsPythonBridgeImplmentation.download_ace_monitor.Status == ThreadPoolOperationStatus.Failed):
                AcousticsPythonBridgeImplmentation.download_ace_monitor = None
                self.current_status.succeeded = False
                self.current_status.message = 'Failed to download results, please ensure your Azure credentials are valid'
                unreal.log_error(self.download_ace_monitor.Result)
            # Succeeded, save config and stop monitoring
            else:
                AcousticsPythonBridgeImplmentation.download_ace_monitor = None
                self.current_status.succeeded = True
                self.current_status.message = "Downloaded results - " + self.get_active_job_ace_filename()

                # Cleanup the job and storage. Download the log files before storage is deleted
                log_path = os.path.join(self.project_config.log_dir,  self.active_job_info.job_id_prefix)
                CloudProcessor.DeleteJob(self.active_job_info.job_id, True, log_path)

                # Done downloading, save the ace filepath and reset active job info
                ace_filepath = self.get_active_job_ace_filepath()
                self.reset_active_job_info()

                # Import the ace as an asset
                self.create_ace_asset(ace_filepath)
            # Return true to inform download monitoring is active
            return True


    @unreal.ufunction(override=True)
    def update_job_status(self):
        if not AcousticsPythonBridgeImplmentation.lock.acquire(False):
            return
        else:
            try:
                # Early return if job submission monitoring is active
                if (self.check_for_submission_status() == True):
                    return

                job_id = self.active_job_info.job_id
                job_info = CloudProcessor.GetJobInformation(job_id)
                self.current_status.succeeded = True
                self.current_status.message = ''

                # Job is being processed
                if (job_info.Status == JobStatus.InProgress):
                    total = job_info.Tasks.Active + job_info.Tasks.Completed + job_info.Tasks.Running;
                    self.current_status.message = "Running: {0}/{1} tasks complete.".format(job_info.Tasks.Completed, total)
                    # Inform about failed tasks
                    if (job_info.Tasks.Failed > 0):
                        self.current_status.message = self.current_status.message + "{0} tasks failed.".format(job_info.Tasks.Failed)

                # Job hasn't started yet...
                elif (job_info.Status == JobStatus.Pending):
                    self.current_status.message = 'Initializing compute pool nodes...'

                # Job has finished
                else:
                    self.current_status.message = "Waiting for job completion..."
                    if (job_info.Status == JobStatus.Completed):
                        self.current_status.message = "Completed"
                        try:
                            # Download results for the active job
                            if (not self.check_for_download_ace_status()): 
                                # Download the ace to the project content folder
                                # Make sure the destination directory exists first
                                if not os.path.exists(self.project_config.game_content_dir):
                                    os.makedirs(self.project_config.game_content_dir)

                                # Ensure current ace file is not write protected
                                # This will block until the file gets write access
                                self.show_readonly_ace_dialog();

                                final_ace_path = self.get_active_job_ace_filepath()
                                AcousticsPythonBridgeImplmentation.download_ace_monitor = CloudProcessor.DownloadAceFileOnThreadPool(job_id, final_ace_path)
                                self.current_status.message = "Downloading results"

                        except Exception as error:
                            self.current_status.succeeded = False
                            self.current_status.message = 'Failed to download the results, please see Output Log for more details and check your Azure account credentials'
                            unreal.log_error(error)
                return
            except Exception as error:
                unreal.log_error(error)
                self.current_status.succeeded = False
                self.current_status.message = 'Failed to query job status, please see Output Log for more details and check your Azure account credentials'
                return
            except:
                unreal.log_error(sys.exc_info()[0])
                self.current_status.succeeded = False
                self.current_status.message = 'Failed to query job status, please see Output Log for more details and check your Azure account credentials'
                return
            finally:
                AcousticsPythonBridgeImplmentation.lock.release()
