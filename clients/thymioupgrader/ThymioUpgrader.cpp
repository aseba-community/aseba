#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QListWidget>
#include <QApplication>
#include <QTextCodec>
#include <QTranslator>
#include <QLibraryInfo>
#include <QtConcurrentRun>
#include <memory>
#include <iostream>

#include "../../common/consts.h"
#include "../../common/utils/HexFile.h"

#include "ThymioUpgrader.h"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
	
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

	
	ThymioUpgraderDialog::ThymioUpgraderDialog(const std::string& target):
		target(target)
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Firmware Upgrader"));
        setWindowIcon(QIcon(":/images/thymioupgrader.svgz"));
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		QHBoxLayout *imageLayout = new QHBoxLayout();
		QLabel* image = new QLabel(this);
		image->setPixmap(QPixmap(":/images/firmwareupgrade.png"));
		imageLayout->addStretch();
		imageLayout->addWidget(image);
		imageLayout->addStretch();
		mainLayout->addLayout(imageLayout);
		
		// file selector
		QGroupBox* fileGroupBox = new QGroupBox(tr("Firmware file"));
		QHBoxLayout *fileLayout = new QHBoxLayout();
		lineEdit = new QLineEdit(this);
		fileButton = new QPushButton(tr("Select..."), this);
		fileButton->setIcon(QIcon(":/images/fileopen.svgz"));
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
		flashButton = new QPushButton(tr("Upgrade"), this);
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
	
	ThymioUpgraderDialog::~ThymioUpgraderDialog()
	{
		flashFuture.waitForFinished();
	}
	
	void ThymioUpgraderDialog::setupFlashButtonState()
	{
		flashButton->setEnabled(
			!lineEdit->text().isEmpty()
		);
	}
	
	void ThymioUpgraderDialog::openFile(void)
	{
		QString name = QFileDialog::getOpenFileName(this, tr("Select hex file"), QString(), tr("Hex files (*.hex)"));
		lineEdit->setText(name);
		setupFlashButtonState();
	}

	void ThymioUpgraderDialog::doFlash(void) 
	{
		// warning message
		const int warnRet = QMessageBox::warning(this, tr("Pre-upgrade warning"), tr("Your are about to write a new firmware to the Thymio II. Make sure that the robot is charged and that the USB cable is properly connected.<p><b>Do not unplug the robot during the upgrade!</b></p>Are you sure you want to proceed?"), QMessageBox::No|QMessageBox::Yes, QMessageBox::No);
		if (warnRet != QMessageBox::Yes)
			return;
		
		// disable buttons while flashing
		quitButton->setEnabled(false);
		flashButton->setEnabled(false);
		fileButton->setEnabled(false);
		lineEdit->setEnabled(false);
	
		// start flash thread
		Q_ASSERT(!flashFuture.isRunning());
		const string hexFileName(lineEdit->text().toLocal8Bit().constData());
		flashFuture = QtConcurrent::run(this, &ThymioUpgraderDialog::flashThread, target, hexFileName);
		flashFutureWatcher.setFuture(flashFuture);
	}
	
	ThymioUpgraderDialog::FlashResult ThymioUpgraderDialog::flashThread(const std::string& _target, const std::string& hexFileName) const
	{
		// open stream
		Dashel::Hub hub;
		Dashel::Stream* stream(0);
		try
		{
			stream = hub.connect(_target);
		}
		catch (Dashel::DashelException& e)
		{
			return FlashResult(FlashResult::WARNING, tr("Cannot connect to Thymio II"), tr("Cannot connect to Thymio II: %1.<p>Most probably another program is currently connected to the Thymio II. Make sure that there are no Studio or other Upgrader running and try again.</p>").arg(e.what()));
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
			return FlashResult(FlashResult::WARNING, tr("Upgrade Error"), tr("Unable to read Hex file: %1").arg(e.toString().c_str()));
		}
		catch (BootloaderInterface::Error& e)
		{
			return FlashResult(FlashResult::FAILURE, tr("Upgrade Error"), tr("A bootloader error happened during the upgrade process: %1").arg(e.what()));
		}
		catch (Dashel::DashelException& e)
		{
			return FlashResult(FlashResult::FAILURE, tr("Upgrade Error"), tr("A communication error happened during the upgrade process: %1").arg(e.what()));
		}
		return FlashResult();
	}
	
	void ThymioUpgraderDialog::flashProgress(int percentage)
	{
		progressBar->setValue(percentage);
	}
	
	void ThymioUpgraderDialog::flashFinished()
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
	
	QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
	
	QTranslator qtTranslator;
	QTranslator translator;
	app.installTranslator(&qtTranslator);
	app.installTranslator(&translator);
	qtTranslator.load(QString("qt_") + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
	translator.load(QString(":/thymioupgrader_") + QLocale::system().name());
	
	const Aseba::PortsMap ports = Dashel::SerialPortEnumerator::getPorts();
	std::string target("ser:device=");
	bool thymioFound(false);
	bool thymiosFound(false);
	for (Aseba::PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		if (it->second.second.compare(0,9,"Thymio-II") == 0)
		{
			if (thymioFound)
				thymiosFound = true;
			thymioFound = true;
			target += it->second.first;
			//std::cout << target << std::endl;
		}
	}
	if (!thymioFound)
	{
		QMessageBox::critical(0, QApplication::tr("Thymio II not found"), QApplication::tr("<p><b>Cannot find Thymio II!</b></p><p>Plug Thymio II or use the command-line firmware upgrader (asebacmd).</p>"));
		return 1;
	}
	if (thymiosFound)
	{
		QMessageBox::critical(0, QApplication::tr("Multiple Thymio II found"), QApplication::tr("<p><b>More than one Thymio II found!</b></p><p>Plug a single Thymio II or use the command-line firmware upgrader (asebacmd).</p>"));
		return 2;
	}
	
	Aseba::ThymioUpgraderDialog upgrader(target);
	
	return app.exec();
}
