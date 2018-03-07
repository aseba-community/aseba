!define LANG "FRENCH" ; Must be the lang name define my NSIS

!insertmacro LANG_STRING STR_Package_Name  "Aseba Studio"
!insertmacro LANG_STRING STR_Package_Name_Devel  "Développement pour Aseba"
!insertmacro LANG_STRING STR_Package_Name_Thymio  "Aseba Studio pour Thymio"
!insertmacro LANG_STRING STR_License_Top  "Ce logiciel est libre, selon les termes de la licence LGPL version 3 (en anglais ci-dessous)."
!insertmacro LANG_STRING STR_Done  "Fait."
!insertmacro LANG_STRING STR_Clean  "Nettoyage des précédentes installations..."
!insertmacro LANG_STRING STR_Install_Type  "Type d'installation"
!insertmacro LANG_STRING STR_Components  "Merci de choisir les composants que vous voulez inclure"
!insertmacro LANG_STRING STR_Robot_Install "Pour ThymioII (Recommandé)"
!insertmacro LANG_STRING STR_Aseba_Install "Uniquement Aseba"
!insertmacro LANG_STRING STR_Previous_Install "Une précédente installation a été détectée. Voulez-vous quand même continuer ?"

; Start menu
!insertmacro LANG_STRING STR_Uninstall  "Désinstallation"
!insertmacro LANG_STRING STR_Simulations  "Simulations"
!insertmacro LANG_STRING STR_Tools  "Outils"
!insertmacro LANG_STRING STR_Cmd_Line  "Ouvrir un terminal"
!insertmacro LANG_STRING STR_USB_Restart  "Redémarrer le port USB"
!insertmacro LANG_STRING STR_Doc_Dir  "Documentation"
!insertmacro LANG_STRING STR_Doc_Browser_Offline  "Aide hors ligne"
!insertmacro LANG_STRING STR_Doc_Browser_Online  "Aide en ligne (internet)"

; Install type
!insertmacro LANG_STRING STR_InstallFull  "Complète"
!insertmacro LANG_STRING STR_InstallRecommended  "Recommandée"
!insertmacro LANG_STRING STR_InstallMin  "Minimale"

; Group and section descriptions
!insertmacro LANG_STRING DESC_GroupAseba  "La suite d'outils pour le language Aseba."
!insertmacro LANG_STRING DESC_SecStudioIDE  "Aseba Studio: un environement intégré de développement pour Aseba."
!insertmacro LANG_STRING DESC_SecQt  "Les librairies Qt4 et autres dépendances."
!insertmacro LANG_STRING DESC_SecSim  "Simulations fun (e-puck, marXbot, etc.)."
!insertmacro LANG_STRING NAME_GroupCLI  "Outils CLI"
!insertmacro LANG_STRING DESC_GroupCLI  "Outils en ligne de commande."
!insertmacro LANG_STRING DESC_SecSwitch  "Aseba Switch."
!insertmacro LANG_STRING DESC_SecHttp  "Aseba Http est util pour se connecter avec Scratch."
!insertmacro LANG_STRING NAME_SecTools  "Autres Outils"
!insertmacro LANG_STRING DESC_SecTools  "Autres outils. Seulement pour utilisateurs expérimentés."

; Driver related
!insertmacro LANG_STRING STR_Drv_Install  "Installation du driver pour ThymioII..."
!insertmacro LANG_STRING STR_Drv_Uninstall  "Désinstallation du driver pour ThymioII..."
!insertmacro LANG_STRING STR_Drv_32bits  "Nous avons détecté une installation 32-bits."
!insertmacro LANG_STRING STR_Drv_64bits  "Nous avons détecté une installation 64-bits."
!insertmacro LANG_STRING STR_Drv_Return_Code  "Le code de retour est: "
!insertmacro LANG_STRING STR_Drv_Problem  "Le driver n'a pas pu être installé."

; Devcon related
!insertmacro LANG_STRING STR_Devcon_Install  "Installation de devcon.exe..."

; E-puck related
!insertmacro LANG_STRING STR_Epuck_Install  "Installation du kit pour l'e-puck..."

; Uninstall
!insertmacro LANG_STRING STR_Uninst_Folder  "Suppression du dossier d'installation..."
!insertmacro LANG_STRING STR_Uninst_Menu  "Suppression des raccourcis du menu démarrer et des clés de registre..."
