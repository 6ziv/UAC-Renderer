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

    //Check Reg
    HKEY Checker;
    DWORD rt=RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UACRenderer",0,NULL,&Checker);
    if(rt==ERROR_SUCCESS){
        list->addItem(QString::fromWCharArray(L"检测到已安装的副本，停止。"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"开始运行"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);



    //Set Working Directory!
    wchar_t *MyDir=reinterpret_cast<wchar_t*>(malloc((QApplication::applicationDirPath().length()+1)*sizeof(wchar_t)));
    for(int i=0;i<QApplication::applicationDirPath().length();i++)MyDir[i]=QApplication::applicationDirPath().toStdWString().at(i);
    MyDir[QApplication::applicationDirPath().length()]=0;
    SetCurrentDirectoryW(MyDir);
    list->addItem(QString::fromWCharArray(L"设置工作目录：")+QString::fromWCharArray(MyDir));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    free(MyDir);

    //Check Files:
    if(0!=_access("Setup.tar",4)){
        list->addItem(QString::fromWCharArray(L"检查数据文件...缺失！"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"检查数据文件...存在！"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    if(0!=_access("7z.exe",4)){
        list->addItem(QString::fromWCharArray(L"检查7-zip...缺失！"));
        list->scrollToBottom();
        return app.exec();
    }
    if(0!=_access("7z.dll",4)){
        list->addItem(QString::fromWCharArray(L"检查7-zip...缺失！"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"检查7-zip...存在！"));
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

    //Check Testsigning
    list->addItem(QString::fromWCharArray(L"正在查询测试模式开关"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    HKEY hKey;
    LSTATUS RegStatus=RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"System\\CurrentControlSet\\Control",0,KEY_QUERY_VALUE,&hKey);
    if(RegStatus!=ERROR_SUCCESS){
        list->addItem(QString::fromWCharArray(L"未能获取到测试模式开关状态"));
        list->scrollToBottom();
        return app.exec();
    }
    DWORD dwSize;
    RegStatus=RegQueryValueExW(hKey,L"SystemStartOptions",NULL,NULL,NULL,&dwSize);
    if(RegStatus!=ERROR_SUCCESS){
        list->addItem(QString::fromWCharArray(L"未能获取到测试模式开关状态"));
        list->scrollToBottom();
        return app.exec();
    }
    wchar_t *StartOptions=reinterpret_cast<wchar_t*>(malloc(dwSize));
    RegStatus=RegQueryValueExW(hKey,L"SystemStartOptions",NULL,NULL,reinterpret_cast<LPBYTE>(StartOptions),&dwSize);
    if(RegStatus!=ERROR_SUCCESS){
        list->addItem(QString::fromWCharArray(L"未能获取到测试模式开关状态"));
        list->scrollToBottom();
        return app.exec();
    }
    if(StrStrIW(StartOptions,L"TESTSIGNING")==NULL){
        free(StartOptions);
        list->addItem(QString::fromWCharArray(L"测试模式未打开，是否开启？"));
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        int ret=MessageBoxW(0,L"开启测试模式可能导致系统安全性下降，是否继续？",L"测试模式",MB_YESNO);
        if(ret!=IDYES){
            list->addItem(QString::fromWCharArray(L"用户拒绝了开启测试模式。安装中断。"));
            list->scrollToBottom();
            return app.exec();
        }else{
            list->addItem(QString::fromWCharArray(L"正在开启测试模式..."));
            list->addItem(QString::fromWCharArray(L"命令行：bcdedit /set testsigning on"));
            app.processEvents(QEventLoop::ExcludeUserInputEvents);
            system("bcdedit /set testsigning on");
            list->addItem(QString::fromWCharArray(L"操作完成。请在重启后继续运行此安装程序。"));
            list->scrollToBottom();
            return app.exec();
        }
    }
    free(StartOptions);
    list->addItem(QString::fromWCharArray(L"已开启测试模式"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    DWORD Size=GetSystemDirectoryW(NULL,0);
    wchar_t *System=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+1)));
    wchar_t *Consent=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+32)));
    wchar_t *ConsentBackup=reinterpret_cast<wchar_t*>(malloc(sizeof(wchar_t)*(Size+32)));
    GetSystemDirectoryW(System,Size);
    swprintf(Consent,L"%s\\consent.exe",System);
    swprintf(ConsentBackup,L"%s\\consent.exe.bak",System);
    list->addItem(QString::fromWCharArray(L"系统目录：")+QString::fromWCharArray(System));
    list->addItem(QString::fromWCharArray(L"检查文件权限"));
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
    list->addItem(QString::fromWCharArray(L"正在迁移consent.exe至consent.exe.bak"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    bool BackupOK=MoveFile(Consent,ConsentBackup);
    if(!BackupOK){
        list->addItem(QString::fromWCharArray(L"迁移失败。"));
        list->addItem(QString::fromWCharArray(L"这可能是因为可能您已经安装过本软件，或提升至TI权限失败。"));
        list->scrollToBottom();
        return app.exec();
    }
    list->addItem(QString::fromWCharArray(L"迁移完成。"));
    list->addItem(QString::fromWCharArray(L"准备解压"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    wchar_t *CommandLine;
    CommandLine=reinterpret_cast<wchar_t*>(malloc((Size+64)*sizeof(wchar_t)));
    swprintf(CommandLine,L"7z.exe x Setup.tar -o%s -aoa",System);
//    free(System);
    list->addItem(QString::fromWCharArray(L"执行参数：")+QString::fromWCharArray(CommandLine));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    memset(&si,0,sizeof(STARTUPINFO));
    memset(&pi,0,sizeof(PROCESS_INFORMATION));
    si.cb=sizeof(STARTUPINFO);

    //Creating a pipe for communication.
    HANDLE Reader,Writer;
    _SECURITY_ATTRIBUTES sa;
    sa.nLength=sizeof(_SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor=NULL;
    sa.bInheritHandle=TRUE;
    CreatePipe(&Reader,&Writer,&sa,0);
    si.hStdInput=Writer;
    si.hStdOutput=Reader;
    si.hStdError=Reader;
    si.dwFlags=STARTF_USESTDHANDLES;
    list->addItem(QString::fromWCharArray(L"开始执行"));
    list->scrollToBottom();
    app.processEvents(QEventLoop::ExcludeUserInputEvents);
    CreateProcessW(NULL,CommandLine,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
    WaitForSingleObject(pi.hProcess,INFINITE);
    CloseHandle(Reader);
    CloseHandle(Writer);


    list->addItem(QString::fromWCharArray(L"执行完毕"));
    list->addItem(QString::fromWCharArray(L"请测试软件运行是否正常。"));
    list->addItem(QString::fromWCharArray(L"如果软件无法正常运行，请点击下方\"回滚\"按钮"));
    list->addItem(QString::fromWCharArray(L"如果软件正常运行，您可以按\"确认\"以完成最后的工作。"));
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
    pbOK->connect(pbOK,&QPushButton::pressed,list,[&]{
        pbRollBack->setEnabled(false);
        pbOK->setEnabled(false);
        list->addItem(QString::fromWCharArray(L"正在写入卸载信息"));
        list->scrollToBottom();
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
        HKEY appReg;
        DWORD Type;
        DWORD ret=RegCreateKeyExA(HKEY_LOCAL_MACHINE,"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\UACRenderer",0,NULL,0,KEY_ALL_ACCESS,NULL,&appReg,&Type);
        if(ERROR_SUCCESS==ret){
            RegSetValueExW(appReg,L"DisplayIcon",0,REG_SZ,(const BYTE *)Consent,sizeof(wchar_t)*(wcslen(Consent)+1));
            RegSetValueExW(appReg,L"DisplayName",0,REG_SZ,(const BYTE *)L"UAC Renderer",sizeof(wchar_t)*(wcslen(L"UAC Renderer")+1));
            RegSetValueExW(appReg,L"DisplayVersion",0,REG_SZ,(const BYTE *)L"1.0.0",sizeof(wchar_t)*(wcslen(L"1.0.0")+1));
            RegSetValueExW(appReg,L"Publisher",0,REG_SZ,(const BYTE *)L"6ziv, Individual",sizeof(wchar_t)*(wcslen(L"6ziv, Individual")+1));
            RegSetValueExW(appReg,L"URLInfoAbout",0,REG_SZ,(const BYTE *)L"https://github.com/6ziv/Custom-Samples/tree/master/UAC",sizeof(wchar_t)*(wcslen(L"https://github.com/6ziv/Custom-Samples/tree/master/UAC")+1));
            RegSetValueExW(appReg,L"InstallLocation",0,REG_SZ,(const BYTE *)System,sizeof(wchar_t)*(wcslen(System)+1));
            RegSetValueExW(appReg,L"Comments",0,REG_SZ,(const BYTE *)L"喵呜！",sizeof(wchar_t)*(wcslen(L"喵呜！")+1));

            wchar_t *uninst=reinterpret_cast<wchar_t*>(malloc((Size+32)*sizeof(wchar_t)));
            swprintf(ConsentBackup,L"%s\\UACRenderer.Uninst.exe",System);
            RegSetValueExW(appReg,L"UninstallString",0,REG_SZ,(const BYTE *)ConsentBackup,sizeof(wchar_t)*(wcslen(ConsentBackup)+1));
            DWORD tmpDWORD;
            tmpDWORD=0x00000000;
            RegSetValueExW(appReg,L"VersionMinor",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));
            tmpDWORD=0x00000001;
            RegSetValueExW(appReg,L"VersionMajor",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));
            tmpDWORD=0x01000000;
            RegSetValueExW(appReg,L"Version",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));
            tmpDWORD=0x00000001;
            RegSetValueExW(appReg,L"NoModify",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));
            tmpDWORD=0x00000001;
            RegSetValueExW(appReg,L"NoRepair",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));
            tmpDWORD=0x00002000;
            RegSetValueExW(appReg,L"EstimatedSize",0,REG_DWORD,(const BYTE *)(&tmpDWORD),sizeof(DWORD));

        }else{
            list->addItem(QString::fromWCharArray(L"无法创建目标Key"));
        }

        list->addItem(QString::fromWCharArray(L"Bye~"));
        list->scrollToBottom();
        free(System);
        app.processEvents(QEventLoop::ExcludeUserInputEvents);
    });
    pbRollBack->connect(pbRollBack,&QPushButton::pressed,list,[&]{
        pbRollBack->setEnabled(false);
        pbOK->setEnabled(false);
        free(System);
        for(int i=0;i<10;i++){
            if(0==DeleteFileW(Consent)){
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试删除文件：失败"));
            }else{
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试删除文件：成功"));
                break;
            }
        }

        for(int i=0;i<10;i++){
            if(0==MoveFileW(ConsentBackup,Consent)){
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试移动文件：失败"));
            }else{
                list->addItem(QString::fromWCharArray(L"第")+QString::number(i+1,10)+QString::fromWCharArray(L"次尝试移动文件：成功"));
                break;
            }
        }
    });
    return app.exec();
}
