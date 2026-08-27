////////////////////////////////////////////////////////////////////////////////////////////////////
// Noesis Studio 图片资源选择器
////////////////////////////////////////////////////////////////////////////////////////////////////


#include "ImagePicker.h"

#include <NsCore/Ptr.h>
#include <NsCore/Boxing.h>
#include <NsCore/ReflectionHelper.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/TypeOf.h>
#include <NsCore/TypeClass.h>
#include <NsCore/TypeProperty.h>
#include <NsGui/BitmapImage.h>
#include <NsGui/Border.h>
#include <NsGui/Button.h>
#include <NsGui/Control.h>
#include <NsGui/FrameworkElement.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/Image.h>
#include <NsGui/Enums.h>
#include <NsGui/ICommand.h>
#include <NsGui/Panel.h>
#include <NsGui/Popup.h>
#include <NsGui/StackPanel.h>
#include <NsGui/TextBlock.h>
#include <NsDrawing/Thickness.h>
#include <NsGui/UIElementCollection.h>
#include <NsGui/UIElementData.h>
#include <NsGui/UICollection.h>
#include <NsGui/Uri.h>
#include <NsGui/UIElementEvents.h>
#include <NsGui/Visual.h>
#include <NsGui/VisualTreeHelper.h>
#include <NsGui/WrapPanel.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>


