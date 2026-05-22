在本地 PC 上烘焙声学是许多用户在场景规模较小、以熟悉技术为目标的场景下使用的选项。随着场景规模增大，使用 Azure Batch 等服务可以减少烘焙完成所需的时间。

Baking acoustics on a local PC is an option that many customers utilize when scene sizes are small and getting familiar with the technology is the goal. As scene sizes increase, using a service like Azure Batch will reduce the time it takes to complete a bake.

通过 **Prepare Local Bake** 按钮创建本地烘焙目录后，点击 **Download Local Bake Tools** 按钮下载烘焙工具包，并将其放入所选的本地烘焙目录中。

After creating a local bake directory via the Prepare Local Bake button, select the Download Local Bake Tools button to download the bake tools package and place the tools in the chosen local bake directory.

工具和配置文件都就位于本地烘焙目录后，执行 `RunLocalBake.bat` 脚本即可启动烘焙，该烘焙将在本地 PC 上串行运行，生成一个以烘焙开始时间戳命名的新工作目录。要将 `.ace` 文件导入项目，请使用 Content Browser 导航到 `Content\Acoustics` 文件夹，将 `.ace` 文件拖放至该文件夹 —— 或者 —— 点击 Import 按钮并找到要导入的 `.ace` 文件。导入完成后，即可将 AcousticsSpace Actor 的 **Acoustics Data** 属性设置为新的 `.ace` 文件。有关将 ACE 文件导入项目的更多文档，请参阅 https://aka.ms/acoustics。

Once the tools and the configuration files are co-located in the local bake directory, execute the "RunLocalBake.bat" script to start a bake that will run serially on your local PC producing a new working directory matching the starting timestamp of the bake. To import the .ace file into your project, use the Content Browser to navigate to the Content\Acoustics folder and either drag and drop your .ace file into the folder -or- select the Import button and find the .ace file you would like to import into the project. Once the import is complete, you can then set the Acoustics Data property of the AcousticsSpace actor to new .ace file. Consult the documentation on https://aka.ms/acoustics for more documentation about importing ACE files into a project.

---

## 命令行参数 / Command-Line Arguments

用法（Usage）：

```
Triton.LocalProcessor.exe --<opt0> <val0> <val1>... --<opt1> <val0> <val1>... ...
```

| 参数 / Argument | 说明 / Description |
|---|---|
| **ConfigFile** *(必填)* | Triton 任务 XML 配置文件名称 / Name of the XML triton job config file |
| **NodeSize** *(可选)* | 运行模拟的 PC/VM 规格，默认为 `Local` / Size of the PC/VM, default `Local` |
| **NumThreads** *(可选)* | 线程数，默认为最大逻辑处理器数 / Number of threads, defaults to max logical processors |
| **OutputName** *(可选)* | 输出 `.ace` 文件名称 / Name of the output ACE file |
| **Resume** *(可选)* | 布尔值，默认 `false`。若设为 `true` 且工作目录中存在 `.enc` 文件，则从最后一个 `.enc` 索引恢复烘焙 / If true with existing .enc files, resumes bake from last .enc index |
| **SharedInputDataDir** *(可选)* | Vox 和配置文件所在路径，相对于工作目录。未指定则假定位于 `WorkingDir` / Path to vox and config files relative to WorkingDir |
| **WorkingDir** *(可选)* | 进程工作目录，任务输出也写入此路径 / Process working directory; task output also written here |