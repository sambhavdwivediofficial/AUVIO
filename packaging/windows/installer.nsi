!include "MUI2.nsh"

Name "AUVIO"
OutFile "AUVIO-Setup.exe"
InstallDir "$PROGRAMFILES64\AUVIO"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "AUVIO" SecMain
    SetOutPath "$INSTDIR"
    File "..\..\build\windows-release\bin\AUVIO.exe"
    File /r "..\..\build\windows-release\bin\*.dll"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\AUVIO"
    CreateShortcut "$SMPROGRAMS\AUVIO\AUVIO.lnk" "$INSTDIR\AUVIO.exe"
    CreateShortcut "$DESKTOP\AUVIO.lnk" "$INSTDIR\AUVIO.exe"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AUVIO" \
        "DisplayName" "AUVIO"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AUVIO" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\AUVIO.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\AUVIO\AUVIO.lnk"
    Delete "$DESKTOP\AUVIO.lnk"
    RMDir "$SMPROGRAMS\AUVIO"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\AUVIO"
SectionEnd
