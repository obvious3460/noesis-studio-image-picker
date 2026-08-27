# Noesis Studio Image Picker / Noesis Studio 图片选择器

[中文](#中文) · [English](#english)

> An independent extension for the `App/StudioTool` sample included with the
> NoesisGUI SDK. No Noesis SDK files or binaries are distributed here.

## 中文

这是一个面向 Noesis StudioTool 的图片资源选择器扩展，用来改善大型 XAML
项目中 `Image.Source` 与 `ImageBrush.ImageSource` 的资源查找体验。

### 功能

- 以竖向层级浏览项目文件夹，并按字母排序。
- 递归列出选中目录及全部子目录中的图片。
- 按文件名首字母进行快速筛选。
- 在文件按钮左侧显示缩略预览图。
- 选择图片后自动回填属性并关闭弹窗。
- 记住最近浏览目录、字母筛选和最后选择的图片。
- 支持 `Image.Source`，以及 `Background`、`Foreground` 等属性中的
  `ImageBrush.ImageSource`。
- 保留 Studio 默认下拉选择：单击 `Source` 文字打开扩展界面；双击
  `ImageSource` 文字打开扩展界面。

### 要求

- 合法取得、并包含 `Src/Packages/App/StudioTool` 的 NoesisGUI 4.0 SDK。
- Windows PowerShell 5.1 或 PowerShell 7。
- Visual Studio 2019、2022 或 2026，对应 SDK 自带的 Windows 工程。

### 安装

关闭正在运行的 Noesis Studio，然后在 PowerShell 中执行：

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\install.ps1 -SdkRoot "C:\path\to\NoesisSDK"
```

安装脚本会：

1. 将扩展文件复制到 `Src/Packages/App/StudioTool`。
2. 对 StudioTool 的两个入口源文件进行最小化接入。
3. 更新现有 Visual Studio 工程，使新增 C++/XAML 文件参与构建。
4. 在 SDK 根目录的 `.image-picker-backup` 中保存原文件备份。

打开 SDK 自带的 Windows 解决方案，构建 `App.StudioTool`。例如：

```text
Build/NoesisGUI-win-x86_64-vs2022.sln
```

### 使用

- `Image.Source`：单击属性左侧的 `Source` 文字。
- `ImageBrush.ImageSource`：双击 `ImageSource` 文字。
- 单击图片条目即可写入属性并自动关闭弹窗。

## English

This extension adds a project-aware image browser to Noesis StudioTool. It is
designed for large XAML projects where the default `ImageSource` drop-down is
not enough.

### Features

- Vertical, alphabetical folder navigation.
- Recursive image listing for the selected folder and all descendants.
- First-letter filename filtering.
- Thumbnail previews next to file names.
- Applies the selected URI and closes the popup automatically.
- Restores the last folder, letter filter, and selected image.
- Supports `Image.Source` and `ImageBrush.ImageSource` inside properties such
  as `Background` and `Foreground`.
- Preserves the native Studio selector: click the `Source` label or
  double-click the `ImageSource` label to open the custom picker.

### Requirements

- A legally obtained NoesisGUI 4.0 SDK containing
  `Src/Packages/App/StudioTool`.
- Windows PowerShell 5.1 or PowerShell 7.
- Visual Studio 2019, 2022, or 2026 with the matching SDK project files.

### Install

Close Noesis Studio and run:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\install.ps1 -SdkRoot "C:\path\to\NoesisSDK"
```

The installer copies the extension, applies the minimum StudioTool integration,
updates the existing Visual Studio projects, and stores backups under
`.image-picker-backup` in the SDK root.

Open the matching Windows solution supplied with the SDK and build
`App.StudioTool`, for example:

```text
Build/NoesisGUI-win-x86_64-vs2022.sln
```

### Usage

- `Image.Source`: click the `Source` property label.
- `ImageBrush.ImageSource`: double-click the `ImageSource` property label.
- Click an image entry to apply it and close the popup.

## Screenshots / 截图

Screenshots will be added after the public test pass.

截图将在公开测试完成后补充。

## Compatibility / 兼容性

The initial release was developed and tested against NoesisGUI 4.0 on Windows
x86_64. StudioTool internals may change between SDK releases; keep the backup
created by the installer.

首个版本基于 Windows x86_64 平台的 NoesisGUI 4.0 开发和测试。不同 SDK
版本的 StudioTool 内部结构可能变化，请保留安装脚本创建的备份。

## License and trademark notice / 许可与商标声明

The extension code in this repository is licensed under the MIT License. The
NoesisGUI SDK and Noesis Studio are not included and remain subject to their
owners' terms. See [NOTICE.md](NOTICE.md).

本仓库中的扩展代码采用 MIT 许可证；仓库不包含 NoesisGUI SDK 与 Noesis
Studio，它们仍受权利人的许可条款约束。详见 [NOTICE.md](NOTICE.md)。
