#ifndef THYMIO_BOOTLOADER_H
#define THYMIO_BOOTLOADER_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QLineEdit>
#include <dashel/dashel.h>

#include <map>
#include <vector>
#include <iterator>

#include <dashel/dashel.h>

#include "MainWindow.h"
#include "DashelTarget.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioBootloaderDialog : public QDialog, public InvasivePlugin
	{
		Q_OBJECT

	private:
		QVBoxLayout	*verticalLayout;
		QHBoxLayout 	*fileLayout;
		QHBoxLayout 	*flashLayout;
		QSpacerItem 	*spacer;
		QLineEdit 	*lineEdit;
		QPushButton 	*fileButton;
		QPushButton 	*quitButton;
		QPushButton 	*flashButton;
		QProgressBar 	*progressBar;
		
		typedef std::map<uint32, std::vector<uint8> > PageMap;
		PageMap 	pageMap;
		PageMap::iterator currentPage;

		unsigned 	id;
		Dashel::Stream 	*stream;

		void writePage(unsigned page, unsigned char * data);
		void flashDone(void);

	public:
		ThymioBootloaderDialog(MainWindow * mainWindow);
		~ThymioBootloaderDialog();
		
		void Flash(class NodeTab * t);
		
	private slots:
		void Ack(unsigned error_code, unsigned address);
		void openFile(void);
		void doFlash(void);
		void doClose(void);
		void vmDisconnect(unsigned);

	protected:	
		void closeEvent(QCloseEvent * event);
	};
	/*@}*/
}; // Aseba

#endif
