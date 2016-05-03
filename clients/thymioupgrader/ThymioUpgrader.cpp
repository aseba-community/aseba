#include <QQueue>
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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomDocument>
#include <memory>
#include <iostream>

#include "../../common/consts.h"
#include "../../common/msg/msg.h"
#include "../../common/utils/HexFile.h"
#include "../../common/utils/utils.h"

#include "ThymioUpgrader.h"

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	//! Hub that receives messages and deletes them
	class MessageHub: public Hub
	{
	public:
		QQueue<Message*> messages;
		
	public:
		MessageHub()
		{}
		
		virtual ~MessageHub()
		{
			while (!messages.empty())
				delete messages.dequeue();
		}
		
		virtual void incomingData(Stream * stream)
		{
			messages.enqueue(Message::receive(stream));
		}
		
		bool isMessage() const
		{
			return !messages.empty();
		}
		
		Message* getMessage() const
		{
			return messages.head();
		}
		
		void clearMessage()
		{
			if (!messages.empty())
				delete messages.dequeue();
		}
	};
	
	typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
	
	QtBootloaderInterface::QtBootloaderInterface(Stream* stream, int dest, int bootloaderDest):
		BootloaderInterface(stream, dest, bootloaderDest),
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
		target(target),
		currentVersion(0),
		currentDevStatus(-1),
		officialVersion(0),
		officialDevStatus(-1)
	{
		// Create the gui ...
		setWindowTitle(tr("Thymio Firmware Upgrader"));
		setWindowIcon(QIcon(":/images/thymioupgrader.svgz"));
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		// image
		QHBoxLayout *imageLayout = new QHBoxLayout();
		QLabel* image = new QLabel(this);
		QPixmap logo(":/images/thymioupgrader.svgz");
		image->setPixmap(logo.scaledToWidth(384));
		imageLayout->addStretch();
		imageLayout->addWidget(image);
		imageLayout->addStretch();
		mainLayout->addLayout(imageLayout);
		
		// current firmware and node identifier
		firmwareVersion = new QLabel("unknown firmware version", this);
		mainLayout->addWidget(firmwareVersion);
		nodeIdText = new QLabel(this);
		mainLayout->addWidget(nodeIdText);
		
		// latest file
		officialGroupBox = new QGroupBox(tr("Latest official firmware"));
		QVBoxLayout *officialLayout = new QVBoxLayout();
		officialFirmwareText = new QLabel("Connection to official firmware server in progress...");
		officialFirmwareText->setWordWrap(true);
		officialLayout->addWidget(officialFirmwareText);
		officialGroupBox->setLayout(officialLayout);
		connect(officialGroupBox, SIGNAL(clicked(bool)), SLOT(officialGroupChecked(bool)));
		mainLayout->addWidget(officialGroupBox);
		
		// file selector
		fileGroupBox = new QGroupBox(tr("Custom firmware file"));
		QHBoxLayout *fileLayout = new QHBoxLayout();
		lineEdit = new QLineEdit(this);
		fileButton = new QPushButton(tr("Select..."), this);
		fileButton->setIcon(QIcon(":/images/fileopen.svgz"));
		fileLayout->addWidget(fileButton);
		fileLayout->addWidget(lineEdit);
		fileGroupBox->setLayout(fileLayout);
		connect(fileGroupBox, SIGNAL(clicked(bool)), SLOT(fileGroupChecked(bool)));
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
		
		// read id and version
		readIdVersion();
		
		// create network stuff to get latest firmware
		networkManager = new QNetworkAccessManager(this);
		connect(networkManager, SIGNAL(finished(QNetworkReply*)), SLOT(networkReplyFinished(QNetworkReply*)));
		networkManager->get(QNetworkRequest(QUrl("http://firmwarefiles.thymio.org/Thymio2-firmware-meta.xml")));
		
		show();
	}
	
	ThymioUpgraderDialog::~ThymioUpgraderDialog()
	{
		flashFuture.waitForFinished();
	}
	
	void ThymioUpgraderDialog::officialGroupChecked(bool on)
	{
		if (on)
			selectOfficialFirmware();
		else
			selectUserFirmware();
	}
	
	void ThymioUpgraderDialog::fileGroupChecked(bool on)
	{
		if (on)
			selectUserFirmware();
		else
			selectOfficialFirmware();
	}
	
	void ThymioUpgraderDialog::selectOfficialFirmware()
	{
		officialGroupBox->setChecked(true);
		fileGroupBox->setChecked(false);
		setupFlashButtonState();
	}
	
	void ThymioUpgraderDialog::selectUserFirmware()
	{
		officialGroupBox->setChecked(false);
		fileGroupBox->setChecked(true);
		setupFlashButtonState();
	}
	
	void ThymioUpgraderDialog::setupFlashButtonState()
	{
		if (flashFuture.isRunning())
		{
			flashButton->setEnabled(false);
		}
		else
		{
			if (officialGroupBox->isChecked())
				flashButton->setEnabled(true);
			else
				flashButton->setEnabled(!lineEdit->text().isEmpty());
		}
	}
	
	void ThymioUpgraderDialog::openFile(void)
	{
		QString name = QFileDialog::getOpenFileName(this, tr("Select hex file"), QString(), tr("Hex files (*.hex)"));
		lineEdit->setText(name);
		setupFlashButtonState();
	}
	
	unsigned ThymioUpgraderDialog::readId(MessageHub& hub, Dashel::Stream* stream) const
	{
		// default id
		unsigned nodeId = 1;
		
		// list nodes to get ids
		ListNodes().serialize(stream);
		GetDescription().serialize(stream);;
		stream->flush();

		// process data for up to two seconds
		QTime listNodeTime(QTime::currentTime());
		int restDuration(2000);
		while (restDuration > 0)
		{
			hub.step(restDuration);
			while (hub.isMessage())
			{
				// if node present, store id from source and quit (protocol 5)
				NodePresent* nodePresent(dynamic_cast<NodePresent*>(hub.getMessage()));
				if (nodePresent)
				{
					nodeId = nodePresent->source;
					hub.clearMessage();
					break;
				}
				// if description, store id from source, but continue reading (protocol 4)
				Description* description(dynamic_cast<Description*>(hub.getMessage()));
				if (description)
				{
					nodeId = description->source;
				}
				hub.clearMessage();
			}
			restDuration = 2000 - listNodeTime.msecsTo(QTime::currentTime());
		}
		return nodeId;
	}
	
	void ThymioUpgraderDialog::readIdVersion(void)
	{
		// open stream
		MessageHub hub;
		Dashel::Stream* stream(0);
		try
		{
			stream = hub.connect(target);
		}
		catch (Dashel::DashelException& e)
		{
			QMessageBox::warning(this, tr("Cannot connect to Thymio II"), tr("Cannot connect to Thymio II: %1.<p>Most probably another program is currently connected to the Thymio II. Make sure that there are no Studio or other Upgrader running and try again.</p>").arg(e.what()));
			return;
		}
		
		// read node id
		const unsigned nodeId(readId(hub, stream));
		nodeIdText->setText(tr("Thymio node identifier: %1").arg(nodeId));
		
		// read firmware version
		const unsigned firmwareAddress(34);
		GetVariables(nodeId, firmwareAddress, 2).serialize(stream);
		stream->flush();
		
		// process data for up to one second
		QTime getVariablesTime(QTime::currentTime());
		int restDuration(1000);
		while (restDuration > 0)
		{
			hub.step(restDuration);
			while (hub.isMessage())
			{
				Variables* variables(dynamic_cast<Variables*>(hub.getMessage()));
				if (variables && variables->start == firmwareAddress && variables->variables.size() == 2)
				{
					const unsigned currentVersion = variables->variables[0];
					const unsigned currentDevStatus = variables->variables[1];
					firmwareVersion->setText(tr("Current firmware: %1").arg(versionDevStatusToString(currentVersion, currentDevStatus)));
					hub.clearMessage();
					break;
				}
				hub.clearMessage();
			}
			restDuration = 1000 - getVariablesTime.msecsTo(QTime::currentTime());
		}
	}
	
	QString ThymioUpgraderDialog::versionDevStatusToString(unsigned version, unsigned devStatus) const
	{
		if (devStatus == 0)
			return tr("version %1 - production").arg(version);
		else
			return tr("version %1 - development %2").arg(version).arg(devStatus);
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
		officialGroupBox->setEnabled(false);
		fileGroupBox->setEnabled(false);
		
		// start flash thread
		Q_ASSERT(!flashFuture.isRunning());
		string hexFileName;
		if (officialGroupBox->isChecked())
			hexFileName = officialHexFile.fileName().toStdString();
		else
			hexFileName = lineEdit->text().toLocal8Bit().constData();
		flashFuture = QtConcurrent::run(this, &ThymioUpgraderDialog::flashThread, target, hexFileName);
		flashFutureWatcher.setFuture(flashFuture);
	}
	
	ThymioUpgraderDialog::FlashResult ThymioUpgraderDialog::flashThread(const std::string& _target, const std::string& hexFileName) const
	{
		// open stream
		MessageHub hub;
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
			// read again node id, in case a different robot was plugged in
			const unsigned nodeId(readId(hub, stream));
			
			// then flash
			QtBootloaderInterface bootloaderInterface(stream, nodeId, 1);
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
		
		// sleep 100 ms
		UnifiedTime(100).sleep();
		
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
		officialGroupBox->setEnabled(true);
		fileGroupBox->setEnabled(true);
		setupFlashButtonState();
		
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
		else
		{
			// re-read version
			readIdVersion();
		}
	}
	
	void ThymioUpgraderDialog::networkReplyFinished(QNetworkReply* reply)
	{
		QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
		if (reply->error())
		{
			networkError();
		}
		else if (!redirectionTarget.isNull())
		{
			networkManager->get(QNetworkRequest(redirectionTarget.toUrl()));
		}
		else
		{	
			if (officialDevStatus == unsigned(-1))
			{
				QDomDocument document;
				document.setContent(reply->readAll(), true);
				QDomElement firmwareMeta(document.firstChildElement("firmware"));
				if (!firmwareMeta.isNull())
				{
					officialVersion = firmwareMeta.attribute("version").toUInt();
					officialDevStatus = firmwareMeta.attribute("devStatus").toUInt();
					networkManager->get(QNetworkRequest(QUrl(firmwareMeta.attribute("url"))));
					
				}
				else
				{
					networkError();
				}
			}
			else
			{
				if (officialHexFile.open())
				{
					officialHexFile.write(reply->readAll());
					officialHexFile.waitForBytesWritten(-1);
					officialFirmwareText->setText(tr("Official firmware: %1").arg(versionDevStatusToString(officialVersion, officialDevStatus)));
					officialGroupBox->setCheckable(true);
					fileGroupBox->setCheckable(true);
					selectOfficialFirmware();
				}
				else
					officialFirmwareText->setText(tr("Cannot open temporary file!"));
			}
		}
		reply->deleteLater();
	}
	
	void ThymioUpgraderDialog::networkError()
	{
		officialFirmwareText->setText(tr("Error connecting to official firmware server!"));
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
	bool wirelessThymioFound(false);
	bool thymioFound(false);
	bool thymiosFound(false);
	for (Aseba::PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		if ((it->second.second.compare(0, 18, "Thymio-II Wireless") == 0) ||
			(it->second.second.compare(0, 18, "Thymio_II Wireless") == 0))
		{
			wirelessThymioFound = true;
		}
		else if ((it->second.second.compare(0, 9, "Thymio-II") == 0) ||
				 (it->second.second.compare(0, 9, "Thymio_II") == 0))
		{
			if (thymioFound)
				thymiosFound = true;
			thymioFound = true;
			target += it->second.first;
			//std::cout << target << std::endl;
		}
	}
	if (wirelessThymioFound)
	{
		QMessageBox::critical(0, QApplication::tr("Wireless Thymio found"), QApplication::tr("<p><b>Wireless connection to Thymio found!</b></p><p>Plug a single Thymio to your computer using the USB cable.</p>"));
		return 3;
	}
	if (!thymioFound)
	{
		QMessageBox::critical(0, QApplication::tr("Thymio not found"), QApplication::tr("<p><b>Cannot find Thymio!</b></p><p>Plug a Thymio to your computer using the USB cable.</p>"));
		return 1;
	}
	if (thymiosFound)
	{
		QMessageBox::critical(0, QApplication::tr("Multiple Thymios found"), QApplication::tr("<p><b>More than one Thymio found!</b></p><p>Plug a single Thymio to your computer using the USB cable.</p>"));
		return 2;
	}
	
	Aseba::ThymioUpgraderDialog upgrader(target);
	
	return app.exec();
}
