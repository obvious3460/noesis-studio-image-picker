////////////////////////////////////////////////////////////////////////////////////////////////////
// Noesis Studio 图片资源选择器
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __STUDIOTOOL_IMAGEPICKER_H__
#define __STUDIOTOOL_IMAGEPICKER_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/String.h>
#include <NsGui/Button.h>
#include <NsGui/FrameworkElement.h>
#include <NsGui/Popup.h>
#include <NsGui/StackPanel.h>
#include <NsGui/TextBlock.h>
#include <NsGui/UserControl.h>
#include <NsGui/UIElementEvents.h>
#include <NsGui/WrapPanel.h>

#include <filesystem>
#include <vector>


class ImagePicker final: public Noesis::UserControl
{
public:
    /// 功能：创建图片选择器并绑定界面控件；参数：无；返回值：无。
    ImagePicker();

    /// 功能：取得当前图片资源路径；参数：无；返回值：资源相对路径。
    const char* GetValue() const;

    /// 功能：设置当前图片资源路径；参数：value 为资源相对路径；返回值：无。
    void SetValue(const char* value);

    /// 功能：设置当前 Noesis 项目的资源根目录；参数：root 为项目目录；返回值：无。
    static void SetRoot(const char* root, const char* assembly);

    /// 功能：安装图片属性文字点击处理和共享选择弹窗；参数：studio 为 Studio 根控件；返回值：无。
    static void Install(Noesis::FrameworkElement* studio);

public:
    static const Noesis::DependencyProperty* ValueProperty;

private:
    struct PickRef
    {
        Noesis::Button* btn;
        std::filesystem::path path;
    };

    struct TabRef
    {
        Noesis::Button* btn;
        char letter;
    };

    /// 功能：打开选择弹窗并定位当前资源目录；参数：sender 为事件源，args 为事件参数；返回值：无。
    void OnOpen(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args);

    /// 功能：关闭图片选择弹窗；参数：sender 为事件源，args 为事件参数；返回值：无。
    void OnClose(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args);

    /// 功能：切换当前文件夹；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
    void OnFolder(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args);

    /// 功能：切换文件首字母筛选；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
    void OnLetter(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args);

    /// 功能：选择文件、回填路径并关闭弹窗；参数：sender 为被点击按钮，args 为事件参数；返回值：无。
    void OnFile(Noesis::BaseComponent* sender, const Noesis::RoutedEventArgs& args);

    /// 功能：设置当前目录并刷新全部列表；参数：dir 为目标目录，restore 表示是否恢复筛选；返回值：无。
    void SetFolder(const std::filesystem::path& dir, bool restore = false);

    /// 功能：取得图片选择器状态文件路径；参数：无；返回值：状态文件路径。
    static std::filesystem::path StatePath();

    /// 功能：读取上次浏览的目录、筛选和文件；参数：无；返回值：无。
    static void LoadState();

    /// 功能：保存当前浏览的目录、筛选和文件；参数：无；返回值：无。
    void SaveState() const;

    /// 功能：按当前目录层级创建文件夹列；参数：无；返回值：无。
    void BuildFolders();

    /// 功能：创建可用的首字母筛选按钮；参数：无；返回值：无。
    void BuildLetters();

    /// 功能：按当前筛选创建递归文件列表；参数：无；返回值：无。
    void BuildFiles();

    /// 功能：递归扫描当前目录及所有子目录中的图片；参数：无；返回值：无。
    void ScanFiles();

    /// 功能：创建单层文件夹导航列；参数：dir 为列目录，sel 为选中子目录；返回值：无。
    void AddColumn(const std::filesystem::path& dir, const std::filesystem::path& sel);

    /// 功能：创建一个图片文件按钮；参数：path 为图片绝对路径；返回值：无。
    void AddFile(const std::filesystem::path& path);

    /// 功能：判断文件是否为支持的图片；参数：path 为文件路径；返回值：是图片返回 true。
    static bool IsImage(const std::filesystem::path& path);

    /// 功能：取得文件名的筛选首字母；参数：path 为文件路径；返回值：A-Z 或 #。
    static char GetLetter(const std::filesystem::path& path);

    /// 功能：将绝对路径转换为项目相对路径；参数：path 为绝对路径；返回值：正斜杠相对路径。
    Noesis::String RelPath(const std::filesystem::path& path) const;

    /// 功能：按文件名进行不区分大小写排序；参数：left 和 right 为路径；返回值：left 应靠前时为 true。
    static bool LessPath(const std::filesystem::path& left, const std::filesystem::path& right);

private:
    static Noesis::String sRoot;
    static Noesis::String sAsm;
    static Noesis::EventHandler sBase;
    static bool sHave;
    static std::filesystem::path sFolder;
    static std::filesystem::path sFile;
    static char sLetter;

    std::filesystem::path mRoot;
    std::filesystem::path mFolder;
    std::vector<std::filesystem::path> mAll;
    std::vector<PickRef> mFolders;
    std::vector<PickRef> mFiles;
    std::vector<TabRef> mTabs;

    Noesis::Popup* mPopup;
    Noesis::StackPanel* mCols;
    Noesis::WrapPanel* mTabPanel;
    Noesis::WrapPanel* mFilePanel;
    Noesis::TextBlock* mPath;
    Noesis::TextBlock* mCount;
    char mLetter;

    NS_DECLARE_REFLECTION(ImagePicker, Noesis::UserControl)
};


#endif
