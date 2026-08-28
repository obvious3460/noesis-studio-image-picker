# Troubleshooting / 故障排查

## 中文

### 点击文字没有打开弹窗

- 确认运行的是重新构建后的 `App.StudioTool.exe`。
- `Image.Source` 使用单击 `Source` 文字。
- `ImageBrush.ImageSource` 使用单击 `ImageSource` 文字。
- 确认当前打开的是带有 `.noesis` 文件的项目，而不是单独打开 XAML。

### 弹窗打开后没有图片

- 图片必须位于 `.noesis` 项目根目录之下。
- 当前版本支持常见的 PNG、JPG/JPEG、BMP、GIF、TGA、DDS 与 WEBP
  图片扩展名；实际预览能力还取决于 Noesis 的资源提供器。
- 尝试选择更上层的文件夹，因为文件列表会递归包含其所有子目录。

### 恢复安装前状态

安装脚本会输出备份目录。关闭 Studio 后，将该备份目录中的文件按原相对
路径复制回 SDK 即可。不要混用不同 SDK 版本生成的备份。

## English

### Clicking the label does not open the popup

- Verify that you are running the rebuilt `App.StudioTool.exe`.
- Click the `Source` label for `Image.Source`.
- Click the `ImageSource` label for an `ImageBrush`.
- Open a project that has a `.noesis` file instead of opening an isolated XAML.

### The picker opens but contains no images

- Images must be located below the `.noesis` project root.
- The picker recognizes common PNG, JPG/JPEG, BMP, GIF, TGA, DDS, and WEBP
  extensions. Preview support also depends on the active Noesis resource
  provider.
- Select a parent directory; the list recursively includes all descendants.

### Restore the pre-install state

The installer prints the backup directory. Close Studio and copy those files
back to the SDK using their original relative paths. Do not restore a backup
created from a different SDK version.
