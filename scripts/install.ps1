[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$SdkRoot
)

$ErrorActionPreference = "Stop"

$sdk = [IO.Path]::GetFullPath($SdkRoot)
$studio = Join-Path $sdk "Src\Packages\App\StudioTool"
$srcDir = Join-Path $studio "Src"
$dataDir = Join-Path $studio "Data"
$main = Join-Path $srcDir "Main.cpp"
$window = Join-Path $srcDir "MainWindow.xaml.mm.cpp"
$repo = Split-Path -Parent $PSScriptRoot
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = Join-Path $sdk ".image-picker-backup\$stamp"
$utf8 = [Text.UTF8Encoding]::new($false)

if (!(Test-Path -LiteralPath $main) || !(Test-Path -LiteralPath $window))
{
    throw "StudioTool was not found under: $sdk"
}

function Backup-File
{
    param([string]$Path)

    if (!(Test-Path -LiteralPath $Path))
    {
        return
    }

    $rel = [IO.Path]::GetRelativePath($sdk, $Path)
    $dest = Join-Path $backup $rel
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $dest) | Out-Null
    Copy-Item -LiteralPath $Path -Destination $dest -Force
}

function Save-Text
{
    param([string]$Path, [string]$Text)

    $old = [IO.File]::ReadAllText($Path)
    if ($old -eq $Text)
    {
        return
    }

    Backup-File $Path
    [IO.File]::WriteAllText($Path, $Text, $utf8)
}

function Add-After
{
    param([string]$Text, [string]$Needle, [string]$Addition, [string]$Label)

    if ($Text.Contains($Addition.Trim()))
    {
        return $Text
    }

    $pos = $Text.IndexOf($Needle, [StringComparison]::Ordinal)
    if ($pos -lt 0)
    {
        throw "Unable to locate $Label"
    }

    $end = $Text.IndexOf("`n", $pos)
    if ($end -lt 0)
    {
        $end = $Text.Length - 1
    }

    return $Text.Insert($end + 1, $Addition)
}

function Add-Before
{
    param([string]$Text, [string]$Needle, [string]$Addition, [string]$Label)

    if ($Text.Contains($Addition.Trim()))
    {
        return $Text
    }

    $pos = $Text.IndexOf($Needle, [StringComparison]::Ordinal)
    if ($pos -lt 0)
    {
        throw "Unable to locate $Label"
    }

    return $Text.Insert($pos, $Addition)
}

