#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QApplication>
#include <QMessageBox>
#include <QRegExpValidator>
#include <QTextCodec>
#include <QTranslator>
#include <QLibraryInfo>

#include <iostream>

#include "../../common/consts.h"

#include "ThymioWNetConfig.h"

class Hex16SpinBox : public QSpinBox
{
public:
	Hex16SpinBox(QWidget *parent = 0) : QSpinBox(parent)
	{
		validator = new QRegExpValidator(QRegExp("[0-9A-Fa-f]{1,4}"), this);
		setPrefix("0x");
		setValue(0);
	}

protected:
	QString textFromValue(int value) const
	{
		return QString::number(value, 16).toUpper();
	}
	
	int valueFromText(const QString &text) const
	{
		return text.toInt(0, 16);
	}
	
	QValidator::State validate(QString &text, int &pos) const
	{
		QString copy(text);
		if (copy.startsWith("0x"))
			copy.remove(0, 2);
		return validator->validate(copy, pos);
	}

private:
	QRegExpValidator *validator;
};

namespace Aseba
{
	using namespace Dashel;
	using namespace std;
	
	typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
	
	#ifdef MSVC
		#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
	#else
		#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
	#endif
	
	PACK(struct Settings
	{
		unsigned short nodeId;
		unsigned short panId;
		unsigned char channel;
		unsigned char txPower;
		unsigned char version;
		unsigned char ctrl;
	});
	
	static Settings settings;

	
	ThymioWNetConfigDialog::ThymioWNetConfigDialog(const std::string& target):
		target(target),
		pairing(false)
	{
		// Create the gui ...
		setWindowTitle(tr("Wireless Thymio Network Configurator"));
		setWindowIcon(QIcon(":/images/thymiownetconfig.svgz"));
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		// image
		QHBoxLayout *imageLayout = new QHBoxLayout();
		QLabel* image = new QLabel(this);
		QPixmap logo(":/images/thymiownetconfig.svgz");
		image->setPixmap(logo.scaledToWidth(384));
		imageLayout->addStretch();
		imageLayout->addWidget(image);
		imageLayout->addStretch();
		mainLayout->addLayout(imageLayout);
		
		// version
		versionLabel = new QLabel();
		mainLayout->addWidget(versionLabel);
		
		// channel
		channel0 = new QRadioButton(tr("0"));
		channel1 = new QRadioButton(tr("1"));
		channel2 = new QRadioButton(tr("2"));
		QHBoxLayout* channelLayout = new QHBoxLayout();
		channelLayout->addWidget(new QLabel(tr("Channel:")));
		channelLayout->addStretch();
		channelLayout->addSpacing(10);
		channelLayout->addWidget(channel0);
		channelLayout->addWidget(channel1);
		channelLayout->addWidget(channel2);
		mainLayout->addLayout(channelLayout);
		
		// network identifier
		networkId = new Hex16SpinBox();
		networkId->setRange(1,65534);
		QHBoxLayout* networkIdLayout = new QHBoxLayout();
		networkIdLayout->addWidget(new QLabel(tr("Network identifier:")));
		networkIdLayout->addStretch();
		networkIdLayout->addSpacing(10);
		networkIdLayout->addWidget(networkId);
		mainLayout->addLayout(networkIdLayout);
		
		// network identifier
		nodeId = new Hex16SpinBox();
		nodeId->setRange(1,65534);
		QHBoxLayout* nodeIdLayout = new QHBoxLayout();
		nodeIdLayout->addWidget(new QLabel(tr("Dongle node identifier:")));
		nodeIdLayout->addStretch();
		nodeIdLayout->addSpacing(10);
		nodeIdLayout->addWidget(nodeId);
		mainLayout->addLayout(nodeIdLayout);

		// buttons
		pairButton = new QPushButton(tr("Enable pairing"));
		flashButton = new QPushButton(tr("Flash into dongle"));
		quitButton = new QPushButton(tr("Quit"));
		mainLayout->addWidget(pairButton);
		mainLayout->addWidget(flashButton);
		mainLayout->addWidget(quitButton);

		// open stream and read initial data
		try
		{
			std::string modifiedTarget = target + ";fc=hard;dtr=false";
			stream = hub.connect(modifiedTarget);
			// we want to read values, not set them
			settings.nodeId = 0xffff;
			settings.panId = 0xffff;
			settings.channel = 0xff;
			settings.txPower = 0;
			settings.version = 0;
			settings.ctrl = 0;
			stream->write(&settings, sizeof(settings));
			stream->read(&settings, sizeof(settings)-1);
			versionLabel->setText(tr("Wireless dongle firmware version %0").arg(settings.version));
			networkId->setValue(settings.panId);
			nodeId->setValue(settings.nodeId);
			if (settings.channel == 15)
				channel0->setChecked(true);
			if (settings.channel == 20)
				channel1->setChecked(true);
			if (settings.channel == 25)
				channel2->setChecked(true);
			
		}
		catch (Dashel::DashelException& e)
		{
			QMessageBox::critical(0, tr("Connection error"), tr("<p><b>Cannot connect to dongle!</b></p><p>Make sure a Wireless Thymio dongle is connected!</p>"));
			return;
		}
		
		// connections
		connect(networkId, SIGNAL(valueChanged(int)), SLOT(updateSettings()));
		connect(nodeId, SIGNAL(valueChanged(int)), SLOT(updateSettings()));
		connect(channel0, SIGNAL(toggled(bool)), SLOT(updateSettings()));
		connect(channel1, SIGNAL(toggled(bool)), SLOT(updateSettings()));
		connect(channel2, SIGNAL(toggled(bool)), SLOT(updateSettings()));
		
 		connect(pairButton, SIGNAL(clicked()), SLOT(togglePairingMode()));
 		connect(flashButton, SIGNAL(clicked()), SLOT(flashSettings()));
 		connect(quitButton, SIGNAL(clicked()), SLOT(quit()));
		
		show();
	}
	
