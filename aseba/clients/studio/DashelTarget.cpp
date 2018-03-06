/*
	Aseba - an event-based framework for distributed robot control
	Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
	with contributions from the community.
	Copyright (C) 2007--2018 the authors, see authors.txt for details.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published
	by the Free Software Foundation, version 3 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "DashelTarget.h"
#include "common/msg/msg.h"
#include "common/utils/utils.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cassert>
#include <QInputDialog>
#include <QtWidgets>
#include <QLibraryInfo>
#include <QHostInfo>
#include <QNetworkInterface>
#include <stdexcept>
#include <regex>

#ifdef WIN32 // for Sleep
#include <windows.h>
#endif

namespace Aseba
{
	using std::copy;
	using namespace Dashel;

	/** \addtogroup studio */
	/*@{*/

	void handleDashelException(Dashel::DashelException e)
	{
		switch(e.source)
		{
		case Dashel::DashelException::ConnectionLost:
		case Dashel::DashelException::IOError:
			// "normal" disconnections, managed internally in Dashel::Hub, so don't care about
			break;
		case Dashel::DashelException::ConnectionFailed:
			// should not happen here, but can because of typos in Dashel, catch it for now
			break;
		default:
			QMessageBox::critical(nullptr, QObject::tr("Unexpected Dashel Error"), QObject::tr("A communication error happened:") + " (" + QString::number(e.source) + ") " + e.what());
			break;
		}
	}

	bool hasIntersection(const QList<QHostAddress> & addressesA, const QList<QHostAddress> & addressesB)
	{
		for (int i = 0; i < addressesA.size(); ++i)
			for (int j = 0; j < addressesB.size(); ++j)
				if (addressesA[i] == addressesB[j])
					return true;
		return false;
	}

	DashelConnectionDialog::DashelConnectionDialog()
	{
		QSettings settings;

		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		// discovered targets
		discoveredList = new QListWidget();
		discoveredList->setSelectionMode(QAbstractItemView::SingleSelection);
		QPalette p = discoveredList->palette();
		p.setColor(QPalette::Highlight, QColor("blue").lighter(175));
		discoveredList->setPalette(p);
		mainLayout->addWidget(new QLabel(tr("Discovered targets")));
		mainLayout->addWidget(discoveredList);

		// selected target
		mainLayout->addWidget(new QLabel(tr("Selected target")));
		QHBoxLayout* targetLayout = new QHBoxLayout();
		currentTarget = new QLineEdit(settings.value("current target", ASEBA_DEFAULT_TARGET).toString());
		targetLayout->addWidget(currentTarget);
		auto templateButton = new QPushButton(QIcon(":/images/info.png"), "");
		templateButton->setMaximumSize(80, 50);
		templateButton->setFocusPolicy(Qt::NoFocus);	// otherwise it's displayed in red once clicked
		auto templateMenu = new QMenu("Target");
		templateMenu->addAction(tr("Serial port"), this, SLOT(targetTemplateSerial()));
		templateMenu->addAction(tr("Local TCP"), this, SLOT(targetTemplateLocalTCP()));
		templateMenu->addAction(tr("TCP"), this, SLOT(targetTemplateTCP()));
		templateMenu->addSeparator();
		templateMenu->addAction(tr("Documentation"), this, SLOT(targetTemplateDoc()));
		templateButton->setMenu(templateMenu);
		targetLayout->addWidget(templateButton);
		mainLayout->addLayout(targetLayout);

		// language dialogue
		languageSelectionBox = new QComboBox;
		languageSelectionBox->addItem(QString::fromUtf8("English"), "en");
		languageSelectionBox->addItem(QString::fromUtf8("Français"), "fr");
		languageSelectionBox->addItem(QString::fromUtf8("Deutsch"), "de");
		languageSelectionBox->addItem(QString::fromUtf8("Español"), "es");
		languageSelectionBox->addItem(QString::fromUtf8("Italiano"), "it");
		languageSelectionBox->addItem(QString::fromUtf8("日本語"), "ja");
		languageSelectionBox->addItem(QString::fromUtf8("汉语"), "zh");
		languageSelectionBox->addItem(QString::fromUtf8("ελληνικά"), "el");
		languageSelectionBox->addItem(QString::fromUtf8("Türk"), "tr");
		languageSelectionBox->addItem(QString::fromUtf8("ру́сский язы́к"), "ru");
		languageSelectionBox->addItem(QString::fromUtf8("Filipino"), "tl");
		/* insert translation here (DO NOT REMOVE -> for automated script) */
		//qDebug() << "locale is " << QLocale::system().name();
		for (int i = 0; i < languageSelectionBox->count(); ++i)
		{
			if (QLocale::system().name().startsWith(languageSelectionBox->itemData(i).toString()))
			{
				languageSelectionBox->setCurrentIndex(i);
				break;
			}
		}
		mainLayout->addWidget(new QLabel(tr("Language")));
		mainLayout->addWidget(languageSelectionBox);

		// ok/cancel buttons
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		connectButton = new QPushButton(QIcon(":/images/ok.png"), tr("Connect"));
		buttonLayout->addWidget(connectButton);
		QPushButton* cancelButton = new QPushButton(QIcon(":/images/no.png"), tr("Cancel"));
		buttonLayout->addWidget(cancelButton);
		mainLayout->addLayout(buttonLayout);

		setWindowTitle(tr("Aseba Target Selection"));
		resize(720, 580);

		// connect signals and slots
		connect(discoveredList, SIGNAL(itemSelectionChanged()), SLOT(updateCurrentTarget()));
		connect(discoveredList, SIGNAL(itemDoubleClicked(QListWidgetItem *)), SLOT(connectToItem(QListWidgetItem *)));
		connect(currentTarget, SIGNAL(textEdited(const QString &)), discoveredList, SLOT(clearSelection()));
		connect(connectButton, SIGNAL(clicked(bool)), SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked(bool)), SLOT(reject()));