Noesis::String ImagePicker::sRoot;
Noesis::String ImagePicker::sAsm;
Noesis::EventHandler ImagePicker::sBase;
bool ImagePicker::sHave = false;
std::filesystem::path ImagePicker::sFolder;
std::filesystem::path ImagePicker::sFile;
char ImagePicker::sLetter = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：创建图片选择器并绑定界面控件；参数：无；返回值：无。
ImagePicker::ImagePicker(): mPopup(nullptr), mCols(nullptr), mTabPanel(nullptr), mFilePanel(nullptr),
    mPath(nullptr), mCount(nullptr), mLetter(0)
{
    Noesis::GUI::LoadComponent(this, "ImagePicker.xaml");

    mPopup = FindName<Noesis::Popup>("PickerPopup");
    mCols = FindName<Noesis::StackPanel>("FolderColumns");
    mTabPanel = FindName<Noesis::WrapPanel>("LetterPanel");
    mFilePanel = FindName<Noesis::WrapPanel>("FilePanel");
    mPath = FindName<Noesis::TextBlock>("FolderPath");
    mCount = FindName<Noesis::TextBlock>("FileCount");

    Noesis::Button* close = FindName<Noesis::Button>("CloseButton");
    if (close != nullptr)
    {
        close->Click() += Noesis::MakeDelegate(this, &ImagePicker::OnClose);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：取得当前图片资源路径；参数：无；返回值：资源相对路径。
const char* ImagePicker::GetValue() const
{
    const Noesis::String& value = Noesis::DependencyObject::GetValue<Noesis::String>(ValueProperty);
    return value.Str();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：设置当前图片资源路径；参数：value 为资源相对路径；返回值：无。
void ImagePicker::SetValue(const char* value)
{
    Noesis::String path;
    if (value != nullptr)
    {
        path = value;
    }
    Noesis::DependencyObject::SetValue<Noesis::String>(ValueProperty, path.Str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：设置当前 Noesis 项目的资源根目录和程序集；参数：root 为项目目录，assembly 为程序集名；返回值：无。
void ImagePicker::SetRoot(const char* root, const char* assembly)
{
    sRoot.Clear();
    sAsm.Clear();
    if (assembly != nullptr)
    {
        sAsm = assembly;
    }
    if (root == nullptr)
    {
        return;
    }

    sRoot = root;
    for (char* it = sRoot.Begin(); it != sRoot.End(); ++it)
    {
        if (*it == '\\')
        {
            *it = '/';
        }
    }
    LoadState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：安装图片属性文字点击处理和共享选择弹窗；参数：studio 为 Studio 根控件；返回值：无。
void ImagePicker::Install(Noesis::FrameworkElement* studio)
{
    if (studio == nullptr)
    {
        return;
    }

    const Noesis::TypeClass* cls = Noesis::TypeOf<Noesis::Control>();
    Noesis::UIElementData* meta = Noesis::FindMeta<Noesis::UIElementData>(cls);
    if (meta == nullptr)
    {
        return;
    }

    Noesis::Ptr<ImagePicker> brush = Noesis::MakePtr<ImagePicker>();
    studio->Loaded() += [brush](Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs&)
    {
        Noesis::FrameworkElement* root = Noesis::DynamicCast<Noesis::FrameworkElement*>(sender);
        if (root == nullptr || Noesis::VisualTreeHelper::GetParent(brush) != nullptr)
        {
            return;
        }

        Noesis::Panel* host = nullptr;
        std::vector<Noesis::Visual*> nodes;
        nodes.push_back(root);
        for (size_t idx = 0; idx < nodes.size() && host == nullptr; ++idx)
        {
            Noesis::Visual* node = nodes[idx];
            host = Noesis::DynamicCast<Noesis::Panel*>(node);
            uint32_t count = Noesis::VisualTreeHelper::GetChildrenCount(node);
            for (uint32_t child = 0; child < count; ++child)
            {
                nodes.push_back(Noesis::VisualTreeHelper::GetChild(node, child));
            }
        }
        if (host != nullptr)
        {
            brush->SetWidth(0.0f);
            brush->SetHeight(0.0f);
            host->GetChildren()->Add(brush);
        }
    };

    const Noesis::RoutedEvent* down = Noesis::UIElement::PreviewMouseDownEvent;
    const Noesis::EventHandlerInfo* info = Noesis::FindRoutedEventHandler(cls, down);
    if (info != nullptr)
    {
        sBase = info->handler;
        sHave = true;
    }

    meta->RegisterEventHandler(down,
        [brush](Noesis::BaseComponent* sender, const Noesis::EventArgs& event)
    {
        const Noesis::MouseButtonEventArgs& args =
            static_cast<const Noesis::MouseButtonEventArgs&>(event);

        if (args.changedButton != Noesis::MouseButton_Left)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }

        Noesis::TextBlock* label = Noesis::DynamicCast<Noesis::TextBlock*>(args.source);
        if (label == nullptr)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }

        bool source = strcmp(label->GetText(), "Source") == 0;
        bool imageSource = strcmp(label->GetText(), "ImageSource") == 0;
        if (!source && !imageSource)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }
        if (imageSource && args.clickCount != 2)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }

        Noesis::BaseComponent* model = label->GetDataContext();
        if (model == nullptr)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }
        Noesis::Symbol packed("PackedUri");
        Noesis::TypeClassProperty image = Noesis::FindProperty(model->GetClassType(), packed);
        if (image.property == nullptr)
        {
            if (sHave)
            {
                sBase(sender, event);
            }
            return;
        }

        Noesis::Symbol valueName("Value");
        Noesis::TypeClassProperty value = Noesis::FindProperty(model->GetClassType(), valueName);
        if (value.property != nullptr)
        {
            Noesis::Ptr<Noesis::BaseComponent> current = value.property->GetComponent(model);
            if (Noesis::Boxing::CanUnbox<Noesis::String>(current))
            {
                const Noesis::String& path = Noesis::Boxing::Unbox<Noesis::String>(current);
                brush->SetValue(path.Str());
            }
        }

        brush->SetDataContext(model);
        if (brush->mPopup != nullptr)
        {
            brush->mPopup->SetPlacementTarget(label);
        }
        brush->OnOpen(sender, args);
        args.handled = true;
    }, true);
}

/// 功能：打开选择弹窗并定位当前资源目录；参数：sender 为事件源，args 为事件参数；返回值：无。
void ImagePicker::OnOpen(Noesis::BaseComponent*, const Noesis::RoutedEventArgs&)
{
    mRoot = std::filesystem::u8path(sRoot.Str());
    std::error_code ec;
    mRoot = std::filesystem::weakly_canonical(mRoot, ec);
    if (ec || !std::filesystem::is_directory(mRoot, ec))
    {
        return;
    }

    std::string raw = GetValue();
    std::replace(raw.begin(), raw.end(), '\\', '/');
    size_t pos = raw.find(";component/");
    if (pos != std::string::npos)
    {
        raw = raw.substr(pos + 11);
    }
    while (!raw.empty() && raw.front() == '/')
    {
        raw.erase(raw.begin());
    }

    std::filesystem::path dir = mRoot;
    bool restore = false;
    if (!sFolder.empty() && std::filesystem::is_directory(sFolder, ec))
    {
        dir = sFolder;
        restore = true;
    }
    else if (!raw.empty())
    {
        std::filesystem::path full = mRoot / std::filesystem::u8path(raw);
        if (std::filesystem::is_regular_file(full, ec))
        {
            dir = full.parent_path();
            sFile = full;
        }
        else if (std::filesystem::is_directory(full, ec))
        {
            dir = full;
        }
    }

    SetFolder(dir, restore);
    if (mPopup != nullptr)
    {
        mPopup->SetIsOpen(true);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：关闭图片选择弹窗；参数：sender 为事件源，args 为事件参数；返回值：无。
void ImagePicker::OnClose(Noesis::BaseComponent*, const Noesis::RoutedEventArgs&)
{
    SaveState();
    if (mPopup != nullptr)
    {
        mPopup->SetIsOpen(false);
    }
    mFolders.clear();
    mFiles.clear();
    mTabs.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：切换当前文件夹；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
void ImagePicker::OnFolder(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs&)
{
    for (const PickRef& ref : mFolders)
    {
        if (ref.btn == sender)
        {
            SetFolder(ref.path);
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：切换文件首字母筛选；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
void ImagePicker::OnLetter(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs&)
{
    for (const TabRef& ref : mTabs)
    {
        if (ref.btn == sender)
        {
            mLetter = ref.letter;
            sLetter = mLetter;
            BuildLetters();
            BuildFiles();
            SaveState();
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：选择文件、回填路径并关闭弹窗；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
void ImagePicker::OnFile(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs&)
{
    for (const PickRef& ref : mFiles)
    {
        if (ref.btn == sender)
        {
            sFile = ref.path;
            sFolder = ref.path.parent_path();
            sLetter = mLetter;
            SaveState();
            Noesis::String path = RelPath(ref.path);
            Noesis::Uri uri = Noesis::Uri::Pack(sAsm.Str(), path.Str());
            Noesis::String value(uri.Str());
            Noesis::BaseComponent* model = GetDataContext();
            if (model != nullptr)
            {
                Noesis::Symbol name("SetValueCommand");
                Noesis::TypeClassProperty info = Noesis::FindProperty(model->GetClassType(), name);
                if (info.property != nullptr)
                {
                    Noesis::Ptr<Noesis::BaseComponent> obj = info.property->GetComponent(model);
                    Noesis::ICommand* cmd = Noesis::DynamicCast<Noesis::ICommand*>(obj.GetPtr());
                    Noesis::Ptr<Noesis::BaseComponent> param = Noesis::Boxing::Box(value);
                    if (cmd != nullptr && cmd->CanExecute(param))
                    {
                        cmd->Execute(param);
                    }
                }
            }
            SetValue(value.Str());
            if (mPopup != nullptr)
            {
                mPopup->SetIsOpen(false);
            }
            return;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：设置当前目录并刷新全部列表；参数：dir 为目标目录；返回值：无。
void ImagePicker::SetFolder(const std::filesystem::path& dir, bool restore)
{
    std::error_code ec;
    std::filesystem::path next = std::filesystem::weakly_canonical(dir, ec);
    if (ec || !std::filesystem::is_directory(next, ec))
    {
        next = mRoot;
    }

    std::filesystem::path rel = std::filesystem::relative(next, mRoot, ec);
    if (ec || (!rel.empty() && *rel.begin() == ".."))
    {
        next = mRoot;
    }

    mFolder = next;
    if (restore)
    {
        mLetter = sLetter;
    }
    else
    {
        mLetter = 0;
    }
    BuildFolders();
    ScanFiles();
    BuildLetters();
    BuildFiles();
    sFolder = mFolder;
    sLetter = mLetter;
    SaveState();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：取得图片选择器状态文件路径；参数：无；返回值：状态文件路径。
std::filesystem::path ImagePicker::StatePath()
{
    std::string base;
  #ifdef NS_PLATFORM_WINDOWS
    char* raw = nullptr;
    size_t size = 0;
    if (_dupenv_s(&raw, &size, "LOCALAPPDATA") == 0 && raw != nullptr)
    {
        base = raw;
        std::free(raw);
    }
  #endif
  #ifdef NS_PLATFORM_APPLE
    const char* raw = std::getenv("HOME");
    if (raw != nullptr)
    {
        base = raw;
    }
  #endif
    if (base.empty())
    {
        return std::filesystem::path();
    }

    std::filesystem::path dir = std::filesystem::u8path(base);
  #ifdef NS_PLATFORM_WINDOWS
    dir /= "Noesis";
    dir /= "StudioTool";
  #endif
  #ifdef NS_PLATFORM_APPLE
    dir /= "Library";
    dir /= "Application Support";
    dir /= "Noesis";
    dir /= "StudioTool";
  #endif
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec)
    {
        return std::filesystem::path();
    }
    return dir / "ImagePickerState";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：读取上次浏览的目录、筛选和文件；参数：无；返回值：无。
void ImagePicker::LoadState()
{
    sFolder.clear();
    sFile.clear();
    sLetter = 0;

    std::filesystem::path state = StatePath();
    std::ifstream input(state, std::ios::binary);
    if (state.empty() || !input)
    {
        return;
    }

    std::string rootLine;
    std::string folderLine;
    std::string letterLine;
    std::string fileLine;
    if (!std::getline(input, rootLine) || !std::getline(input, folderLine) ||
        !std::getline(input, letterLine) || !std::getline(input, fileLine))
    {
        return;
    }

    std::error_code ec;
    std::filesystem::path root = std::filesystem::weakly_canonical(
        std::filesystem::u8path(sRoot.Str()), ec);
    if (ec)
    {
        return;
    }
    std::filesystem::path saved = std::filesystem::weakly_canonical(
        std::filesystem::u8path(rootLine), ec);
    if (ec || saved != root)
    {
        return;
    }

    std::filesystem::path folder = std::filesystem::weakly_canonical(
        root / std::filesystem::u8path(folderLine), ec);
    std::filesystem::path rel = std::filesystem::relative(folder, root, ec);
    if (!ec && std::filesystem::is_directory(folder, ec) &&
        (rel.empty() || *rel.begin() != ".."))
    {
        sFolder = folder;
    }

    if (letterLine == "0")
    {
        sLetter = 0;
    }
    else if (letterLine.size() == 1 &&
        ((letterLine[0] >= 'A' && letterLine[0] <= 'Z') || letterLine[0] == '#'))
    {
        sLetter = letterLine[0];
    }

    std::filesystem::path file = std::filesystem::weakly_canonical(
        root / std::filesystem::u8path(fileLine), ec);
    rel = std::filesystem::relative(file, root, ec);
    if (!ec && std::filesystem::is_regular_file(file, ec) &&
        (rel.empty() || *rel.begin() != ".."))
    {
        sFile = file;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：保存当前浏览的目录、筛选和文件；参数：无；返回值：无。
void ImagePicker::SaveState() const
{
    if (mRoot.empty() || mFolder.empty())
    {
        return;
    }

    std::filesystem::path state = StatePath();
    std::ofstream output(state, std::ios::binary | std::ios::trunc);
    if (state.empty() || !output)
    {
        return;
    }

    std::error_code ec;
    std::filesystem::path folder = std::filesystem::relative(mFolder, mRoot, ec);
    if (ec)
    {
        return;
    }
    std::filesystem::path file;
    if (!sFile.empty())
    {
        file = std::filesystem::relative(sFile, mRoot, ec);
        if (ec)
        {
            file.clear();
        }
    }

    output << mRoot.generic_u8string() << '\n';
    output << folder.generic_u8string() << '\n';
    if (mLetter == 0)
    {
        output << "0\n";
    }
    else
    {
        output << mLetter << '\n';
    }
    output << file.generic_u8string() << '\n';
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：按当前目录层级创建文件夹列；参数：无；返回值：无。
void ImagePicker::BuildFolders()
{
    if (mCols == nullptr)
    {
        return;
    }

    mCols->GetChildren()->Clear();
    mFolders.clear();

    std::vector<std::filesystem::path> dirs;
    dirs.push_back(mRoot);
    std::error_code ec;
    std::filesystem::path rel = std::filesystem::relative(mFolder, mRoot, ec);
    if (!ec)
    {
        std::filesystem::path cur = mRoot;
        for (const std::filesystem::path& part : rel)
        {
            if (part == ".")
            {
                continue;
            }
            cur /= part;
            dirs.push_back(cur);
        }
    }

    for (size_t i = 0; i < dirs.size(); ++i)
    {
        std::filesystem::path sel;
        if (i + 1 < dirs.size())
        {
            sel = dirs[i + 1];
        }
        AddColumn(dirs[i], sel);
    }

    if (mPath != nullptr)
    {
        Noesis::String path = RelPath(mFolder);
        if (path.Empty())
        {
            path = ".";
        }
        mPath->SetText(path.Str());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：创建可用的首字母筛选按钮；参数：无；返回值：无。
void ImagePicker::BuildLetters()
{
    if (mTabPanel == nullptr)
    {
        return;
    }

    bool avail[27] = {};
    for (const std::filesystem::path& path : mAll)
    {
        char ch = GetLetter(path);
        int idx = 26;
        if (ch >= 'A' && ch <= 'Z')
        {
            idx = ch - 'A';
        }
        avail[idx] = true;
    }

    mTabPanel->GetChildren()->Clear();
    mTabs.clear();
    for (int i = -1; i <= 26; ++i)
    {
        char value = 0;
        std::string name = "ALL";
        bool enabled = true;
        if (i >= 0 && i < 26)
        {
            value = (char)('A' + i);
            name.assign(1, value);
            enabled = avail[i];
        }
        else if (i == 26)
        {
            value = '#';
            name = "#";
            enabled = avail[26];
        }

        if (value == mLetter)
        {
            name = "[" + name + "]";
        }

        Noesis::Ptr<Noesis::Button> btn = Noesis::MakePtr<Noesis::Button>();
        Noesis::Ptr<Noesis::TextBlock> text = Noesis::MakePtr<Noesis::TextBlock>();
        text->SetText(name.c_str());
        btn->SetContent(text);
        btn->SetMinWidth(34.0f);
        btn->SetHeight(28.0f);
        btn->SetMargin(Noesis::Thickness(2.0f, 2.0f, 2.0f, 2.0f));
        btn->SetIsEnabled(enabled);
        btn->Click() += Noesis::MakeDelegate(this, &ImagePicker::OnLetter);
        mTabPanel->GetChildren()->Add(btn);
        mTabs.push_back({btn.GetPtr(), value});
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：按当前筛选创建递归文件列表；参数：无；返回值：无。
void ImagePicker::BuildFiles()
{
    if (mFilePanel == nullptr)
    {
        return;
    }

    mFilePanel->GetChildren()->Clear();
    mFiles.clear();
    uint32_t count = 0;
    uint32_t shown = 0;
    uint32_t limit = (uint32_t)mAll.size();
    if (mLetter == 0 && limit > 250)
    {
        limit = 250;
    }
    if (!sFile.empty() && std::find(mAll.begin(), mAll.end(), sFile) != mAll.end() &&
        (mLetter == 0 || GetLetter(sFile) == mLetter))
    {
        AddFile(sFile);
        ++shown;
    }
    for (const std::filesystem::path& path : mAll)
    {
        if (mLetter != 0 && GetLetter(path) != mLetter)
        {
            continue;
        }
        ++count;
        if (path == sFile)
        {
            continue;
        }
        if (shown < limit)
        {
            AddFile(path);
            ++shown;
        }
    }

    if (mCount != nullptr)
    {
        char text[64];
        snprintf(text, sizeof(text), "%u shown / %u matched", shown, count);
        mCount->SetText(text);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：递归扫描当前目录及所有子目录中的图片；参数：无；返回值：无。
void ImagePicker::ScanFiles()
{
    mAll.clear();
    std::error_code ec;
    std::filesystem::directory_options opt = std::filesystem::directory_options::skip_permission_denied;
    std::filesystem::recursive_directory_iterator it(mFolder, opt, ec);
    std::filesystem::recursive_directory_iterator end;
    while (it != end)
    {
        if (!ec && it->is_regular_file(ec) && IsImage(it->path()))
        {
            mAll.push_back(it->path());
        }
        it.increment(ec);
        if (ec)
        {
            ec.clear();
        }
    }
    std::sort(mAll.begin(), mAll.end(), &ImagePicker::LessPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：创建单层文件夹导航列；参数：dir 为列目录，sel 为选中子目录；返回值：无。
void ImagePicker::AddColumn(const std::filesystem::path& dir, const std::filesystem::path& sel)
{
    std::vector<std::filesystem::path> dirs;
    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    std::filesystem::directory_iterator end;
    while (it != end)
    {
        if (!ec && it->is_directory(ec))
        {
            dirs.push_back(it->path());
        }
        it.increment(ec);
        if (ec)
        {
            ec.clear();
        }
    }
    std::sort(dirs.begin(), dirs.end(), &ImagePicker::LessPath);

    Noesis::Ptr<Noesis::Border> box = Noesis::MakePtr<Noesis::Border>();
    Noesis::Ptr<Noesis::StackPanel> panel = Noesis::MakePtr<Noesis::StackPanel>();
    box->SetWidth(390.0f);
    box->SetMargin(Noesis::Thickness(0.0f, 0.0f, 0.0f, 8.0f));
    box->SetPadding(Noesis::Thickness(6.0f, 6.0f, 6.0f, 6.0f));

    Noesis::Ptr<Noesis::TextBlock> title = Noesis::MakePtr<Noesis::TextBlock>();
    std::string head = dir.filename().u8string();
    if (dir == mRoot)
    {
        head = "ROOT";
    }
    title->SetText(head.c_str());
    title->SetFontSize(13.0f);
    title->SetMargin(Noesis::Thickness(4.0f, 2.0f, 4.0f, 8.0f));
    panel->GetChildren()->Add(title);

    if (dirs.empty())
    {
        Noesis::Ptr<Noesis::TextBlock> empty = Noesis::MakePtr<Noesis::TextBlock>();
        empty->SetText("没有子文件夹");
        empty->SetOpacity(0.55f);
        empty->SetMargin(Noesis::Thickness(4.0f, 8.0f, 4.0f, 8.0f));
        panel->GetChildren()->Add(empty);
    }

    for (const std::filesystem::path& path : dirs)
    {
        std::string name = path.filename().u8string();
        if (!sel.empty() && path == sel)
        {
            name = "● " + name;
        }
        Noesis::Ptr<Noesis::Button> btn = Noesis::MakePtr<Noesis::Button>();
        Noesis::Ptr<Noesis::TextBlock> text = Noesis::MakePtr<Noesis::TextBlock>();
        text->SetText(name.c_str());
        btn->SetContent(text);
        btn->SetHeight(34.0f);
        btn->SetMargin(Noesis::Thickness(1.0f, 1.0f, 1.0f, 1.0f));
        btn->Click() += Noesis::MakeDelegate(this, &ImagePicker::OnFolder);
        panel->GetChildren()->Add(btn);
        mFolders.push_back({btn.GetPtr(), path});
    }

    box->SetChild(panel);
    mCols->GetChildren()->Add(box);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：创建一个图片文件按钮；参数：path 为图片绝对路径；返回值：无。
void ImagePicker::AddFile(const std::filesystem::path& path)
{
    Noesis::String rel = RelPath(path);
    std::string name = path.filename().u8string();
    if (path == sFile)
    {
        name = "● " + name;
    }

    Noesis::Ptr<Noesis::StackPanel> panel = Noesis::MakePtr<Noesis::StackPanel>();
    Noesis::Ptr<Noesis::StackPanel> info = Noesis::MakePtr<Noesis::StackPanel>();
    Noesis::Ptr<Noesis::Image> image = Noesis::MakePtr<Noesis::Image>();
    Noesis::Ptr<Noesis::TextBlock> title = Noesis::MakePtr<Noesis::TextBlock>();
    Noesis::Ptr<Noesis::TextBlock> detail = Noesis::MakePtr<Noesis::TextBlock>();
    panel->SetOrientation(Noesis::Orientation_Horizontal);
    image->SetWidth(44.0f);
    image->SetHeight(44.0f);
    image->SetStretch(Noesis::Stretch_Uniform);
    image->SetMargin(Noesis::Thickness(0.0f, 0.0f, 8.0f, 0.0f));
    Noesis::Uri uri = Noesis::Uri::Pack(sAsm.Str(), rel.Str());
    image->SetSource(Noesis::MakePtr<Noesis::BitmapImage>(uri));
    title->SetText(name.c_str());
    title->SetFontSize(13.0f);
    detail->SetText(rel.Str());
    detail->SetFontSize(10.0f);
    detail->SetOpacity(0.6f);
    info->GetChildren()->Add(title);
    info->GetChildren()->Add(detail);
    panel->GetChildren()->Add(image);
    panel->GetChildren()->Add(info);

    Noesis::Ptr<Noesis::Button> btn = Noesis::MakePtr<Noesis::Button>();
    btn->SetContent(panel);
    btn->SetWidth(240.0f);
    btn->SetHeight(62.0f);
    btn->SetMargin(Noesis::Thickness(3.0f, 3.0f, 3.0f, 3.0f));
    btn->Click() += Noesis::MakeDelegate(this, &ImagePicker::OnFile);
    mFilePanel->GetChildren()->Add(btn);
    mFiles.push_back({btn.GetPtr(), path});
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：判断文件是否为支持的图片；参数：path 为文件路径；返回值：是图片返回 true。
bool ImagePicker::IsImage(const std::filesystem::path& path)
{
    std::string ext = path.extension().u8string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch)
    {
        return (char)std::tolower(ch);
    });
    return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" ||
        ext == ".gif" || ext == ".tga" || ext == ".dds" || ext == ".webp";
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：取得文件名的筛选首字母；参数：path 为文件路径；返回值：A-Z 或 #。
char ImagePicker::GetLetter(const std::filesystem::path& path)
{
    std::string name = path.filename().u8string();
    if (name.empty())
    {
        return '#';
    }
    unsigned char raw = (unsigned char)name[0];
    char ch = (char)std::toupper(raw);
    if (ch >= 'A' && ch <= 'Z')
    {
        return ch;
    }
    return '#';
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：将绝对路径转换为项目相对路径；参数：path 为绝对路径；返回值：正斜杠相对路径。
Noesis::String ImagePicker::RelPath(const std::filesystem::path& path) const
{
    std::error_code ec;
    std::filesystem::path rel = std::filesystem::relative(path, mRoot, ec);
    if (ec)
    {
        return Noesis::String();
    }
    std::string text = rel.generic_u8string();
    return Noesis::String(text.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// 功能：按文件名进行不区分大小写排序；参数：left 和 right 为路径；返回值：left 应靠前时为 true。
bool ImagePicker::LessPath(const std::filesystem::path& left, const std::filesystem::path& right)
{
    std::string a = left.filename().u8string();
    std::string b = right.filename().u8string();
    std::transform(a.begin(), a.end(), a.begin(), [](unsigned char ch)
    {
        return (char)std::tolower(ch);
    });
    std::transform(b.begin(), b.end(), b.begin(), [](unsigned char ch)
    {
        return (char)std::tolower(ch);
    });
    if (a == b)
    {
        return left.generic_u8string() < right.generic_u8string();
    }
    return a < b;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
NS_BEGIN_COLD_REGION

NS_IMPLEMENT_REFLECTION(ImagePicker, "StudioTool.ImagePicker")
{
    Noesis::UIElementData* data = NsMeta<Noesis::UIElementData>(Noesis::TypeOf<SelfClass>());
    data->RegisterProperty<Noesis::String>(ValueProperty, "Value",
        Noesis::PropertyMetadata::Create(Noesis::String()));
}

NS_END_COLD_REGION

////////////////////////////////////////////////////////////////////////////////////////////////////
const Noesis::DependencyProperty* ImagePicker::ValueProperty;
