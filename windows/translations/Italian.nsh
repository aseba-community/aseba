!define LANG "ITALIAN" ; Must be the lang name define my NSIS

!insertmacro LANG_STRING STR_Package_Name  "Aseba Studio"
!insertmacro LANG_STRING STR_Package_Name_Devel  "Sviluppo ASEBA"
!insertmacro LANG_STRING STR_Package_Name_Thymio  "Aseba Studio per Thymio"
!insertmacro LANG_STRING STR_License_Top  "Questo è software libero sotto i termini della licenza LGPL v3 (in inglese sotto)."
!insertmacro LANG_STRING STR_Done  "Fatto."
!insertmacro LANG_STRING STR_Clean  "Pulizia di installazioni precedenti ..."
!insertmacro LANG_STRING STR_Install_Type  "Tipo di installazione"
!insertmacro LANG_STRING STR_Components  "Grazie a scegliere i componenti che volete includere"
!insertmacro LANG_STRING STR_Robot_Install "Per Thymio II (raccomandato)"
!insertmacro LANG_STRING STR_Aseba_Install "Solamente Aseba"
!insertmacro LANG_STRING STR_Previous_Install "È stata rilevata una precedente installazione. Si desidera continuare comunque?"

; Start menu
!insertmacro LANG_STRING STR_Uninstall  "Disinstallazione"
!insertmacro LANG_STRING STR_Simulations  "Simulazioni"
!insertmacro LANG_STRING STR_Tools  "Strumenti"
!insertmacro LANG_STRING STR_Cmd_Line  "Aprire un terminale"
!insertmacro LANG_STRING STR_USB_Restart  "USB Restart"
!insertmacro LANG_STRING STR_Doc_Dir  "Documentazione"
!insertmacro LANG_STRING STR_Doc_Browser_Offline  "Offline Aiuto"
!insertmacro LANG_STRING STR_Doc_Browser_Online  "Guida in linea (internet)"

; Install type
!insertmacro LANG_STRING STR_InstallFull  "Completa"
!insertmacro LANG_STRING STR_InstallRecommended  "Raccomandato"
!insertmacro LANG_STRING STR_InstallMin  "Minimale"

; Group and section descriptions
!insertmacro LANG_STRING DESC_GroupAseba  "Pacchetto di strumenti per il linguaggio ASEBA."
!insertmacro LANG_STRING DESC_SecStudioIDE  "Aseba Studio: un ambiente di sviluppo integrato per ASEBA."
!insertmacro LANG_STRING DESC_SecQt  "Librerie Qt4 e altre dipendenze."
!insertmacro LANG_STRING DESC_SecSim  "Simulazioni (e-puck, marXbot, ecc)."
!insertmacro LANG_STRING NAME_GroupCLI  "Strumenti CLI"
!insertmacro LANG_STRING DESC_GroupCLI  "Strumenti in linea di comando."
!insertmacro LANG_STRING DESC_SecSwitch  "Commutazione ASEBA."
!insertmacro LANG_STRING DESC_SecHttp  "Aseba Http è utile per collegarsi con Scratch."
!insertmacro LANG_STRING NAME_SecTools  "Altri strumenti"
!insertmacro LANG_STRING DESC_SecTools  "Altri strumenti. Solo per gli utenti con esperienza."

; Driver related
!insertmacro LANG_STRING STR_Drv_Install  "Installazione del driver per ThymioII ..."
!insertmacro LANG_STRING STR_Drv_Uninstall  "Disinstallare il driver per ThymioII ..."
!insertmacro LANG_STRING STR_Drv_32bits  "Abbiamo rilevato una installazione a 32 bit."
!insertmacro LANG_STRING STR_Drv_64bits  "Abbiamo rilevato una installazione a 64 bit."
!insertmacro LANG_STRING STR_Drv_Return_Code  "Il codice di ritorno è:"
!insertmacro LANG_STRING STR_Drv_Problem  "Il driver non può essere installato."

; Devcon related
!insertmacro LANG_STRING STR_Devcon_Install  "Installazione devcon.exe..."

; E-puck related
!insertmacro LANG_STRING STR_Epuck_Install  "Installazione del kit per l'e-puck ..."

; Uninstall
!insertmacro LANG_STRING STR_Uninst_Folder  "Rimozione del directory di installazione ..."
!insertmacro LANG_STRING STR_Uninst_Menu  "L'eliminazione del menu di scelta rapida e dei chiavi di registro ..."