#ifdef ZEROCONF_SUPPORT
		connect(&zeroconf, SIGNAL(zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation&)), SLOT(zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation&)));
		connect(&zeroconf, SIGNAL(zeroconfTargetRemoved(const Aseba::Zeroconf::TargetInformation&)), SLOT(zeroconfTargetRemoved(const Aseba::Zeroconf::TargetInformation&)));
		zeroconf.browse();
#endif // ZEROCONF_SUPPORT

		// start timer and force a first update
		startTimer(1000);
		timerEvent(nullptr);
	}

#ifdef ZEROCONF_SUPPORT
	//! A target was found on zeroconf, add it to the list if not already present
	void DashelConnectionDialog::zeroconfTargetFound(const Aseba::Zeroconf::TargetInformation& target)
	{
		const auto dashelTarget(QString::fromUtf8(target.dashel().c_str()));
		const auto busy(target.properties.find("busy") != target.properties.end());
		// only add new Dashel targets
		for (int i = 0; i < discoveredList->count(); ++i)
		{
			QListWidgetItem *item(discoveredList->item(i));
			auto itemDashelTarget(item->data(Qt::UserRole));
			if (itemDashelTarget == dashelTarget)
			{
				if (busy)
					delete discoveredList->takeItem(discoveredList->row(item));
				return;
			}
		}
		// not found, if busy return
		if (busy)
			return;
		// resolve hostname and see if it is local
		const auto localAddresses(QNetworkInterface().allAddresses());
		const auto hostAddresses(QHostInfo::fromName(QString::fromStdString(target.host)).addresses());
		const bool isLocal(hasIntersection(localAddresses, hostAddresses));
		// create and add the item
		QString additionalInfo;
		try
		{
			if (target.properties.find("type") != target.properties.end())
				additionalInfo = QString(tr(" – type %1")).arg(QString::fromStdString(target.properties.at("type")));
		}
		catch (std::out_of_range& e)
		{}
		const auto name(QString::fromUtf8(target.name.c_str()));
		const auto host(QString::fromUtf8(target.host.c_str()));
		addEntry(name, isLocal ? tr("local on computer") : tr("distant on network"), dashelTarget, additionalInfo, { name });
	}

	//! A target was removed from zeroconf, remove it to the list if not already present
	void DashelConnectionDialog::zeroconfTargetRemoved(const Aseba::Zeroconf::TargetInformation& target)
	{
		for (int i = 0; i < discoveredList->count(); ++i)
		{
			const QListWidgetItem *item(discoveredList->item(i));
			if (item->data(Qt::UserRole + 1).toString().toStdString() == target.name)
			{
				delete discoveredList->takeItem(i);
				return;
			}
		}
	}
