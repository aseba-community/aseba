/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2012:
		Stephane Magnenat <stephane at magnenat dot net>
		(http://stephane.magnenat.net)
		and other contributors, see authors.txt for details
	
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
#include "../msg/msg.h"
#include "../utils/utils.h"
#include <algorithm>
#include <iostream>
#include <ostream>
#include <sstream>
#include <cassert>
#include <QInputDialog>
#include <QtGui>
#include <QLibraryInfo>
#include <stdexcept>


#include <DashelTarget.moc>

#ifdef WIN32 // for Sleep
#include <windows.h>
#endif

namespace Aseba
{
	using std::copy;
	using namespace Dashel;

	/** \addtogroup studio */
	/*@{*/
	
	DashelConnectionDialog::DashelConnectionDialog()
	{
		typedef std::map<int, std::pair<std::string, std::string> > PortsMap;
		const PortsMap ports = SerialPortEnumerator::getPorts();
		
		QSettings settings;
		unsigned sectionEnabled(settings.value("connection dialog enabled group", 0).toUInt());
		if ((sectionEnabled == 1) && (ports.size() == 0))
			sectionEnabled = 0;
		
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		
		netGroupBox = new QGroupBox(tr("Network (TCP)"));
		netGroupBox->setCheckable(true);
		netGroupBox->setChecked(sectionEnabled == 0);
		QGridLayout* netLayout = new QGridLayout;
		netLayout->addWidget(new QLabel(tr("Host")), 0, 0);
		netLayout->addWidget(new QLabel(tr("Port")), 0, 1);
		host = new QLineEdit(settings.value("tcp host", ASEBA_DEFAULT_HOST).toString());
		netLayout->addWidget(host, 1, 0);
		port = new QSpinBox();
		port->setMinimum(0);
		port->setMaximum(65535);
		port->setValue(settings.value("tcp port", ASEBA_DEFAULT_PORT).toInt());
		netLayout->addWidget(port, 1, 1);
		netGroupBox->setLayout(netLayout);
		connect(netGroupBox, SIGNAL(clicked()), SLOT(netGroupChecked()));
		mainLayout->addWidget(netGroupBox);
		
		serialGroupBox = new QGroupBox(tr("Serial"));
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
			if (settings.value("serial name") == text)
			{
				serial->setCurrentItem(item);
				serialPortSet = true;
			}
		}
		if (sectionEnabled == 1 && !serialPortSet)
			sectionEnabled = 2;
		serialGroupBox->setChecked(sectionEnabled == 1);
		serial->setSelectionMode(QAbstractItemView::SingleSelection);
		serialLayout->addWidget(serial);
		connect(serial, SIGNAL(itemSelectionChanged()), SLOT(setupOkStateFromListSelection()));
		serialGroupBox->setLayout(serialLayout);
		connect(serialGroupBox, SIGNAL(clicked()), SLOT(serialGroupChecked()));
		mainLayout->addWidget(serialGroupBox);
		
		customGroupBox = new QGroupBox(tr("Custom"));
		customGroupBox->setCheckable(true);
		customGroupBox->setChecked(sectionEnabled == 2);
		QHBoxLayout* customLayout = new QHBoxLayout();
		custom = new QLineEdit(settings.value("custom target", ASEBA_DEFAULT_TARGET).toString());
		customLayout->addWidget(custom);
		customGroupBox->setLayout(customLayout);
		connect(customGroupBox, SIGNAL(clicked()), SLOT(customGroupChecked()));
		mainLayout->addWidget(customGroupBox);
		
		languageSelectionBox = new QComboBox;
		languageSelectionBox->addItem(QString::fromUtf8("English"), "en");
		languageSelectionBox->addItem(QString::fromUtf8("Français"), "fr");
		languageSelectionBox->addItem(QString::fromUtf8("Deutsch"), "de");
		languageSelectionBox->addItem(QString::fromUtf8("Español"), "es");
		languageSelectionBox->addItem(QString::fromUtf8("Italiano"), "it");
		//qDebug() << "locale is " << QLocale::system().name();
		for (int i = 0; i < languageSelectionBox->count(); ++i)
		{
			if (QLocale::system().name().startsWith(languageSelectionBox->itemData(i).toString()))
			{
				languageSelectionBox->setCurrentIndex(i);
				break;
			}
		}
		mainLayout->addWidget(languageSelectionBox);
		
		QHBoxLayout* buttonLayout = new QHBoxLayout();
		connectButton = new QPushButton(QIcon(":/images/ok.png"), tr("Connect"));
		connect(connectButton, SIGNAL(clicked(bool)), SLOT(accept()));
		buttonLayout->addWidget(connectButton);
		QPushButton* cancelButton = new QPushButton(QIcon(":/images/no.png"), tr("Cancel"));
		connect(cancelButton, SIGNAL(clicked(bool)), SLOT(reject()));
		buttonLayout->addWidget(cancelButton);
		mainLayout->addLayout(buttonLayout);
		
		setWindowTitle(tr("Aseba Target Selection"));
	}
	
	std::string DashelConnectionDialog::getTarget()
	{
		QSettings settings;
		if (netGroupBox->isChecked())
		{
			settings.setValue("connection dialog enabled group", 0);
			settings.setValue("tcp host", host->text());
			settings.setValue("tcp port", port->value());
			std::ostringstream oss;
			oss << "tcp:host=" << host->text().toLocal8Bit().constData() << ";port=" << port->value();
			return oss.str();
		}
		else if (serialGroupBox->isChecked())
		{
			const QItemSelectionModel* model(serial->selectionModel());
			assert(model && !model->selectedRows().isEmpty());
			const QModelIndex item(model->selectedRows().first());
			settings.setValue("connection dialog enabled group", 1);
			settings.setValue("serial name", item.data());
			QString target("ser:device=%0");
			return target.arg(item.data(Qt::UserRole).toString()).toLocal8Bit().constData();
		}
		else if (customGroupBox->isChecked())
		{
			settings.setValue("connection dialog enabled group", 2);
			settings.setValue("custom target", custom->text());
			return custom->text().toLocal8Bit().constData();
		}
		else
		{
			assert(false);
			return "";
		}
	}
	
	QString DashelConnectionDialog::getLocaleName()
	{
		return languageSelectionBox->itemData(languageSelectionBox->currentIndex()).toString();
	}
	
	void DashelConnectionDialog::netGroupChecked()
	{
		netGroupBox->setChecked(true);
		serialGroupBox->setChecked(false);
		customGroupBox->setChecked(false);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::serialGroupChecked()
	{
		netGroupBox->setChecked(false);
		serialGroupBox->setChecked(true);
		customGroupBox->setChecked(false);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::customGroupChecked()
	{
		netGroupBox->setChecked(false);
		serialGroupBox->setChecked(false);
		customGroupBox->setChecked(true);
		setupOkStateFromListSelection();
	}
	
	void DashelConnectionDialog::setupOkStateFromListSelection()
	{
		connectButton->setEnabled(serial->selectionModel()->hasSelection() || (!serialGroupBox->isChecked()));
	}
	
	
	DashelInterface::DashelInterface(QVector<QTranslator*> translators, const QString& commandLineTarget) :
		isRunning(true),
		stream(0)
	{
		// first use local name
		const QString& systemLocale(QLocale::system().name());
		translators[0]->load(QString("qt_") + systemLocale, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
		translators[1]->load(QString(":/asebastudio_") + systemLocale);
		translators[2]->load(QString(":/compiler_") + systemLocale);
		
		// try to connect to cammand line target, if any
		DashelConnectionDialog targetSelector;
		if (!commandLineTarget.isEmpty())
		{
			bool failed = false;
			try
			{
				const std::string& testTarget(commandLineTarget.toStdString());
				stream = Hub::connect(testTarget);
				lastConnectedTarget = testTarget;
			}
			catch (DashelException e)
			{
				// exception, try again
				failed = true;
			}
			
			if (failed)
				QMessageBox::warning(0, tr("Connection to command line target failed"), tr("Cannot connect to target %0").arg(commandLineTarget));
			else
				return;
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
				assert(translators.size() == 3);
				language = targetSelector.getLocaleName();
				translators[0]->load(QString("qt_") + language, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
				translators[1]->load(QString(":/asebastudio_") + language);
				translators[2]->load(QString(":/compiler_") + language);
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
		assert(stream == 0);
		try
		{
			// we have to stop hub because otherwise it will be forever in poll()
			stream = Hub::connect(lastConnectedTarget);
			Dashel::Hub::stop();
			if (stream)
				return true;
		}
		catch (DashelException e)
		{
		}
		return false;
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
		emit dashelDisconnection();
		Q_ASSERT(stream == this->stream);
		this->stream = 0;
	}
	
	void SignalingDescriptionsManager::nodeProtocolVersionMismatch(const std::string &nodeName, uint16 protocolVersion)
	{
		if (protocolVersion > ASEBA_PROTOCOL_VERSION)
		{
			QMessageBox::warning(0,
				QApplication::tr("Protocol version mismatch"),
				QApplication::tr("Aseba Studio uses an older protocol (%1) than node %0 (%2), please upgrade Aseba Studio.").arg(QString::fromUtf8(nodeName.c_str())).arg(ASEBA_PROTOCOL_VERSION).arg(protocolVersion)
			);
		}
		else if (protocolVersion < ASEBA_PROTOCOL_VERSION)
		{
			QMessageBox::warning(0,
				QApplication::tr("Protocol version mismatch"),
				QApplication::tr("Node %0 uses an older protocol (%2) than Aseba Studio (%1), please upgrade the node firmware.").arg(QString::fromUtf8(nodeName.c_str())).arg(ASEBA_PROTOCOL_VERSION).arg(protocolVersion)
			);
		}
	}
	
	void SignalingDescriptionsManager::nodeDescriptionReceived(unsigned nodeId)
	{
		emit nodeDescriptionReceivedSignal(nodeId);
	}
	
	
	enum InNextState
	{
		NOT_IN_NEXT,
		WAITING_INITAL_PC,
		WAITING_LINE_CHANGE
	};
	
	DashelTarget::Node::Node()
	{
		executionMode = EXECUTION_UNKNOWN;
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
		connect(&descriptionManager, SIGNAL(nodeDescriptionReceivedSignal(unsigned)), SLOT(nodeDescriptionReceived(unsigned)));
		
		messagesHandlersMap[ASEBA_MESSAGE_DISCONNECTED] = &Aseba::DashelTarget::receivedDisconnected;
		messagesHandlersMap[ASEBA_MESSAGE_VARIABLES] = &Aseba::DashelTarget::receivedVariables;
		messagesHandlersMap[ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS] = &Aseba::DashelTarget::receivedArrayAccessOutOfBounds;
		messagesHandlersMap[ASEBA_MESSAGE_DIVISION_BY_ZERO] = &Aseba::DashelTarget::receivedDivisionByZero;
		messagesHandlersMap[ASEBA_MESSAGE_EVENT_EXECUTION_KILLED] = &Aseba::DashelTarget::receivedEventExecutionKilled;
		messagesHandlersMap[ASEBA_MESSAGE_NODE_SPECIFIC_ERROR] = &Aseba::DashelTarget::receivedNodeSpecificError;
		messagesHandlersMap[ASEBA_MESSAGE_EXECUTION_STATE_CHANGED] = &Aseba::DashelTarget::receivedExecutionStateChanged;
		messagesHandlersMap[ASEBA_MESSAGE_BREAKPOINT_SET_RESULT] = &Aseba::DashelTarget::receivedBreakpointSetResult;
		messagesHandlersMap[ASEBA_MESSAGE_BOOTLOADER_ACK] = &Aseba::DashelTarget::receivedBootloaderAck;

		dashelInterface.start();
	}
	
	DashelTarget::~DashelTarget()
	{
		dashelInterface.stop();
		dashelInterface.wait();
		DashelTarget::disconnect();
	}
	
	void DashelTarget::disconnect()
	{
		assert(writeBlocked == false);
		
		if (dashelInterface.stream)
		{
			try
			{
				// detach all nodes
				for (NodesMap::const_iterator node = nodes.begin(); node != nodes.end(); ++node)
				{
					//DetachDebugger(node->first).serialize(dashelInterface.stream);
					BreakpointClearAll(node->first).serialize(dashelInterface.stream);
					Run(node->first).serialize(dashelInterface.stream);
				}
				dashelInterface.stream->flush();
			}
			catch(Dashel::DashelException e)
			{
				handleDashelException(e);
			}
		}
	}
	
	const TargetDescription * const DashelTarget::getDescription(unsigned node) const
	{
		return descriptionManager.getDescription(node);
	}
	
	void DashelTarget::broadcastGetDescription()
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			GetDescription().serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::uploadBytecode(unsigned node, const BytecodeVector &bytecode)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		// fill debug bytecode and build address map
		nodeIt->second.debugBytecode = bytecode;
		nodeIt->second.eventAddressToId = bytecode.getEventAddressesToIds();
		
		// send bytecode
		try
		{
			sendBytecode(dashelInterface.stream, node, std::vector<uint16>(bytecode.begin(), bytecode.end()));
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::writeBytecode(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			WriteBytecode(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::reboot(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			Reboot(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::sendEvent(unsigned id, const VariablesDataVector &data)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			UserMessage(id, data).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::setVariables(unsigned node, unsigned start, const VariablesDataVector &data)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			SetVariables(node, start, data).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::getVariables(unsigned node, unsigned start, unsigned length)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		const unsigned variablesPayloadSize = ASEBA_MAX_EVENT_ARG_COUNT-1;

		try
		{
			while (length > variablesPayloadSize)
			{
				GetVariables(node, start, variablesPayloadSize).serialize(dashelInterface.stream);
				start += variablesPayloadSize;
				length -= variablesPayloadSize;
			}

			GetVariables(node, start, length).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::reset(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			Reset(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::run(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		try
		{
			if (nodeIt->second.executionMode == EXECUTION_STEP_BY_STEP)
				Step(node).serialize(dashelInterface.stream);
			Run(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::pause(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			Pause(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::next(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		NodesMap::iterator nodeIt = nodes.find(node);
		assert(nodeIt != nodes.end());
		
		nodeIt->second.steppingInNext = WAITING_INITAL_PC;
		
		GetExecutionState getExecutionStateMessage;
		getExecutionStateMessage.dest = node;
		
		try
		{
			getExecutionStateMessage.serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::stop(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		try
		{
			Stop(node).serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
		}
	}
	
	void DashelTarget::setBreakpoint(unsigned node, unsigned line)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointSet breakpointSetMessage;
			breakpointSetMessage.pc = pc;
			breakpointSetMessage.dest = node;
			
			try
			{
				breakpointSetMessage.serialize(dashelInterface.stream);
				dashelInterface.stream->flush();
			}
			catch(Dashel::DashelException e)
			{
				handleDashelException(e);
			}
		}
	}
	
	void DashelTarget::clearBreakpoint(unsigned node, unsigned line)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		int pc = getPCFromLine(node, line);
		if (pc >= 0)
		{
			BreakpointClear breakpointClearMessage;
			breakpointClearMessage.pc = pc;
			breakpointClearMessage.dest = node;
			
			try
			{
				breakpointClearMessage.serialize(dashelInterface.stream);
				dashelInterface.stream->flush();
			}
			catch(Dashel::DashelException e)
			{
				handleDashelException(e);
			}
		}
	}
	
	void DashelTarget::clearBreakpoints(unsigned node)
	{
		if (writeBlocked || !dashelInterface.stream) return;
		
		BreakpointClearAll breakpointClearAllMessage;
		breakpointClearAllMessage.dest = node;
		
		try
		{
			breakpointClearAllMessage.serialize(dashelInterface.stream);
			dashelInterface.stream->flush();
		}
		catch(Dashel::DashelException e)
		{
			handleDashelException(e);
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
	
	void DashelTarget::messageFromDashel(Message *message)
	{
		bool deleteMessage = true;
		//message->dump(std::cout);
		//std::cout << std::endl;
		
		// let the description manager filter the message
		descriptionManager.processMessage(message);
		
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
	
	struct ReconnectionDialog: public QMessageBox
	{
		ReconnectionDialog(DashelInterface& dashelInterface):
			dashelInterface(dashelInterface)
		{
			setWindowTitle(tr("Aseba Studio - Connection closed"));
			setText(tr("Warning, connection closed: I am trying to reconnect."));
			setStandardButtons(QMessageBox::Cancel);
			setEscapeButton(QMessageBox::Cancel);
			setIcon(QMessageBox::Warning);
			
			startTimer(1000);
		}
	
	protected:
		virtual void timerEvent ( QTimerEvent * event )
		{
			if (dashelInterface.attemptToReconnect())
				accept();
		}
		
		DashelInterface& dashelInterface;
	};
	
	void DashelTarget::disconnectionFromDashel()
	{
		emit networkDisconnected();
		nodes.clear();
		descriptionManager.reset();
		
		// show a dialog box that is trying to reconnect
		ReconnectionDialog reconnectionDialog(dashelInterface);
		reconnectionDialog.exec();
	}
	
	void DashelTarget::nodeDescriptionReceived(unsigned nodeId)
	{
		Node& node = nodes[nodeId];
		
		node.steppingInNext = NOT_IN_NEXT;
		node.lineInNext = 0;
		
		emit nodeConnected(nodeId);
	}
	
	void DashelTarget::receivedVariables(Message *message)
	{
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
		assert(dashelInterface.stream);
		
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
						
						try
						{
							Step(ess->source).serialize(dashelInterface.stream);
							dashelInterface.stream->flush();
						}
						catch(Dashel::DashelException e)
						{
							handleDashelException(e);
						}
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
							try
							{
								Step(ess->source).serialize(dashelInterface.stream);
								dashelInterface.stream->flush();
							}
							catch(Dashel::DashelException e)
							{
								handleDashelException(e);
							}
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
	
	void DashelTarget::receivedDisconnected(Message *message)
	{
		Disconnected *disconnected = polymorphic_downcast<Disconnected *>(message);
		
		emit nodeDisconnected(disconnected->source);
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

	void DashelTarget::handleDashelException(Dashel::DashelException e)
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
			QMessageBox::critical(NULL, tr("Unexpected Dashel Error"), tr("A communication error happened:") + " (" + QString::number(e.source) + ") " + e.what());
			break;
		}
	}
	
	/*@}*/
}
