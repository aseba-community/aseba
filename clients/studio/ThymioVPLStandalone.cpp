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

#include "ThymioVPLStandalone.h"
#include "AeslEditor.h"
#include "plugins/ThymioVPL/ThymioVisualProgramming.h"
#include "../../common/consts.h"
#include "../../common/productids.h"
#include "translations/CompilerTranslator.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QScrollBar>
#include <QSplitter>

#include <version.h>

namespace Aseba
{
 	/** \addtogroup studio */
	/*@{*/
	
	ThymioVPLStandaloneInterface::ThymioVPLStandaloneInterface(ThymioVPLStandalone* vplStandalone):
		vplStandalone(vplStandalone)
	{
	};

	Target* ThymioVPLStandaloneInterface::getTarget()
	{
		return vplStandalone->target.get();
	}
	
	unsigned ThymioVPLStandaloneInterface::getNodeId()
	{
		return vplStandalone->id;
	}
	
	void ThymioVPLStandaloneInterface::displayCode(const QList<QString>& code, int elementToHighlight)
	{
		vplStandalone->editor->replaceAndHighlightCode(code, elementToHighlight);
	}
	
	void ThymioVPLStandaloneInterface::loadAndRun()
	{
		getTarget()->uploadBytecode(vplStandalone->id, vplStandalone->bytecode);
		getTarget()->run(vplStandalone->id);
	}

	void ThymioVPLStandaloneInterface::stop()
	{
		getTarget()->stop(vplStandalone->id);
	}
	
	bool ThymioVPLStandaloneInterface::saveFile(bool as)
	{
		return vplStandalone->saveFile(as);
	}

	void ThymioVPLStandaloneInterface::openFile()
	{
		vplStandalone->openFile();
	}
	
	bool ThymioVPLStandaloneInterface::newFile()
	{
		return true;
	}
	
	TargetVariablesModel * ThymioVPLStandaloneInterface::getVariablesModel()
	{
		return vplStandalone->variablesModel;
	}
	
	void ThymioVPLStandaloneInterface::setVariableValues(unsigned addr, const VariablesDataVector &data)
	{
		getTarget()->setVariables(vplStandalone->id, addr, data);
	}
	
	//////
	
	ThymioVPLStandalone::ThymioVPLStandalone(QVector<QTranslator*> translators, const QString& commandLineTarget):
		VariableListener(new TargetVariablesModel()),
		// create target
		target(new DashelTarget(translators, commandLineTarget)),
		// setup initial values
		id(0),
		vpl(0),
		allocatedVariablesCount(0),
		getDescriptionTimer(0)
	{
		subscribeToVariableOfInterest(ASEBA_PID_VAR_NAME);
		
		// create gui
		setupWidgets();
		setupConnections();
		
		// when everything is ready, get description
		target->broadcastGetDescription();
		
		// resize if not android
		#ifndef ANDROID
		resize(780,500);
		#endif // ANDROID
	}
	
	ThymioVPLStandalone::~ThymioVPLStandalone()
	{
		// delete variablesModel from VariableListener and set it to 0 to prevent double deletion
		delete variablesModel;
		variablesModel = 0;
	}
	
	void ThymioVPLStandalone::setupWidgets()
	{
		// VPL part
		vplLayout = new QVBoxLayout;
		QWidget* vplContainer = new QWidget;
		vplContainer->setLayout(vplLayout);
		
		addWidget(vplContainer);
		
		// editor part
		editor = new AeslEditor;
		editor->setReadOnly(true);
		#ifdef ANDROID
		editor->setTextInteractionFlags(Qt::NoTextInteraction);
		editor->setStyleSheet(
			"QTextEdit { font-size: 10pt; font-family: \"Courrier\" }"
		);
		editor->setTabStopWidth(QFontMetrics(editor->font()).width(' '));
		#endif // ANDROID
		new AeslHighlighter(editor, editor->document());
		
		/*QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(editor, 1);
		const QString text = tr("Aseba ver. %0 (build %1/protocol %2); Dashel ver. %3").
			arg(ASEBA_VERSION).
			arg(ASEBA_BUILD_VERSION).
			arg(ASEBA_PROTOCOL_VERSION).
			arg(DASHEL_VERSION);
		QLabel* label(new QLabel(text));
		label->setWordWrap(true);
		label->setStyleSheet("QLabel { font-size: 8pt; }");
		layout->addWidget(label);
		QWidget* textWidget = new QWidget;
		textWidget->setLayout(layout);*/
		
		addWidget(editor);
	}
	