	ThymioWNetConfigDialog::~ThymioWNetConfigDialog()
	{
	}
	
	void ThymioWNetConfigDialog::updateSettings()
	{
		settings.panId = networkId->value();
		settings.nodeId = nodeId->value();
		if (channel0->isChecked())
			settings.channel = 15;
		if (channel1->isChecked())
			settings.channel = 20;
		if (channel2->isChecked())
			settings.channel = 25;
		settings.ctrl = 0;
		stream->write(&settings, sizeof(settings));
		stream->read(&settings, sizeof(settings)-1);
	}
	
	void ThymioWNetConfigDialog::flashSettings()
	{
		settings.ctrl = 0x1;
		stream->write(&settings, sizeof(settings));
		stream->read(&settings, sizeof(settings)-1);
	}
	
	void ThymioWNetConfigDialog::togglePairingMode()
	{
		settings.ctrl = 0x4;
		stream->write(&settings, sizeof(settings));
		stream->read(&settings, sizeof(settings)-1);
		
		if (!pairing)
		{
			pairButton->setText(tr("Disable pairing"));
		}
		else
		{
			pairButton->setText(tr("Enable pairing"));
		}
		pairing = !pairing;
	}
	
	void ThymioWNetConfigDialog::quit()
	{
		if (pairing)
		{
			settings.ctrl = 0x4;
			stream->write(&settings, sizeof(settings));
			stream->read(&settings, sizeof(settings)-1);
		}
		close();
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
	translator.load(QString(":/thymiownetconfig_") + QLocale::system().name());
	
	const Aseba::PortsMap ports = Dashel::SerialPortEnumerator::getPorts();
	std::string target("ser:device=");
	bool thymioFound(false);
	bool thymiosFound(false);
	for (Aseba::PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
	{
		if ((it->second.second.compare(0, 18, "Thymio-II Wireless") == 0) ||
			(it->second.second.compare(0, 18, "Thymio_II Wireless") == 0))
		{
			if (thymioFound)
				thymiosFound = true;
			thymioFound = true;
			target += it->second.first;
		}
	}
	if (!thymioFound)
	{
		QMessageBox::critical(0, QApplication::tr("Wireless Thymio dongle not found"), QApplication::tr("<p><b>Cannot find a Wireless Thymio dongle!</b></p><p>Plug a dongle into one of your USB ports and try again.</p>"));
		return 1;
	}
	if (thymiosFound)
	{
		QMessageBox::critical(0, QApplication::tr("Multiple Wireless Thymio dongles found"), QApplication::tr("<p><b>More than one Wireless Thymio dongles found!</b></p><p>Plug a single dongle into your computer and try again.</p>"));
		return 2;
	}
	
	int retCode(1);
	{
		Aseba::ThymioWNetConfigDialog configurator(target);
		if (configurator.isVisible())
			retCode = app.exec();
	}
	
	return retCode;
}
