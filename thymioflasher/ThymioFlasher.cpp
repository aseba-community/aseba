#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QLineEdit>
#include <QSettings>
#include <QGroupBox>
#include <QListWidget>
#include <QApplication>
#include <QtConcurrentRun>
#include <memory>

#include "../common/consts.h"
#include "../utils/HexFile.h"

#include "ThymioFlasher.h"
#include "ThymioFlasher.moc"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	
	QtBootloaderInterface::QtBootloaderInterface(Stream* stream, int dest):
		BootloaderInterface(stream, dest),
		pagesCount(0),
		pagesDoneCount(0)
	{}
	
	void QtBootloaderInterface::writeHexGotDescription(unsigned pagesCount)
	{
		this->pagesCount = pagesCount;
		this->pagesDoneCount = 0;
	}
	
	void QtBootloaderInterface::writePageStart(unsigned pageNumber, const uint8* data, bool simple)
	{
		pagesDoneCount += 1;
		emit flashProgress((100*pagesDoneCount)/pagesCount);
	}
	
	void QtBootloaderInterface::errorWritePageNonFatal(unsigned pageNumber)
	{
		qDebug() << "Warning, error while writing page" << pageNumber << "continuing ...";
	}

	
	ThymioFlasherDialog::ThymioFlasherDialog()
	{
		typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
		const PortsMap ports = SerialPortEnumerator::getPorts();
		
		QSettings settings;
		
		// Create the gui ...
		setWindowTitle(tr("Thymio Firmware Updater"));
		resize(600,500);
		
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		// make sure the port list is enabled only if serial ports are found
		unsigned sectionEnabled(ports.empty() ? 1 : 0);
		
		// serial port
		serialGroupBox = new QGroupBox(tr("Serial connection"));
		serialGroupBox->setCheckable(true);
		QHBoxLayout* serialLayout = new QHBoxLayout();

		serial = new QListWidget();
		bool serialPortSet(false);
		for (PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
		{
			const QString text(it->second.second.c_str());
			QListWidgetItem* item = new QListWidgetItem(text);
			item->setData(Qt::UserRole, QVariant(QString::fromUtf8(it->second.first.c_str())));
			serial->addItem(item);
			if (it->second.second.compare(0,9,"Thymio-II") == 0)
			{
				serial->setCurrentItem(item);
				serialPortSet = true;
			}
		}
		if (sectionEnabled == 0 && !serialPortSet)
			sectionEnabled = 1;
		serialGroupBox->setChecked(sectionEnabled == 0);
		serial->setSelectionMode(QAbstractItemView::SingleSelection);
		serialLayout->addWidget(serial);
		connect(serial, SIGNAL(itemSelectionChanged()), SLOT(setupFlashButtonState()));
		serialGroupBox->setLayout(serialLayout);
		connect(serialGroupBox, SIGNAL(clicked()), SLOT(serialGroupChecked()));
		mainLayout->addWidget(serialGroupBox);
		
		// custom target
		customGroupBox = new QGroupBox(tr("Custom connection"));
		customGroupBox->setCheckable(true);
		customGroupBox->setChecked(sectionEnabled == 1);
		QHBoxLayout* customLayout = new QHBoxLayout();
		QLineEdit* custom = new QLineEdit(settings.value("custom target", ASEBA_DEFAULT_TARGET).toString());
		customLayout->addWidget(custom);
		customGroupBox->setLayout(customLayout);
		connect(customGroupBox, SIGNAL(clicked()), SLOT(customGroupChecked()));
		mainLayout->addWidget(customGroupBox);
		
		// file selector
		QGroupBox* fileGroupBox = new QGroupBox(tr("Firmware file"));
		QHBoxLayout *fileLayout = new QHBoxLayout();
		lineEdit = new QLineEdit(this);
		fileButton = new QPushButton(tr("Select..."), this);
		fileLayout->addWidget(fileButton);
		fileLayout->addWidget(lineEdit);
		fileGroupBox->setLayout(fileLayout);
		mainLayout->addWidget(fileGroupBox);

		// progress bar
		progressBar = new QProgressBar(this);
		progressBar->setValue(0);
		progressBar->setRange(0, 100);
		mainLayout->addWidget(progressBar);

		// flash and quit buttons
		QHBoxLayout *flashLayout = new QHBoxLayout();
		flashButton = new QPushButton(tr("Update"), this);
		flashButton->setEnabled(false);
		flashLayout->addWidget(flashButton);
		quitButton = new QPushButton(tr("Quit"), this);
		flashLayout->addWidget(quitButton);
		mainLayout->addItem(flashLayout);

		// connections
		connect(fileButton, SIGNAL(clicked()), SLOT(openFile()));
		connect(flashButton, SIGNAL(clicked()), SLOT(doFlash()));
		connect(quitButton, SIGNAL(clicked()), SLOT(close()));
		connect(&flashFutureWatcher, SIGNAL(finished()), SLOT(flashFinished()));
		
		show();
	}
	
	ThymioFlasherDialog::~ThymioFlasherDialog()
	{
		flashFuture.waitForFinished();
	}
	
	void ThymioFlasherDialog::serialGroupChecked()
	{
		serialGroupBox->setChecked(true);
		customGroupBox->setChecked(false);
		setupFlashButtonState();
	}
	
	void ThymioFlasherDialog::customGroupChecked()
	{
		serialGroupBox->setChecked(false);
		customGroupBox->setChecked(true);
		setupFlashButtonState();
	}
	
	void ThymioFlasherDialog::setupFlashButtonState()
	{
		flashButton->setEnabled(
			(serial->selectionModel()->hasSelection() || (!serialGroupBox->isChecked())) &&
			!lineEdit->text().isEmpty()
		);
	}
	
	void ThymioFlasherDialog::openFile(void)
	{
		QString name = QFileDialog::getOpenFileName(this, tr("Select hex file"), QString(), tr("Hex files (*.hex)"));
		lineEdit->setText(name);
		setupFlashButtonState();
	}

	void ThymioFlasherDialog::doFlash(void) 
	{
		// warning message
		const int warnRet = QMessageBox::warning(this, tr("Pre-update warning"), tr("Your are about to write a new firmware to the Thymio II. Make sure that the robot is charged and that the USB cable is properly connected.<p><b>Do not unplug the robot during the update!</b></p>Are you sure you want to proceed?"), QMessageBox::No|QMessageBox::Yes, QMessageBox::No);
		if (warnRet != QMessageBox::Yes)
			return;
		
		// get target from GUI
		string target;
		if (serialGroupBox->isChecked())
		{
			const QItemSelectionModel* model(serial->selectionModel());
			Q_ASSERT(model && !model->selectedRows().isEmpty());
			const QModelIndex item(model->selectedRows().first());
			target = QString("ser:device=%0").arg(item.data(Qt::UserRole).toString()).toLocal8Bit().constData();
		}
		else
			target = lineEdit->text().toLocal8Bit().constData();
		
		// disable buttons while flashing
		quitButton->setEnabled(false);
		flashButton->setEnabled(false);
		fileButton->setEnabled(false);
		lineEdit->setEnabled(false);
	
		// start flash thread
		Q_ASSERT(!flashFuture.isRunning());
		flashFuture = QtConcurrent::run(this, &ThymioFlasherDialog::flashThread, target, lineEdit->text().toLocal8Bit().constData());
		flashFutureWatcher.setFuture(flashFuture);
	}
	
	ThymioFlasherDialog::FlashResult ThymioFlasherDialog::flashThread(const std::string& target, const std::string& hexFileName) const
	{
		// open stream
		Dashel::Hub hub;
		Dashel::Stream* stream(0);
		try
		{
			stream = hub.connect(target);
		}
		catch (Dashel::DashelException& e)
		{
			return FlashResult(FlashResult::WARNING, tr("Cannot connect to target"), tr("Cannot connect to target: %1").arg(e.what()));
		}
		
		// do flash
		try
		{
			QtBootloaderInterface bootloaderInterface(stream, 1);
			connect(&bootloaderInterface, SIGNAL(flashProgress(int)), this, SLOT(flashProgress(int)), Qt::QueuedConnection);
			bootloaderInterface.writeHex(hexFileName, true, true);
		}
		catch (HexFile::Error& e)
		{
			return FlashResult(FlashResult::WARNING, tr("Update Error"), tr("Unable to read Hex file: %1").arg(e.toString().c_str()));
		}
		catch (BootloaderInterface::Error& e)
		{
			return FlashResult(FlashResult::FAILURE, tr("Update Error"), tr("A bootloader error happened during the update process: %1").arg(e.what()));
		}
		catch (Dashel::DashelException& e)
		{
			return FlashResult(FlashResult::FAILURE, tr("Update Error"), tr("A communication error happened during the update process: %1").arg(e.what()));
		}
		return FlashResult();
	}
	
	void ThymioFlasherDialog::flashProgress(int percentage)
	{
		progressBar->setValue(percentage);
	}
	
	void ThymioFlasherDialog::flashFinished()
	{
		// re-enable buttons
		quitButton->setEnabled(true);
		flashButton->setEnabled(true);
		fileButton->setEnabled(true);
		lineEdit->setEnabled(true);
		
		// handle flash result
		const FlashResult& result(flashFutureWatcher.result());
		if (result.status == FlashResult::WARNING) 
		{
			QMessageBox::warning(this, result.title, result.text);
		}
		else if (result.status == FlashResult::FAILURE)
		{
			QMessageBox::critical(this, result.title, result.text);
			close();
		}
	}
};


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	
	// TODO: add translations
	Aseba::ThymioFlasherDialog flasher;
	return app.exec();
}