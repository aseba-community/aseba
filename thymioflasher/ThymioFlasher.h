#ifndef THYMIO_FLASHER_H
#define THYMIO_FLASHER_H

#include <QWidget>

#include <dashel/dashel.h>


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
	
	class ThymioFlasherDialog : public QWidget
	{
		Q_OBJECT

	private:
		QVBoxLayout* mainLayout;
		QListWidget* serial;
		QGroupBox* serialGroupBox;
		QGroupBox* customGroupBox;
		QHBoxLayout* fileLayout;
		QHBoxLayout* flashLayout;
		QLineEdit* lineEdit;
		QProgressBar* progressBar;
		QPushButton *flashButton;

	public:
		ThymioFlasherDialog();
		~ThymioFlasherDialog();
		
	private slots:
		void serialGroupChecked();
		void customGroupChecked();
		void setupFlashButtonState();
		void openFile(void);
		void doFlash(void);
	};
	/*@}*/
}; // Aseba

#endif // THYMIO_FLASHER_H