#endif // ZEROCONF_SUPPORT

	//! Update the port list, if toSelect is not empty, select the added item matching this name, return whether an item was selected
	bool DashelConnectionDialog::updatePortList(const QString& toSelect)
	{
		typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
		const PortsMap ports = SerialPortEnumerator::getPorts();
		bool discoveredListPortSet(false);
		const std::regex extractName("^(.*)\\ \\((.*)\\)$");
		std::vector<bool> seen(discoveredList->count(), false);

		// mark non-serial targets as seen
		for (int i = 0; i < discoveredList->count(); ++i)
		{
			const QListWidgetItem *item(discoveredList->item(i));
			auto dashelTarget(item->data(Qt::UserRole));
			if (!dashelTarget.toString().startsWith("ser:"))
				seen[i] = true;
		}

		// FIXME: change this algo from O(n^2) to O(n log(n))
		// add newly seen devices
		for (PortsMap::const_iterator it = ports.begin(); it != ports.end(); ++it)
		{
			const QString discoveredListDashelTarget(QString("ser:device=%0").arg(QString::fromUtf8(it->second.first.c_str())));
			// look if item is already in the list
			bool found(false);
			for (int i = 0; i < discoveredList->count(); ++i)
			{
				const QListWidgetItem *item(discoveredList->item(i));
				auto dashelTarget(item->data(Qt::UserRole));
				// this entry is the same as the detected port
				if (dashelTarget == discoveredListDashelTarget)
				{
					seen[i] = true;
					found = true;
				}
			}
			// if not, add it
			if (!found)
			{
				QString text;
				QString additionalInfo;
				std::smatch match;
				if (regex_match(it->second.second, match, extractName) && match.size() > 2)
				{
					text = QString::fromStdString(match.str(1));
					additionalInfo = tr(" – device %1").arg(QString::fromStdString(match.str(2)));
				}
				else
					text =  QString::fromStdString(it->second.second);
				auto item = addEntry(text, tr("serial port or USB"), discoveredListDashelTarget, additionalInfo);
				if (!toSelect.isEmpty() && (toSelect == text))
				{
					discoveredList->setCurrentItem(item);
					discoveredListPortSet = true;
				}
			}
		}
		// now removed unseen devices
		for (int i=seen.size()-1; i>=0; --i)
		{
			if (!seen[i])
				delete discoveredList->takeItem(i);
		}

		return discoveredListPortSet;
	}

	QListWidgetItem* DashelConnectionDialog::addEntry(const QString& title, const QString& connectionType, const QString& dashelTarget, const QString& additionalInfo, const QVariantList& additionalData)
	{
		auto* item = new QListWidgetItem();
		item->setSizeHint(QSize(200, discoveredList->fontMetrics().height() * 3.8));
		item->setData(Qt::UserRole, dashelTarget);
		for (int i = 0; i < additionalData.size(); ++i)
			item->setData(Qt::UserRole + 1 + i, additionalData[i]);
		discoveredList->addItem(item);
		auto* label(new QLabel(
			QString("<p style=\"line-height:120%;margin-left:10px;margin-right:10px;\"><b>%1</b> – %2%3<br/><span style=\"color:gray;\">%4</span></p>")
			.arg(title).arg(connectionType).arg(additionalInfo).arg(dashelTarget),
		discoveredList));
		label->setStyleSheet("QLabel { border-bottom: 1px solid gray }");
		discoveredList->setItemWidget(item, label);
		return item;
	}

	void DashelConnectionDialog::timerEvent(QTimerEvent * event)
	{
		updatePortList("");
	}

	std::string DashelConnectionDialog::getTarget()
	{
		QSettings settings;
		settings.setValue("current target", currentTarget->text());
		return currentTarget->text().toLocal8Bit().constData();
	}

	QString DashelConnectionDialog::getLocaleName()
	{
		return languageSelectionBox->itemData(languageSelectionBox->currentIndex()).toString();
	}

	void DashelConnectionDialog::updateCurrentTarget()
	{
		const QItemSelectionModel* model(discoveredList->selectionModel());
		if (model && !model->selectedRows().isEmpty())
		{
			const QModelIndex item(model->selectedRows().first());
			currentTarget->setText(item.data(Qt::UserRole).toString());
		}
	}

	void DashelConnectionDialog::connectToItem(QListWidgetItem *item)
	{
		currentTarget->setText(item->data(Qt::UserRole).toString());
		accept();
	}

	void DashelConnectionDialog::targetTemplateSerial()
	{
		currentTarget->setText("ser:device=/dev/ttyDev");
	}

	void DashelConnectionDialog::targetTemplateLocalTCP()
	{
		currentTarget->setText("tcp:host=localhost;port=33333");
	}

	void DashelConnectionDialog::targetTemplateTCP()
	{
		currentTarget->setText("tcp:host=192.168.1.200;port=33333");
	}

	void DashelConnectionDialog::targetTemplateDoc()
	{
		QDesktopServices::openUrl(QUrl("http://aseba-community.github.io/dashel#TargetNamingSec"));
	}


	DashelInterface::DashelInterface(QVector<QTranslator*> translators, const QString& commandLineTarget) :
		isRunning(true),
		stream(0)
	{
		// first use local name
		const QString& systemLocale(QLocale::system().name());
		assert(translators.size() == 4);
		translators[0]->load(QString("qt_") + systemLocale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		translators[1]->load(QString(":/asebastudio_") + systemLocale);
		translators[2]->load(QString(":/compiler_") + systemLocale);
		translators[3]->load(QString(":/qtabout_") + systemLocale);

		// try to connect to cammand line target, if any
		DashelConnectionDialog targetSelector;
		language = targetSelector.getLocaleName();
		if (!commandLineTarget.isEmpty())
		{
			bool failed = false;
			try
			{
				const std::string& testTarget(commandLineTarget.toStdString());
				stream = Hub::connect(testTarget);
				lastConnectedTarget = testTarget;
				lastConnectedTargetName = stream->getTargetName();
			}
			catch (DashelException e)
			{
				// exception, try again
				failed = true;
			}
			if (!failed)
				return;
#ifndef ANDROID
			// have user-friendly Thymio-specific message
			if (commandLineTarget == "ser:name=Thymio-II")
				QMessageBox::warning(0, tr("Thymio not found"), tr("<p><b>Cannot find Thymio!</b></p><p>Connect a Thymio to your computer using the USB cable/dongle, and make sure no other program is using Thymio.</p>"));
			else
				QMessageBox::warning(0, tr("Connection to command line target failed"), tr("Cannot connect to target %0").arg(commandLineTarget));
#endif
		}

		// show connection dialog
		while (true)
		{
			if (targetSelector.exec() == QDialog::Rejected)
			{
				throw std::runtime_error("connection dialog closed");
			}

			try
			{
				//qDebug() << "Connecting to " << targetSelector.getTarget().c_str();
				const std::string& testTarget(targetSelector.getTarget());
				stream = Hub::connect(testTarget);
				lastConnectedTarget = testTarget;
				lastConnectedTargetName = stream->getTargetName();
				assert(translators.size() == 4);
				language = targetSelector.getLocaleName();
				translators[0]->load(QString("qt_") + language, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
				translators[1]->load(QString(":/asebastudio_") + language);
				translators[2]->load(QString(":/compiler_") + language);
				translators[3]->load(QString(":/qtabout_") + language);
				break;
			}
			catch (DashelException e)
			{
				// exception, try again
			}
		}
	}

	bool DashelInterface::attemptToReconnect()
	{
		try
		{
			lock();
			assert(stream == 0);
			stream = Hub::connect(lastConnectedTarget);
			unlock();
			reset();
		}
		catch (DashelException e)
		{
			unlock();
		}

		try
		{
			// we have to stop hub because otherwise it will be forever in poll()
			Dashel::Hub::stop();
		}
		catch (DashelException e)
		{
		}

		return (stream != 0);
	}


	void DashelInterface::stop()
	{
		isRunning = false;
		Dashel::Hub::stop();
	}

	// In QThread main function, we just make our Dashel hub switch listen for incoming data
	void DashelInterface::run()
	{
		while (isRunning)
			Dashel::Hub::run();
	}

	void DashelInterface::incomingData(Stream *stream)
	{

		Message *message = Message::receive(stream);
		emit messageAvailable(message);
	}

	void DashelInterface::connectionClosed(Stream* stream, bool abnormal)
	{
		Q_UNUSED(stream);
		Q_UNUSED(abnormal);

		// mark all nodes as being disconnected
		for (NodesMap::iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); ++nodeIt)
		{
			nodeIt->second.connected = false;
			nodeDisconnected(nodeIt->first);
		}

		// notify target for showing reconnection message
		emit dashelDisconnection();
		Q_ASSERT(stream == this->stream);
		this->stream = 0;
	}

	void DashelInterface::sendMessage(const Message& message)
	{
		// this is called from the GUI thread through processMessage() or pingNetwork(), so we must lock the Hub before sending
		lock();
		if (stream)
		{
			try
			{
				message.serialize(stream);
				stream->flush();
				unlock();
			}
			catch(Dashel::DashelException e)
			{
				unlock();
				handleDashelException(e);
			}
		}
		else
		{
			unlock();
		}
	}

	void DashelInterface::nodeProtocolVersionMismatch(unsigned nodeId, const std::wstring &nodeName, uint16_t protocolVersion)
	{
		// show a different warning in function of the mismatch
		if (protocolVersion > ASEBA_PROTOCOL_VERSION)
		{
			QMessageBox::warning(0,
				QApplication::tr("Protocol version mismatch"),
				QApplication::tr("Aseba Studio uses an older protocol (%1) than node %0 (%2), please upgrade Aseba Studio.").arg(QString::fromStdWString(nodeName.c_str())).arg(ASEBA_PROTOCOL_VERSION).arg(protocolVersion)
			);
		}
		else if (protocolVersion < ASEBA_PROTOCOL_VERSION)
		{
			QMessageBox::warning(0,
				QApplication::tr("Protocol version mismatch"),
				QApplication::tr("Node %0 uses an older protocol (%2) than Aseba Studio (%1), please upgrade the node firmware.").arg(QString::fromStdWString(nodeName.c_str())).arg(ASEBA_PROTOCOL_VERSION).arg(protocolVersion)
			);
		}
	}

	void DashelInterface::nodeDescriptionReceived(unsigned nodeId)
	{
		emit nodeDescriptionReceivedSignal(nodeId);
	}

	void DashelInterface::nodeConnected(unsigned nodeId)
	{
		emit nodeConnectedSignal(nodeId);
	}

	void DashelInterface::nodeDisconnected(unsigned nodeId)
	{
		emit nodeDisconnectedSignal(nodeId);
	}


	enum InNextState
	{
		NOT_IN_NEXT,
		WAITING_INITAL_PC,
		WAITING_LINE_CHANGE
	};

	DashelTarget::Node::Node() :
		steppingInNext(NOT_IN_NEXT),
		lineInNext(0),
		executionMode(EXECUTION_UNKNOWN)
	{
	}

	DashelTarget::DashelTarget(QVector<QTranslator*> translators, const QString& commandLineTarget) :
		dashelInterface(translators, commandLineTarget),
		writeBlocked(false)
	{
		userEventsTimer.setSingleShot(true);
		connect(&userEventsTimer, SIGNAL(timeout()), SLOT(updateUserEvents()));

		// we connect the events from the stream listening thread to slots living in our gui thread
		connect(&dashelInterface, SIGNAL(messageAvailable(Message *)), SLOT(messageFromDashel(Message *)), Qt::QueuedConnection);
		connect(&dashelInterface, SIGNAL(dashelDisconnection()), SLOT(disconnectionFromDashel()), Qt::QueuedConnection);

		// we also connect to the description manager to know when we have a new node available
		connect(&dashelInterface, SIGNAL(nodeDescriptionReceivedSignal(unsigned)), SLOT(nodeDescriptionReceived(unsigned)));
		// note: queued connections are necessary to prevent multiple locking of Hub by the GUI thread
		connect(&dashelInterface, SIGNAL(nodeConnectedSignal(unsigned)), SIGNAL(nodeConnected(unsigned)));
		connect(&dashelInterface, SIGNAL(nodeDisconnectedSignal(unsigned)), SIGNAL(nodeDisconnected(unsigned)));

		// table for handling incoming messages
		messagesHandlersMap[ASEBA_MESSAGE_VARIABLES] = &Aseba::DashelTarget::receivedVariables;
		messagesHandlersMap[ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS] = &Aseba::DashelTarget::receivedArrayAccessOutOfBounds;
		messagesHandlersMap[ASEBA_MESSAGE_DIVISION_BY_ZERO] = &Aseba::DashelTarget::receivedDivisionByZero;
		messagesHandlersMap[ASEBA_MESSAGE_EVENT_EXECUTION_KILLED] = &Aseba::DashelTarget::receivedEventExecutionKilled;
		messagesHandlersMap[ASEBA_MESSAGE_NODE_SPECIFIC_ERROR] = &Aseba::DashelTarget::receivedNodeSpecificError;
		messagesHandlersMap[ASEBA_MESSAGE_EXECUTION_STATE_CHANGED] = &Aseba::DashelTarget::receivedExecutionStateChanged;
		messagesHandlersMap[ASEBA_MESSAGE_BREAKPOINT_SET_RESULT] = &Aseba::DashelTarget::receivedBreakpointSetResult;
		messagesHandlersMap[ASEBA_MESSAGE_BOOTLOADER_ACK] = &Aseba::DashelTarget::receivedBootloaderAck;

		dashelInterface.start();

		// list nodes every 1s
		connect(&listNodesTimer, SIGNAL(timeout()), SLOT(listNodes()));
		listNodesTimer.start(1000);
	}

	DashelTarget::~DashelTarget()
	{
		listNodesTimer.stop();
		dashelInterface.stop();
		dashelInterface.wait();
		DashelTarget::disconnect();
	}

	void DashelTarget::disconnect()
	{
		assert(writeBlocked == false);

		dashelInterface.lock();
		if (dashelInterface.stream)
		{
			try
			{
				// detach all nodes
				for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
				{
					BreakpointClearAll(node->first).serialize(dashelInterface.stream);
					Run(node->first).serialize(dashelInterface.stream);
				}
				dashelInterface.stream->flush();
				dashelInterface.unlock();
			}
			catch(Dashel::DashelException e)
			{
				dashelInterface.unlock();
				handleDashelException(e);
			}
		}
		else
			dashelInterface.unlock();
	}

	QList<unsigned> DashelTarget::getNodesList() const
	{
		QList<unsigned> nodeIds;
		for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
			nodeIds.append(node->first);
		return nodeIds;
	}

	const TargetDescription * const DashelTarget::getDescription(unsigned node) const
	{
		return dashelInterface.getDescription(node);
	}

	void DashelTarget::uploadBytecode(unsigned node, const BytecodeVector &bytecode)
	{
		dashelInterface.lock();
		if (dashelInterface.stream && !writeBlocked)
		{
			NodesMap::iterator nodeIt = nodes.find(node);
			assert(nodeIt != nodes.end());

			// fill debug bytecode and build address map
			nodeIt->second.debugBytecode = bytecode;
			nodeIt->second.eventAddressToId = bytecode.getEventAddressesToIds();

			// send bytecode
			try
			{
				sendBytecode(dashelInterface.stream, node, std::vector<uint16_t>(bytecode.begin(), bytecode.end()));
				dashelInterface.stream->flush();
				dashelInterface.unlock();
			}
			catch(Dashel::DashelException e)
			{
				dashelInterface.unlock();
				handleDashelException(e);
			}
		}
		else
			dashelInterface.unlock();
	}

	void DashelTarget::writeBytecode(unsigned node)
	{
		if (!writeBlocked)
		{
			WriteBytecode writeBytecodeMessage(node);
			dashelInterface.sendMessage(writeBytecodeMessage);
		}
	}

	void DashelTarget::reboot(unsigned node)
	{
		if (!writeBlocked)
		{
			Reboot rebootMessage(node);
			dashelInterface.sendMessage(rebootMessage);
		}
	}

 	void DashelTarget::sendEvent(unsigned id, const VariablesDataVector &data)
	{
		if (!writeBlocked)
		{
			UserMessage userMessage(id, data);
			dashelInterface.sendMessage(userMessage);
		}
	}

	void DashelTarget::setVariables(unsigned node, unsigned start, const VariablesDataVector &data)
	{
		if (!writeBlocked)
		{
			SetVariables setVariablesMessage(node, start, data);
			dashelInterface.sendMessage(setVariablesMessage);
		}
	}

	static QDateTime lastTime;
	static unsigned getVariablesCounter = 0;
	static unsigned VariablesCounter = 0;

	void DashelTarget::getVariables(unsigned node, unsigned start, unsigned length)
	{
		QElapsedTimer timer;
		timer.start();
		dashelInterface.lock();
		if (dashelInterface.stream && !writeBlocked)
		{
			const unsigned variablesPayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-1;

			try
			{
				while (length > variablesPayloadSize)
				{
					getVariablesCounter++;
					GetVariables(node, start, variablesPayloadSize).serialize(dashelInterface.stream);
					start += variablesPayloadSize;
					length -= variablesPayloadSize;
				}

				getVariablesCounter++;
				GetVariables(node, start, length).serialize(dashelInterface.stream);
				dashelInterface.stream->flush();
				dashelInterface.unlock();
			}
			catch(Dashel::DashelException e)
			{
				dashelInterface.unlock();
				handleDashelException(e);
			}
		}
		else
			dashelInterface.unlock();
		//qDebug() << "getVariables duration: " << timer.elapsed();

		QDateTime curTime(QDateTime::currentDateTime());
		//qDebug() << "variable rate" << lastTime.msecsTo(curTime);
		lastTime = curTime;
	}

	void DashelTarget::reset(unsigned node)
	{
		if (!writeBlocked)
		{
			Reset resetMessage(node);
			dashelInterface.sendMessage(resetMessage);
		}
	}

	void DashelTarget::run(unsigned node)
	{
		dashelInterface.lock();
		if (dashelInterface.stream && !writeBlocked)
		{
			NodesMap::iterator nodeIt = nodes.find(node);
			assert(nodeIt != nodes.end());

			try
			{
				if (nodeIt->second.executionMode == EXECUTION_STEP_BY_STEP)
					Step(node).serialize(dashelInterface.stream);
				Run(node).serialize(dashelInterface.stream);
				dashelInterface.stream->flush();
				dashelInterface.unlock();
			}
			catch(Dashel::DashelException e)
			{
				dashelInterface.unlock();
				handleDashelException(e);
			}
		}
		else
			dashelInterface.unlock();
	}

	void DashelTarget::pause(unsigned node)
	{
		if (!writeBlocked)
		{
			Pause pauseMessage(node);
			dashelInterface.sendMessage(pauseMessage);
		}
	}

	void DashelTarget::next(unsigned node)
	{
		if (!writeBlocked)
		{
			NodesMap::iterator nodeIt = nodes.find(node);
			assert(nodeIt != nodes.end());

			nodeIt->second.steppingInNext = WAITING_INITAL_PC;

			GetExecutionState getExecutionStateMessage(node);
			dashelInterface.sendMessage(getExecutionStateMessage);
		}
	}

	void DashelTarget::stop(unsigned node)
	{
		if (!writeBlocked)
		{
			Stop stopMessage(node);
			dashelInterface.sendMessage(stopMessage);
		}
	}

	void DashelTarget::setBreakpoint(unsigned node, unsigned line)
	{
		const int pc = getPCFromLine(node, line);
		if (pc < 0)
			return;

		if (!writeBlocked)
		{
			BreakpointSet breakpointSetMessage(node, pc);
			dashelInterface.sendMessage(breakpointSetMessage);
		}
	}

	void DashelTarget::clearBreakpoint(unsigned node, unsigned line)
	{
		const int pc = getPCFromLine(node, line);
		if (pc < 0)
			return;

		if (!writeBlocked)
		{
			BreakpointClear breakpointClearMessage(node, pc);
			dashelInterface.sendMessage(breakpointClearMessage);
		}
	}

	void DashelTarget::clearBreakpoints(unsigned node)
	{
		if (!writeBlocked)
		{
			BreakpointClearAll breakpointClearAllMessage(node);
			dashelInterface.sendMessage(breakpointClearAllMessage);
		}
	}

	void DashelTarget::blockWrite()
	{
		writeBlocked = true;
	}

	void DashelTarget::unblockWrite()
	{
		writeBlocked = false;
	}

	void DashelTarget::updateUserEvents()
	{
		// send only 20 latest user events
		if (userEventsQueue.size() > 20)
			emit userEventsDropped(userEventsQueue.size() - 20);
		while (userEventsQueue.size() > 20)
		{
			delete userEventsQueue.head();
			userEventsQueue.dequeue();
		}

		while (!userEventsQueue.isEmpty())
		{
			emit userEvent(userEventsQueue.head()->type, userEventsQueue.head()->data);
			delete userEventsQueue.head();
			userEventsQueue.dequeue();
		}
	}

	//! regularly probe aseba network for new connections
	void DashelTarget::listNodes()
	{
		dashelInterface.pingNetwork();
	}

	void DashelTarget::messageFromDashel(Message *message)
	{
		bool deleteMessage = true;
		//message->dump(std::cout);
		//std::cout << std::endl;

		// let the nodes manager filter the message
		if (!writeBlocked)
			dashelInterface.processMessage(message);

		// see if we have a registered handler for this message
		MessagesHandlersMap::const_iterator messageHandler = messagesHandlersMap.find(message->type);
		if (messageHandler == messagesHandlersMap.end())
		{
			UserMessage *userMessage = dynamic_cast<UserMessage *>(message);
			if (userMessage)
			{
				userEventsQueue.enqueue(userMessage);
				if (!userEventsTimer.isActive())
					userEventsTimer.start(50);
				deleteMessage = false;
			}
			/*else
				qDebug() << QString("Unhandeled non user message of type 0x%0 received from %1").arg(message->type, 6).arg(message->source);*/
		}
		else
		{
			(this->*(messageHandler->second))(message);
		}

		// if required, garbage collect this message
		if (deleteMessage)
			delete message;
	}


	ReconnectionDialog::ReconnectionDialog(DashelInterface& dashelInterface):
		dashelInterface(dashelInterface),
		counter(0)
	{
		setWindowTitle(tr("Connection closed"));
		setText(tr("Warning, connection closed: I am trying to reconnect."));
		addButton(tr("Stop trying"), QMessageBox::RejectRole);
		setEscapeButton(QMessageBox::Cancel);
		if (dashelInterface.lastConnectedTargetName.find("ser:") != 0)
			setIcon(QMessageBox::Warning);

		startTimer(200);
	}

	void ReconnectionDialog::timerEvent ( QTimerEvent * event )
	{
		if (dashelInterface.lastConnectedTargetName.find("ser:") == 0)
		{
			// serial port, show Thymio animation
			const unsigned iconStep(counter%15);
			if (iconStep < 8)
				setIconPixmap(QPixmap(QString(":/images/thymio-plugin-anim%0.png").arg(iconStep)));
		}
		const unsigned connectStep(counter%5);
		if ((connectStep == 0) && dashelInterface.attemptToReconnect())
			accept();
		counter++;
	}


	void DashelTarget::disconnectionFromDashel()
	{
		// show a dialog box that is trying to reconnect
		ReconnectionDialog reconnectionDialog(dashelInterface);
		reconnectionDialog.exec();
	}

	void DashelTarget::nodeDescriptionReceived(unsigned nodeId)
	{
		Node& node = nodes[nodeId];

		node.steppingInNext = NOT_IN_NEXT;
		node.lineInNext = 0;
	}

	void DashelTarget::receivedVariables(Message *message)
	{
		VariablesCounter++;
		//qDebug() << "diff get recv variables" << int(getVariablesCounter) - int(VariablesCounter);

		Variables *variables = polymorphic_downcast<Variables *>(message);

		emit variablesMemoryChanged(variables->source, variables->start, variables->variables);
	}

	void DashelTarget::receivedArrayAccessOutOfBounds(Message *message)
	{
		ArrayAccessOutOfBounds *aa = polymorphic_downcast<ArrayAccessOutOfBounds *>(message);

		int line = getLineFromPC(aa->source, aa->pc);
		if (line >= 0)
		{
			emit arrayAccessOutOfBounds(aa->source, line, aa->size, aa->index);
			emit executionModeChanged(aa->source, EXECUTION_STOP);
		}
	}

	void DashelTarget::receivedDivisionByZero(Message *message)
	{
		DivisionByZero *dz = polymorphic_downcast<DivisionByZero *>(message);

		int line = getLineFromPC(dz->source, dz->pc);
		if (line >= 0)
		{
			emit divisionByZero(dz->source, line);
			emit executionModeChanged(dz->source, EXECUTION_STOP);
		}
	}

	void DashelTarget::receivedEventExecutionKilled(Message *message)
	{
		EventExecutionKilled *eek = polymorphic_downcast<EventExecutionKilled *>(message);

		int line = getLineFromPC(eek->source, eek->pc);
		if (line >= 0)
		{
			emit eventExecutionKilled(eek->source, line);
		}
	}

	void DashelTarget::receivedNodeSpecificError(Message *message)
	{
		NodeSpecificError *nse = polymorphic_downcast<NodeSpecificError *>(message);

		int line = getLineFromPC(nse->source, nse->pc);
		// The NodeSpecificError can be triggered even if the pc is not valid
//		if (line >= 0)
//		{
			emit nodeSpecificError(nse->source, line, QString::fromStdWString(nse->message));
			emit executionModeChanged(nse->source, EXECUTION_STOP);
//		}
	}

	void DashelTarget::receivedExecutionStateChanged(Message *message)
	{
		ExecutionStateChanged *ess = polymorphic_downcast<ExecutionStateChanged *>(message);

		Node &node = nodes[ess->source];
		int line = getLineFromPC(ess->source, ess->pc);

		assert(writeBlocked == false);

		Target::ExecutionMode mode;
		if (ess->flags & ASEBA_VM_STEP_BY_STEP_MASK)
		{
			if (ess->flags & ASEBA_VM_EVENT_ACTIVE_MASK)
			{
				mode = EXECUTION_STEP_BY_STEP;
				if (line >= 0)
				{
					// Step by step, manage next
					if (node.steppingInNext == NOT_IN_NEXT)
					{
						emit executionPosChanged(ess->source, line);
						emit executionModeChanged(ess->source, mode);
						emit variablesMemoryEstimatedDirty(ess->source);
					}
					else if (node.steppingInNext == WAITING_INITAL_PC)
					{
						// we have line, do steps now
						node.lineInNext = line;
						node.steppingInNext = WAITING_LINE_CHANGE;

						dashelInterface.lock();
						if (dashelInterface.stream)
						{
							try
							{
								Step(ess->source).serialize(dashelInterface.stream);
								dashelInterface.stream->flush();
								dashelInterface.unlock();
							}
							catch(Dashel::DashelException e)
							{
								dashelInterface.unlock();
								handleDashelException(e);
							}
						}
						else
							dashelInterface.unlock();
					}
					else if (node.steppingInNext == WAITING_LINE_CHANGE)
					{
						if (
							(node.eventAddressToId.find(ess->pc) != node.eventAddressToId.end()) ||
							(line != static_cast<int>(node.lineInNext))
						)
						{
							node.steppingInNext = NOT_IN_NEXT;
							emit executionPosChanged(ess->source, line);
							emit executionModeChanged(ess->source, mode);
							emit variablesMemoryEstimatedDirty(ess->source);
						}
						else
						{
							dashelInterface.lock();
							if (dashelInterface.stream)
							{
								try
								{
									Step(ess->source).serialize(dashelInterface.stream);
									dashelInterface.stream->flush();
									dashelInterface.unlock();
								}
								catch(Dashel::DashelException e)
								{
									dashelInterface.unlock();
									handleDashelException(e);
								}
							}
							else
								dashelInterface.unlock();
						}
					}
					else
						assert(false);
					// we can safely return here, all case that require
					// emitting signals have been handeled
					node.executionMode = mode;
					return;
				}
			}
			else
			{
				mode = EXECUTION_STOP;
			}
		}
		else
		{
			mode = EXECUTION_RUN;
		}

		emit executionModeChanged(ess->source, mode);
		if (node.executionMode != mode)
		{
			emit variablesMemoryEstimatedDirty(ess->source);
			node.executionMode = mode;
		}
	}

	void DashelTarget::receivedBreakpointSetResult(Message *message)
	{
		BreakpointSetResult *bsr = polymorphic_downcast<BreakpointSetResult *>(message);
		unsigned node = bsr->source;
		emit breakpointSetResult(node, getLineFromPC(node, bsr->pc), bsr->success);
	}

	void DashelTarget::receivedBootloaderAck(Message *message)
	{
		BootloaderAck *ack = polymorphic_downcast<BootloaderAck*>(message);
		emit bootloaderAck(ack->errorCode, ack->errorAddress);
	}

	int DashelTarget::getPCFromLine(unsigned node, unsigned line)
	{
		// first lookup node
		NodesMap::const_iterator nodeIt = nodes.find(node);

		if (nodeIt == nodes.end())
			return -1;

		// then find PC
		for (size_t i = 0; i < nodeIt->second.debugBytecode.size(); i++)
			if (nodeIt->second.debugBytecode[i].line == line)
				return i;

		return -1;
	}

	int DashelTarget::getLineFromPC(unsigned node, unsigned pc)
	{
		// first lookup node
		NodesMap::const_iterator nodeIt = nodes.find(node);

		if (nodeIt == nodes.end())
			return -1;

		// then get line
		if (pc < nodeIt->second.debugBytecode.size())
			return nodeIt->second.debugBytecode[pc].line;

		return -1;
	}

	/*@}*/
}