	void ThymioVPLStandalone::setupConnections()
	{
		// editor
		connect(editor, SIGNAL(textChanged()), SLOT(editorContentChanged()));
		
		// target events
		connect(target.get(), SIGNAL(nodeConnected(unsigned)), SLOT(nodeConnected(unsigned)));
		connect(target.get(), SIGNAL(nodeDisconnected(unsigned)), SLOT(nodeDisconnected(unsigned)));
		connect(target.get(), SIGNAL(networkDisconnected()),  SLOT(networkDisconnected()));
		
		// right now, we ignore errors
		/*connect(target, SIGNAL(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)), SLOT(arrayAccessOutOfBounds(unsigned, unsigned, unsigned, unsigned)));
		connect(target, SIGNAL(divisionByZero(unsigned, unsigned)), SLOT(divisionByZero(unsigned, unsigned)));
		connect(target, SIGNAL(eventExecutionKilled(unsigned, unsigned)), SLOT(eventExecutionKilled(unsigned, unsigned)));
		connect(target, SIGNAL(nodeSpecificError(unsigned, unsigned, QString)), SLOT(nodeSpecificError(unsigned, unsigned, QString)));
		*/
		
		connect(target.get(), SIGNAL(variablesMemoryEstimatedDirty(unsigned)), SLOT(variablesMemoryEstimatedDirty(unsigned)));
		connect(target.get(), SIGNAL(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)), SLOT(variablesMemoryChanged(unsigned, unsigned, const VariablesDataVector &)));
	}
	
	void ThymioVPLStandalone::resizeEvent( QResizeEvent *event )
	{
		if (event->size().height() > event->size().width())
			setOrientation(Qt::Vertical);
		else
			setOrientation(Qt::Horizontal);
		QSplitter::resizeEvent(event);
		resetSizes();
	}
	
	void ThymioVPLStandalone::resetSizes()
	{
		// make sure that VPL is larger than the editor
		QList<int> sizes;
		if (orientation() == Qt::Vertical)
		{
			sizes.push_back((height() * 5) / 7);
			sizes.push_back((height() * 2) / 7);
		}
		else
		{
			sizes.push_back((width() * 2) / 3);
			sizes.push_back((width() * 1) / 3);
		}
		setSizes(sizes);
	}
	
	//! The content of a variable has changed
	void ThymioVPLStandalone::variableValueUpdated(const QString& name, const VariablesDataVector& values)
	{
		if ((name == ASEBA_PID_VAR_NAME) && (values.size() >= 1))
		{
			// make sure that pid is ASEBA_PID_THYMIO2, otherwise print an error and quit
			if (values[0] != ASEBA_PID_THYMIO2)
			{
				QMessageBox::critical(this, tr("Thymio VPL Error"), tr("You need to connect a Thymio II to use this application"));
				close();
			}
		}
	}
	
	//! Received a close event, forward to VPL
	void ThymioVPLStandalone::closeEvent ( QCloseEvent * event )
	{
		if (vpl && !vpl->closeFile())
			event->ignore();
	}
	
	//! Save a minimal but valid aesl file
	bool ThymioVPLStandalone::saveFile(bool as)
	{
		// we need a valid VPL to save something
		if (!vpl)
			return false;
		
		// open file
		if (as || fileName.isEmpty())
		{
			#ifdef ANDROID
			if (fileName.isEmpty())
				fileName = "/sdcard/";
			#endif // ANDROID
			fileName = QFileDialog::getSaveFileName(this,
				tr("Save Script"), fileName, "Aseba scripts (*.aesl)");
		}
		
		if (fileName.isEmpty())
			return false;
		
		if (fileName.lastIndexOf(".") < 0)
			fileName += ".aesl";
		
		QFile file(fileName);
		if (!file.open(QFile::WriteOnly | QFile::Truncate))
			return false;
		
		// initiate DOM tree
		QDomDocument document("aesl-source");
		QDomElement root = document.createElement("network");
		document.appendChild(root);
		
		// add a node for VPL
		QDomElement element = document.createElement("node");
		element.setAttribute("name", target->getName(id));
		element.setAttribute("nodeId", id);
		element.appendChild(document.createTextNode(editor->toPlainText()));
		QDomElement plugins = document.createElement("toolsPlugins");
		QDomElement plugin(document.createElement("ThymioVisualProgramming"));
		plugin.appendChild(document.importNode(vpl->saveToDom().documentElement(), true));
		plugins.appendChild(plugin);
		element.appendChild(plugins);
		root.appendChild(element);
		
		QTextStream out(&file);
		document.save(out, 0);
		
		return true;
	}

	//! Load a aesl file
	void ThymioVPLStandalone::openFile()
	{
		// we need a valid VPL to save something
		if (!vpl)
			return;
		
		#ifdef ANDROID
		QString dir("/sdcard/");
		#else // ANDROID
		QString dir("");
		#endif // ANDROID
		
		// get file name
		const QString newFileName(QFileDialog::getOpenFileName(this,
				tr("Open Script"), dir, "Aseba scripts (*.aesl)"));
		QFile file(newFileName);
		if (!file.open(QFile::ReadOnly))
			return;
		
		// open DOM document
		QDomDocument document("aesl-source");
		QString errorMsg;
		int errorLine;
		int errorColumn;
		if (document.setContent(&file, false, &errorMsg, &errorLine, &errorColumn))
		{
			// load Thymio node data
			bool dataLoaded(false);
			QDomNode domNode = document.documentElement().firstChild();
			while (!domNode.isNull())
			{
				// if the element is a node
				if (domNode.isElement())
				{
					QDomElement element = domNode.toElement();
					if (element.tagName() == "node")
					{
						// if node corresponds to the connected robot
						if ((element.attribute("nodeId").toUInt() == id) &&
							(element.attribute("name") == target->getName(id)))
						{
							// restore VPL
							QDomElement toolsPlugins(element.firstChildElement("toolsPlugins"));
							QDomElement toolPlugin(toolsPlugins.firstChildElement());
							while (!toolPlugin.isNull())
							{
								// if VPL found
								if (toolPlugin.nodeName() == "ThymioVisualProgramming")
								{
									// restore VPL data
									QDomDocument pluginDataDocument("tool-plugin-data");
									pluginDataDocument.appendChild(pluginDataDocument.importNode(toolPlugin.firstChildElement(), true));
									vpl->loadFromDom(pluginDataDocument, true);
									dataLoaded = true;
									break;
								}
								toolPlugin = toolPlugin.nextSiblingElement();
							}
						}
					}
				}
				domNode = domNode.nextSibling();
			}
			// check whether we did load data
			if(dataLoaded)
			{
				fileName = newFileName;
			}
			else
			{
				QMessageBox::warning(this,
					tr("Loading"),
					tr("No Thymio VPL data were found in the script file, file ignored")
				);
			}
		}
		else
		{
			QMessageBox::warning(this,
				tr("Loading"),
				tr("Error in XML source file: %0 at line %1, column %2").arg(errorMsg).arg(errorLine).arg(errorColumn)
			);
		}
		
		file.close();
	}
	
	//! If any node was disconnected, send get description
	void ThymioVPLStandalone::timerEvent ( QTimerEvent * event )
	{
		target.get()->broadcastGetDescription();
	}
	
	//! The content of the editor was changed by VPL, recompile
	void ThymioVPLStandalone::editorContentChanged()
	{
		const TargetDescription* targetDescription(target->getDescription(id));
		if (targetDescription)
		{
			CommonDefinitions commonDefinitions;
			Compiler compiler;
			compiler.setTargetDescription(target->getDescription(id));
			compiler.setTranslateCallback(CompilerTranslator::translate);
			compiler.setCommonDefinitions(&commonDefinitions);
			
			std::wistringstream is(editor->toPlainText().toStdWString());
			
			Aseba::Error error;
			const bool success = compiler.compile(is, bytecode, allocatedVariablesCount, error);
			if (success)
			{
				variablesModel->updateVariablesStructure(compiler.getVariablesMap());
			}
			else
			{
				wcerr << "AESL Compilation error: " << error.toWString() << endl;
			}
		}
	}
	
	//! A new node has connected to the network.
	void ThymioVPLStandalone::nodeConnected(unsigned node)
	{
		// only allow a single node connected at a given time
		if (vpl)
		{
			QMessageBox::critical(this, tr("Thymio VPL Error"), tr("This application only supports a single robot at a time"));
			close();
			return;
		}
			
		// save node information
		id = node;
		
		// create the VPL widget
		vpl = new ThymioVisualProgramming(new ThymioVPLStandaloneInterface(this), false);
		vpl->loadFromDom(savedContent, false);
		vplLayout->addWidget(vpl);
		
		// connect callbacks
		connect(vpl, SIGNAL(compilationOutcome(bool)), editor, SLOT(setEnabled(bool)));
		
		// reset sizes
		resetSizes();
		
		// do a first compilation
		editorContentChanged();
		
		// read variables once to get PID
		target->getVariables(id, 0, allocatedVariablesCount);
		
		// stop timer if any
		if (getDescriptionTimer)
		{
			killTimer(getDescriptionTimer);
			getDescriptionTimer = 0;
		}
	}
	
	//! A node has disconnected from the network.
	void ThymioVPLStandalone::nodeDisconnected(unsigned node)
	{
		if (vpl && (node == id))
		{
			savedContent = vpl->saveToDom();
			vplLayout->removeWidget(vpl);
			delete vpl;
			vpl = 0;
		}
	}
	
	//! The network connection has been cut: disconnect VPL node if connected
	void ThymioVPLStandalone::networkDisconnected()
	{
		// disconnect VPL if present
		if (vpl)
			nodeDisconnected(id);
		// start timer for reconnection
		if (!getDescriptionTimer)
			getDescriptionTimer = startTimer(2000);
	}
	
	//! The execution state logic thinks variables might need a refresh
	void ThymioVPLStandalone::variablesMemoryEstimatedDirty(unsigned node)
	{
		if (node == id)
			target->getVariables(id, 0, allocatedVariablesCount);
	}
	
	//! Content of target memory has changed
	void ThymioVPLStandalone::variablesMemoryChanged(unsigned node, unsigned start, const VariablesDataVector &variables)
	{
		// update variables model
		if (node == id)
			variablesModel->setVariablesData(start, variables);
	}
	
	/*@}*/
}; // Aseba