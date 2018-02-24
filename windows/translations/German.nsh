!define LANG "GERMAN" ; Must be the lang name define my NSIS

!insertmacro LANG_STRING STR_Package_Name  "Aseba Studio"
!insertmacro LANG_STRING STR_Package_Name_Devel  "Aseba Development"
!insertmacro LANG_STRING STR_Package_Name_Thymio  "Aseba Studio für Thymio"
!insertmacro LANG_STRING STR_License_Top  "Diese Software ist Open Source unter LGPL Version 3 lizenziert."
!insertmacro LANG_STRING STR_Done  "Fertig."
!insertmacro LANG_STRING STR_Clean  "Reinigung jede frühere Installation ..."
!insertmacro LANG_STRING STR_Install_Type  "Installationstyp"
!insertmacro LANG_STRING STR_Components  "Bitte wählen Sie die Komponenten die Sie hinzufügen möchten"
!insertmacro LANG_STRING STR_Robot_Install "ThymioII Package (Empfohlen)"
!insertmacro LANG_STRING STR_Aseba_Install "Nur Aseba"
!insertmacro LANG_STRING STR_Previous_Install "Alte Installation gefunden. Wollen Sie immer noch weiter?"

; Start menu
!insertmacro LANG_STRING STR_Uninstall  "Uninstall"
!insertmacro LANG_STRING STR_Simulations  "Simulationen"
!insertmacro LANG_STRING STR_Tools  "Tools"
!insertmacro LANG_STRING STR_Cmd_Line  "Öffnen Sie ein Terminal"
!insertmacro LANG_STRING STR_USB_Restart  "Restart USB-Anschluss"
!insertmacro LANG_STRING STR_Doc_Dir  "Dokumentation"
!insertmacro LANG_STRING STR_Doc_Browser_Offline  "Offline-Hilfe (auf Festplatte)"
!insertmacro LANG_STRING STR_Doc_Browser_Online  "Online-Hilfe (Internet)"

; Install type
!insertmacro LANG_STRING STR_InstallFull  "Vollständige"
!insertmacro LANG_STRING STR_InstallRecommended  "Empfohlen"
!insertmacro LANG_STRING STR_InstallMin  "Minimal"

; Group and section descriptions
!insertmacro LANG_STRING DESC_GroupAseba  "Die Aseba Sprachtools Suite."
!insertmacro LANG_STRING DESC_SecStudioIDE  "Aseba Studio: eine integrierte Entwicklungsumgebung für Aseba."
!insertmacro LANG_STRING DESC_SecQt  "Die Qt4-Bibliotheken und anderen Abhängigkeiten."
!insertmacro LANG_STRING DESC_SecSim  "Fun-Simulationen (E-Puck, marXbot, ...)."
!insertmacro LANG_STRING NAME_GroupCLI  "CLI-Tools"
!insertmacro LANG_STRING DESC_GroupCLI  "Command Line Tools."
!insertmacro LANG_STRING DESC_SecSwitch  "Aseba Switch."
!insertmacro LANG_STRING DESC_SecHttp  "Aseba Http ist sinnvoll, um mit Scratch verbinden."
!insertmacro LANG_STRING NAME_SecTools  "Sonstiges Tools"
!insertmacro LANG_STRING DESC_SecTools  "Weiter sonstiges Tools. Nur für erfahrene Benutzer."

; Driver related
!insertmacro LANG_STRING STR_Drv_Install  "Installieren des Treibers ThymioII..."
!insertmacro LANG_STRING STR_Drv_Uninstall  "Der ThymioII Treiber Entfernen..."
!insertmacro LANG_STRING STR_Drv_32bits  "Wir haben eine 32-Bit-Installation detektiert."
!insertmacro LANG_STRING STR_Drv_64bits  "Wir haben eine 64-Bit-Installation detektiert."
!insertmacro LANG_STRING STR_Drv_Return_Code  "Rückkehrcode ist: "
!insertmacro LANG_STRING STR_Drv_Problem  "Treiber konnte nicht installiert werden."

; Devcon related
!insertmacro LANG_STRING STR_Devcon_Install  "Installieren devcon.exe..."

; E-puck related
!insertmacro LANG_STRING STR_Epuck_Install  "Installieren des E-Puck Kit"

; Uninstall
!insertmacro LANG_STRING STR_Uninst_Folder  "Entfernen Sie den Installationsordner..."
!insertmacro LANG_STRING STR_Uninst_Menu  "Entfernen der Startmenü-Verknüpfungen und Registry-Schlüssel..."