function Copy-Addon
{
    param([string]$Source, [string]$Destination)

    Backup-File $Destination
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

$nl = "`r`n"
$mainText = [IO.File]::ReadAllText($main)
if (!$mainText.Contains("`r`n"))
{
    $nl = "`n"
}

$mainText = Add-After $mainText '#include "MainWindow.xaml.h"' `
    ('#include "ImagePicker.h"' + $nl) "MainWindow include"
$mainText = Add-After $mainText '#include "MainWindow.xaml.bin.h"' `
    ('#include "ImagePicker.xaml.bin.h"' + $nl + '#include "ImagePicker.Resources.xaml.bin.h"' + $nl) `
    "embedded MainWindow XAML include"
$mainText = Add-After $mainText 'RegisterComponent<StudioTool::MainWindow>();' `
    ('        RegisterComponent<ImagePicker>();' + $nl) "component registration"
$mainText = Add-After $mainText '{ "MainWindow.xaml", MainWindow_xaml },' `
    ('            { "ImagePicker.xaml", ImagePicker_xaml },' + $nl +
     '            { "ImagePicker.Resources.xaml", ImagePicker_Resources_xaml },' + $nl) `
    "embedded XAML table"
Save-Text $main $mainText

$winText = [IO.File]::ReadAllText($window)
$nl = "`r`n"
if (!$winText.Contains("`r`n"))
{
    $nl = "`n"
}

$winText = Add-After $winText '#include "MainWindow.xaml.h"' `
    ('#include "ImagePicker.h"' + $nl) "MainWindow implementation include"
$winText = Add-After $winText '    FileName(projectPath, projectAsm);' `
    ('    ImagePicker::SetRoot(projectRoot.Str(), projectAsm.Str());' + $nl) "project root setup"
$winText = Add-After $winText '    Noesis::Ptr<FrameworkElement> studio = Studio::Create(projectPath, options);' `
    ('    ImagePicker::Install(studio);' + $nl) "Studio picker installation"
Save-Text $window $winText

Copy-Addon (Join-Path $repo "addon\Src\ImagePicker.h") (Join-Path $srcDir "ImagePicker.h")
Copy-Addon (Join-Path $repo "addon\Src\ImagePicker.xaml.cpp") (Join-Path $srcDir "ImagePicker.xaml.cpp")
Copy-Addon (Join-Path $repo "addon\Data\ImagePicker.xaml") (Join-Path $dataDir "ImagePicker.xaml")
Copy-Addon (Join-Path $repo "addon\Data\ImagePicker.Resources.xaml") `
    (Join-Path $dataDir "ImagePicker.Resources.xaml")

$projectRoot = Join-Path $sdk "Src\Projects\StudioTool"
$projects = Get-ChildItem -LiteralPath $projectRoot -Recurse -Filter "App.StudioTool-vs*.vcxproj"
foreach ($project in $projects)
{
    $text = [IO.File]::ReadAllText($project.FullName)
    $nl = "`r`n"
    if (!$text.Contains("`r`n"))
    {
        $nl = "`n"
    }

    $srcItems = '    <ClInclude Include="..\..\..\Packages\App\StudioTool\Src\ImagePicker.h" />' + $nl +
        '    <ClCompile Include="..\..\..\Packages\App\StudioTool\Src\ImagePicker.xaml.cpp" />' + $nl
    $text = Add-Before $text `
        '    <ClInclude Include="..\..\..\Packages\App\StudioTool\Src\MainWindow.xaml.h"' `
        $srcItems "Visual Studio source item group"

    if (!$text.Contains('Data\ImagePicker.xaml'))
    {
        $pattern = '(?s)    <CustomBuild Include="[^\r\n]*Data\\MainWindow\.xaml">.*?    </CustomBuild>\r?\n'
        $match = [regex]::Match($text, $pattern)
        if (!$match.Success)
        {
            throw "Unable to locate MainWindow XAML build rule in $($project.FullName)"
        }

        $picker = $match.Value.Replace("MainWindow.xaml", "ImagePicker.xaml")
        $resources = $match.Value.Replace("MainWindow.xaml", "ImagePicker.Resources.xaml")
        $text = $text.Insert($match.Index + $match.Length, $picker + $resources)
    }

    Save-Text $project.FullName $text

    $filterPath = $project.FullName + ".filters"
    if (Test-Path -LiteralPath $filterPath)
    {
        $filter = [IO.File]::ReadAllText($filterPath)
        $nl = "`r`n"
        if (!$filter.Contains("`r`n"))
        {
            $nl = "`n"
        }

        $filterItems = '    <ClInclude Include="..\..\..\Packages\App\StudioTool\Src\ImagePicker.h">' + $nl +
            '      <Filter>Src</Filter>' + $nl + '    </ClInclude>' + $nl +
            '    <ClCompile Include="..\..\..\Packages\App\StudioTool\Src\ImagePicker.xaml.cpp">' + $nl +
            '      <Filter>Src</Filter>' + $nl + '    </ClCompile>' + $nl
        $filter = Add-Before $filter `
            '    <ClInclude Include="..\..\..\Packages\App\StudioTool\Src\MainWindow.xaml.h"' `
            $filterItems "Visual Studio filter source items"

        if (!$filter.Contains('Data\ImagePicker.xaml'))
        {
            $pattern = '(?s)    <CustomBuild Include="[^\r\n]*Data\\MainWindow\.xaml">.*?    </CustomBuild>\r?\n'
            $match = [regex]::Match($filter, $pattern)
            if (!$match.Success)
            {
                throw "Unable to locate MainWindow XAML filter in $filterPath"
            }

            $picker = $match.Value.Replace("MainWindow.xaml", "ImagePicker.xaml")
            $resources = $match.Value.Replace("MainWindow.xaml", "ImagePicker.Resources.xaml")
            $filter = $filter.Insert($match.Index + $match.Length, $picker + $resources)
        }

        Save-Text $filterPath $filter
    }
}

Write-Host "Image picker installed successfully." -ForegroundColor Green
Write-Host "Backup: $backup"
Write-Host "Build App.StudioTool from the NoesisGUI Windows solution for your Visual Studio version."
