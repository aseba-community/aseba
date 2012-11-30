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

#include "../common/consts.h"

#include "ThymioFlasher.h"
#include "ThymioFlasher.moc"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
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
		// TODO: wait for future
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
		
		// open 
		
		/*HexFile hex;
		try {
			hex.read(lineEdit->text().toStdString());
		}
		catch(HexFile::Error& e) {
			QMessageBox::critical(this, tr("Update Error"), tr("Unable to read Hex file"));
			return;
		}
		quitButton->setEnabled(false);
		flashButton->setEnabled(false);
		fileButton->setEnabled(false);
		lineEdit->setEnabled(false);
		
		// make target ready for flashing
		target->blockWrite();
		connect(target, SIGNAL(bootloaderAck(uint,uint)), this, SLOT(ackReceived(uint,uint)));
		connect(target, SIGNAL(nodeDisconnected(uint)), this, SLOT(vmDisconnected(unsigned)));

		// Now we have a valid hex file ...
		pageMap.clear();

		for (HexFile::ChunkMap::iterator it = hex.data.begin(); it != hex.data.end(); it ++)
		{
			// get page number
			unsigned chunkAddress = it->first;
			// index inside data chunk
			unsigned chunkDataIndex = 0;
			// size of chunk in bytes
			unsigned chunkSize = it->second.size();
			// copy data from chunk to page
			do
			{
				// get page number
				unsigned pageIndex = (chunkAddress + chunkDataIndex) / 2048;
				// get address inside page
				unsigned byteIndex = (chunkAddress + chunkDataIndex) % 2048;
				// if page does not exists, create it
				if (pageMap.find(pageIndex) == pageMap.end())
					pageMap[pageIndex] = vector<uint8>(2048, (uint8)0);
				// copy data
				unsigned amountToCopy = min(2048 - byteIndex, chunkSize - chunkDataIndex);
				copy(it->second.begin() + chunkDataIndex, it->second.begin() + chunkDataIndex + amountToCopy, pageMap[pageIndex].begin() + byteIndex);
				// increment chunk data pointer
				chunkDataIndex += amountToCopy;
			}
			while(chunkDataIndex < chunkSize);
		}

		progressBar->setMaximum(pageMap.size());
		progressBar->setValue(0);
		
		// Ask the pic to switch into bootloader
		Reboot msg(nodeId);
		try
		{
			msg.serialize(stream);
			stream->flush();
			// now, wait until the disconnect event is sent.
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}*/
	}
	
	/*
	void ThymioFlasherDialog::handleDashelException(Dashel::DashelException e)
	{
		switch(e.source)
		{
		default:
			// oops... we are doomed
			// non-modal message box
			QMessageBox* message = new QMessageBox(QMessageBox::Critical, tr("Dashel Unexpected Error"), tr("A communication error happened during the update process:") + " (" + QString::number(e.source) + ") " + e.what(), QMessageBox::NoButton, this);
			message->setWindowModality(Qt::NonModal);
			message->show();
		}
	}*/
};


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	Aseba::ThymioFlasherDialog flasher;
	return app.exec();
}