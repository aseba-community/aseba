!define LANG "ENGLISH" ; Must be the lang name define my NSIS

!insertmacro LANG_STRING STR_Package_Name  "Aseba Studio"
!insertmacro LANG_STRING STR_Package_Name_Devel  "Aseba Development"
!insertmacro LANG_STRING STR_Package_Name_Thymio  "Aseba Studio for Thymio"
!insertmacro LANG_STRING STR_License_Top  "This software is open source, licensed under LGPL version 3."
!insertmacro LANG_STRING STR_Done  "Done."
!insertmacro LANG_STRING STR_Clean  "Cleaning any previous installation..."
!insertmacro LANG_STRING STR_Install_Type  "Installation type"
!insertmacro LANG_STRING STR_Components  "Please choose the components you want to include"
!insertmacro LANG_STRING STR_Robot_Install "ThymioII Package (Recommended)"
!insertmacro LANG_STRING STR_Aseba_Install "Only Aseba"
!insertmacro LANG_STRING STR_Previous_Install "Previous installation detected. Do you still want to continue?"

; Start menu
!insertmacro LANG_STRING STR_Uninstall  "Uninstall"
!insertmacro LANG_STRING STR_Simulations  "Simulations"
!insertmacro LANG_STRING STR_Tools  "Tools"
!insertmacro LANG_STRING STR_Cmd_Line  "Open terminal"
!insertmacro LANG_STRING STR_USB_Restart  "Restart USB port"
!insertmacro LANG_STRING STR_Doc_Dir  "Documentation"
!insertmacro LANG_STRING STR_Doc_Browser_Offline  "Offline help (on disk)"
!insertmacro LANG_STRING STR_Doc_Browser_Online  "Online help (internet)"

; Install type
!insertmacro LANG_STRING STR_InstallFull  "Full"
!insertmacro LANG_STRING STR_InstallRecommended  "Recommended"
!insertmacro LANG_STRING STR_InstallMin  "Minimal"

; Group and section descriptions
!insertmacro LANG_STRING DESC_GroupAseba  "The Aseba language tools suite."
!insertmacro LANG_STRING DESC_SecStudioIDE  "Aseba Studio: an integrated development environment for Aseba."
!insertmacro LANG_STRING DESC_SecQt  "The Qt4 libraries and other dependencies."
!insertmacro LANG_STRING DESC_SecSim  "Fun simulations (e-puck, marXbot,...)."
!insertmacro LANG_STRING NAME_GroupCLI  "CLI Tools"
!insertmacro LANG_STRING DESC_GroupCLI  "Command Line Tools."
!insertmacro LANG_STRING DESC_SecSwitch  "Aseba Switch."
!insertmacro LANG_STRING DESC_SecHttp  "Aseba Http is usefull to connect with Scratch."
!insertmacro LANG_STRING NAME_SecTools  "Other Tools"
!insertmacro LANG_STRING DESC_SecTools  "Other optional tools. Only for expert users."

; Driver related
!insertmacro LANG_STRING STR_Drv_Install  "Installing the ThymioII driver..."
!insertmacro LANG_STRING STR_Drv_Uninstall  "Removing the ThymioII driver..."
!insertmacro LANG_STRING STR_Drv_32bits  "We have detected a 32-bits installation."
!insertmacro LANG_STRING STR_Drv_64bits  "We have detected a 64-bits installation."
!insertmacro LANG_STRING STR_Drv_Return_Code  "Return code is: "
!insertmacro LANG_STRING STR_Drv_Problem  "Driver could not be installed."

; Devcon related
!insertmacro LANG_STRING STR_Devcon_Install  "Installing devcon.exe..."

; E-puck related
!insertmacro LANG_STRING STR_Epuck_Install  "Installing the e-puck kit..."

; Uninstall
!insertmacro LANG_STRING STR_Uninst_Folder  "Removing the installation folder..."
!insertmacro LANG_STRING STR_Uninst_Menu  "Removing the start menu shortcuts and registry keys..."
