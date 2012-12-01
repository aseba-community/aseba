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
#include <memory>

#include "../common/consts.h"
#include "../utils/HexFile.h"

#include "ThymioFlasher.h"
#include "ThymioFlasher.moc"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	
	QtBootloaderInterface::QtBootloaderInterface(Stream* stream, int dest, QProgressBar* progressBar):
		BootloaderInterface(stream, dest),
		progressBar(progressBar)
	{}
	
	void QtBootloaderInterface::writeHexGotDescription(unsigned pagesCount)
	{
		progressBar->setMaximum(pagesCount);
		progressBar->setValue(0);
	}
	
	void QtBootloaderInterface::writePageStart(int pageNumber, const uint8* data, bool simple)
	{
		progressBar->setValue(progressBar->value()+1);
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
		QPushButton *fileButton = new QPushButton(tr("Select..."), this);
		fileLayout->addWidget(fileButton);
		fileLayout->addWidget(lineEdit);
		fileGroupBox->setLayout(fileLayout);
		mainLayout->addWidget(fileGroupBox);

		// progress bar
		progressBar = new QProgressBar(this);
		progressBar->setValue(0);
		mainLayout->addWidget(progressBar);

		// flash and quit buttons
		QHBoxLayout *flashLayout = new QHBoxLayout();
		flashButton = new QPushButton(tr("Update"), this);
		flashButton->setEnabled(false);
		flashLayout->addWidget(flashButton);
		QPushButton *quitButton = new QPushButton(tr("Quit"), this);
		flashLayout->addWidget(quitButton);
		mainLayout->addItem(flashLayout);

		// connections
		connect(fileButton, SIGNAL(clicked()),this,SLOT(openFile()));
		connect(flashButton, SIGNAL(clicked()), this, SLOT(doFlash()));
		connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
		
		show();
	}
	
	ThymioFlasherDialog::~ThymioFlasherDialog()
	{
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
		// TODO: load hex file
	}

	void ThymioFlasherDialog::doFlash(void) 
	{
		// warning message
		const int warnRet = QMessageBox::warning(this, tr("Pre-update warning"), tr("Your are about to write a new firmware to the Thymio II. Make sure that the robot is charged and that the USB cable is properly connected.<p><b>Do not unplug the robot during the update!</b></p>Are you sure you want to proceed?"), QMessageBox::No|QMessageBox::Yes, QMessageBox::No);
		if (warnRet != QMessageBox::Yes)
			return;
		
		// open stream
		Dashel::Hub hub;
		Dashel::Stream* stream(0);
		string target;
		try
		{
			if (serialGroupBox->isChecked())
			{
				const QItemSelectionModel* model(serial->selectionModel());
				Q_ASSERT(model && !model->selectedRows().isEmpty());
				const QModelIndex item(model->selectedRows().first());
				target = QString("ser:device=%0").arg(item.data(Qt::UserRole).toString()).toStdString();
			}
			else
				target = lineEdit->text().toStdString();
			stream = hub.connect(target);
		}
		catch (Dashel::DashelException& e)
		{
			QMessageBox::warning(this, tr("Cannot connect to target"), tr("Cannot connect to target: %1").arg(e.what()));
			return;
		}
		
		// do flash
		try
		{
			QtBootloaderInterface bootloaderInterface(stream, 1, progressBar);
			bootloaderInterface.writeHex(lineEdit->text().toStdString(), true, true);
			// TODO: fix potential bug with UTF8 file names
		}
		catch (HexFile::Error& e)
		{
			QMessageBox::warning(this, tr("Update Error"), tr("Unable to read Hex file, update aborted"));
		}
		catch (BootloaderInterface::Error& e)
		{
			QMessageBox::critical(this, tr("Update Error"), tr("A bootloader error happened during the update process: %1").arg(e.what()));
			close();
		}
		catch (Dashel::DashelException& e)
		{
			QMessageBox::critical(this, tr("Update Error"), tr("A communication error happened during the update process: %1").arg(e.what()));
			close();
		}
	}
};


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	Aseba::ThymioFlasherDialog flasher;
	return app.exec();
}