#ifndef THYMIO_FLASHER_H
#define THYMIO_FLASHER_H

#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <dashel/dashel.h>
#include "../utils/BootloaderInterface.h"

class QVBoxLayout;
class QHBoxLayout;
class QProgressBar;
class QPushButton;
class QLineEdit;
class QListWidget;
class QGroupBox;

namespace Aseba
{
	/** \addtogroup thymioflasher */
	/*@{*/
	
	class QtBootloaderInterface:public QObject, public BootloaderInterface
	{
		Q_OBJECT
		
	public:
		QtBootloaderInterface(Dashel::Stream* stream, int dest);
		
	protected:
		virtual void writeHexGotDescription(unsigned pagesCount);
		virtual void writePageStart(unsigned pageNumber, const uint8* data, bool simple);
		virtual void errorWritePageNonFatal(unsigned pageNumber);
	
	protected:
		unsigned pagesCount;
		unsigned pagesDoneCount;
		
	signals:
		void flashProgress(int percentage);
	};
	
	class ThymioFlasherDialog : public QWidget
	{
		Q_OBJECT
		
	private:
		struct FlashResult
		{
			enum Status
			{
				SUCCESS = 0,
				WARNING,
				FAILURE
			} status;
			QString title;
			QString text;
			
			FlashResult():status(SUCCESS) {}
			FlashResult(Status status, const QString& title, const QString& text):status(status), title(title), text(text) {}
		};
		
	private:
		QVBoxLayout* mainLayout;
		QListWidget* serial;
		QGroupBox* serialGroupBox;
		QGroupBox* customGroupBox;
		QHBoxLayout* fileLayout;
		QHBoxLayout* flashLayout;
		QLineEdit* lineEdit;
		QPushButton* fileButton;
		QProgressBar* progressBar;
		QPushButton* flashButton;
		QPushButton* quitButton;
		QFuture<FlashResult> flashFuture;
		QFutureWatcher<FlashResult> flashFutureWatcher;

	public:
		ThymioFlasherDialog();
		~ThymioFlasherDialog();
		
	private:
		FlashResult flashThread(const std::string target, const std::string hexFileName) const;
	
	private slots:
		void serialGroupChecked();
		void customGroupChecked();
		void setupFlashButtonState();
		void openFile(void);
		void doFlash(void);
		void flashProgress(int percentage);
		void flashFinished();
	};
	
	/*@}*/
}; // Aseba

#endif // THYMIO_FLASHER_H

