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

#include "../Plugin.h"
#include "../DashelTarget.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class ThymioBootloaderDialog : public QDialog, public InvasivePlugin, public NodeToolInterface
	{
		Q_OBJECT

	private:
		QVBoxLayout	*verticalLayout;
		QHBoxLayout *fileLayout;
		QHBoxLayout *flashLayout;
		QSpacerItem *spacer;
		QLineEdit 	*lineEdit;
		QPushButton *fileButton;
		QPushButton *quitButton;
		QPushButton *flashButton;
		QProgressBar *progressBar;
		
		typedef std::map<uint32, std::vector<uint8> > PageMap;
		PageMap pageMap;
		PageMap::iterator currentPage;

		// we must cache this because during the flashing process, the tab is not there any more
		unsigned nodeId;
		Target * target;
		Dashel::Stream 	*stream;

		int delete_myself;

	public:
		ThymioBootloaderDialog(NodeTab* nodeTab);
		~ThymioBootloaderDialog();
		
		virtual QWidget* createMenuEntry();
		virtual void closeAsSoonAsPossible();
		virtual bool surviveTabDestruction() const;
		
	private slots:
		void showFlashDialog();
		
		void openFile(void);
		void doFlash(void);
		void doClose(void);
		
		void ackReceived(unsigned error_code, unsigned address);
		void vmDisconnected(unsigned);

	private:
		void writePage(unsigned page, unsigned char * data);
		void flashDone(void);
		void timerEvent(QTimerEvent *event);
		void closeEvent(QCloseEvent * event);
	};
	/*@}*/
}; // Aseba

#endif
