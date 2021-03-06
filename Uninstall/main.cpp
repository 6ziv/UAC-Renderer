#include <QApplication>
#include <QProcess>
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QEventLoop>
#include <QStringList>
#include <QPlainTextEdit>
#include "io.h"
#include "Windows.h"
#include "Shlwapi.h"
#include "tirun.h"
bool IsAdmin()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    if (!AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &AdministratorsGroup))
        {
            return false;
        }
    BOOL b = FALSE;
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
        {
            b = FALSE;
        }
    FreeSid(AdministratorsGroup);
    return b == TRUE;
}

int main(int argc, char *argv[])
{

    QApplication app(argc, argv);
    QWidget *MainPage=new QWidget(nullptr);
    MainPage->setGeometry(300,100,400,500);
    QListWidget *list=new QListWidget(MainPage);
    list->setGeometry(25,100,350,300);
    list->show();
    MainPage->show();

    list->addItem(QString::fromWCharArray(L"开始运行"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    DWORD Size=GetSystemDirectoryW(NULL,0);
    wchar_t *System=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+1)));
    wchar_t *Consent=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+32)));
    wchar_t *ConsentBackup=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+32)));
    wchar_t *ConsentTmp=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+32)));
    wchar_t *Suicide=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+128)));

    GetSystemDirectoryW(System,Size);
    swprintf(Consent,L"%s\\consent.exe",System);
    swprintf(ConsentBackup,L"%s\\consent.exe.bak",System);
    swprintf(ConsentTmp,L"%s\\consent.exe.backup",System);
    swprintf(Suicide,L"taskkill -f -im UACRenderer.Uninst.exe && del %s\\UACRenderer.Uninst.exe",System);
    list->addItem(QString::fromWCharArray(L"系统目录：")+QString::fromWCharArray(System));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    free(System);

    //Check Files:
    if(0!=_waccess(ConsentBackup,4)){
        list->addItem(QString::fromWCharArray(L"检查备份文件...缺失！"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"检查备份文件...存在！"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    //Know Yourself!
    wchar_t *SelfPath;
    DWORD PathLen;
    PathLen=MAX_PATH;
    while(PathLen<65536){
        SetLastError(0);
        SelfPath=reinterpret_cast<wchar_t*>(malloc(PathLen*sizeof(wchar_t)));
        DWORD ret=GetModuleFileNameW(NULL,SelfPath,PathLen);
        if(ret==0){
            list->addItem(QString::fromWCharArray(L"错误：无法获得程序路径！"));
            list->scrollToBottom();
            return app.exec();
        }
        if(ret==PathLen && GetLastError()==ERROR_INSUFFICIENT_BUFFER){
            free(SelfPath);
            PathLen+=MAX_PATH;
            continue;
        }
        break;
    }
    list->addItem(QString::fromWCharArray(L"程序路径：")+QString::fromWCharArray(SelfPath));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    //Ensure that this application is run as Administrator
    list->addItem(QString::fromWCharArray(L"正在检查管理员权限"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    if(!IsAdmin()){
        list->addItem(QString::fromWCharArray(L"正在试图获取管理员权限"));
        list->scrollToBottom();
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        INT NewExe=(INT)ShellExecuteW(NULL,L"runas",SelfPath,NULL,NULL,SW_SHOW);
        if(NewExe<=32){
            list->addItem(QString::fromWCharArray(L"未能获取管理员权限"));
            list->scrollToBottom();
            app.processEvents(QEventLoop::ExcludeUserInputEvents);
            return app.exec();
        }
        else{
            ExitProcess(0);
        }
    }
    list->addItem(QString::fromWCharArray(L"已获得管理员权限"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    list->addItem(QString::fromWCharArray(L"检查TI特权"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
        bool TI=argc==2 && strcmp(argv[1],"-TI")==0;
        if(!TI){
            list->addItem(QString::fromWCharArray(L"未观测到Trusted Installer记号："));

            list->addItem(QString::fromWCharArray(L"试图获取TrustedInstaller特权"));
            list->scrollToBottom();
            app.processEvents(QEventLoop::ExcludeUserInputEvents);
            //Run Me As TrustedInstaller!
            wchar_t *PathWithPara=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(PathLen+32)));
            swprintf(PathWithPara,L"%s -TI",SelfPath);
            CreateProcessAsTrustedInstaller(PathWithPara);
            free(PathWithPara);
            ExitProcess(0);
        }else{
            list->addItem(QString::fromWCharArray(L"观测到Trusted Installer记号。"));
            list->addItem(QString::fromWCharArray(L"假定已具有TI特权。"));
            list->scrollToBottom();
            app.processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    free(SelfPath);
    list->addItem(QString::fromWCharArray(L"TI特权标签检验毕!"));
    list->addItem(QString::fromWCharArray(L"正在迁移consent.exe至consent.exe.backup"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    bool BackupOK=MoveFile(Consent,ConsentTmp);
    if(!BackupOK){
        list->addItem(QString::fromWCharArray(L"迁移失败。"));
        list->addItem(QString::fromWCharArray(L"卸载中止"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"迁移完成。"));
    list->addItem(QString::fromWCharArray(L"正在迁移consent.exe.bak至consent.exe"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    bool RestoreOK=MoveFile(ConsentBackup,Consent);
    if(!RestoreOK){
        list->addItem(QString::fromWCharArray(L"迁移失败。"));
        list->addItem(QString::fromWCharArray(L"卸载中止"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"迁移完成。"));
    list->addItem(QString::fromWCharArray(L"卸载程序执行完毕"));
    list->addItem(QString::fromWCharArray(L"请测试软件运行是否正常。"));
    list->addItem(QString::fromWCharArray(L"如果软件无法正常运行，请点击下方\"回滚\"按键，卸载程序会将操作复原。"));
    list->addItem(QString::fromWCharArray(L"如果软件正常运行，您可以按\"确认\"以退出。卸载程序会完成一些收尾的清理工作。"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    //do some testing

    QPushButton *pbOK=new QPushButton(MainPage);
    pbOK->setGeometry(25,425,150,25);
    pbOK->setText(QString::fromWCharArray(L"确认"));
    pbOK->show();

    QPushButton *pbRollBack=new QPushButton(MainPage);
    pbRollBack->setGeometry(225,425,150,25);
    pbRollBack->setText(QString::fromWCharArray(L"回滚"));
    pbRollBack->show();

    pbRollBack->connect(pbRollBack,&QPushButton::pressed,list,[&]{
        pbRollBack->setEnabled(false);
        pbOK->setEnabled(false);
        for(int i=0;i<10;i++){
            if(0==MoveFileW(Consent,ConsentBackup)){
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试还原备份：失败"));
            }else{
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试还原备份：成功"));
                break;
            }
        }

        for(int i=0;i<10;i++){
            if(0==MoveFileW(ConsentTmp,Consent)){
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试还原consent：失败"));
            }else{
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试还原consent：成功"));
                break;
            }
        }
    });

    pbOK->connect(pbOK,&QPushButton::pressed,list,[&]{
        pbRollBack->setEnabled(false);
        pbOK->setEnabled(false);
        list->addItem(QString::fromWCharArray(L"正在移除卸载信息"));
        list->scrollToBottom();
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        DWORD ret=RegDeleteKeyA(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UACRenderer");
        if(ERROR_SUCCESS==ret){
            list->addItem(QString::fromWCharArray(L"成功移除卸载信息"));
        }else{
            list->addItem(QString::fromWCharArray(L"未能移除卸载信息"));
        }
        list->addItem(QString::fromWCharArray(L"正在移除备份文件"));
        list->scrollToBottom();
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        if(DeleteFileW(ConsentTmp)){
            list->addItem(QString::fromWCharArray(L"成功移除备份文件"));
        }else{
            list->addItem(QString::fromWCharArray(L"未能移除备份文件"));
        }
        //list->addItem(QString::fromWCharArray(L"卸载完成！您可能需要手动删除")+QString::fromLocal8Bit(argv[0]));
        list->scrollToBottom();
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        DWORD rtn=MessageBoxW(0,L"是否要关闭测试模式？",L"测试模式",MB_YESNO);
        if(ret!=IDYES){
            list->addItem(QString::fromWCharArray(L"用户保留了测试模式。安装完成。"));
            list->scrollToBottom();
        //    return app.exec();
        }else{
            list->addItem(QString::fromWCharArray(L"正在关闭测试模式..."));
            list->addItem(QString::fromWCharArray(L"命令行：bcdedit /set testsigning on"));
            app.processEvents(QEventLoop::ExcludeUserInputEvents);
            system("bcdedit /set testsigning off");
            list->addItem(QString::fromWCharArray(L"安装完成。重启后测试模式将会关闭。"));
            list->scrollToBottom();
        //    return app.exec();
        }
        _wsystem(Suicide);
        return app.exec();
    });
    return app.exec();
}
