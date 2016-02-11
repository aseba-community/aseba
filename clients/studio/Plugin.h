/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2016:
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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QLabel>
#include <QDomDocument>
#include <QMultiMap>
#include <stdexcept>
#include <iostream>
#include <set>
#include <memory>
#include <dashel/dashel.h>
#include "../../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class Target;
	class TargetVariablesModel;
	
	// TODO: constify DevelopmentEnvironmentInterface
	
	//! To access private members of the development environment (MainWindow and its children), a plugin must use the method of a subclass of this class.
	//! This class interfaces to a specific node
	struct DevelopmentEnvironmentInterface
	{
		//! Virtual destructor
		virtual ~DevelopmentEnvironmentInterface() {}
		//! Return the target Aseba network
		virtual Target * getTarget() = 0;
		//! Return the node ID of the node this plugin talks to
		virtual unsigned getNodeId() const = 0;
		//! Return the product ID of the node this plugin talks to
		virtual unsigned getProductId() const = 0;
		//! Set constants and global events
		virtual void setCommonDefinitions(const CommonDefinitions& commonDefinitions) = 0;
		//! Set the code of the editor from a sequence of elements (each can be multiple sloc) and highlight one of these elements
		virtual void displayCode(const QList<QString>& code, int elementToHighlight) = 0;
		//! Load code and runs it on the node
		virtual void loadAndRun() = 0;
		//! Stop execution of the node
		virtual void stop() = 0;
		//! Return the variables model of the node
		virtual TargetVariablesModel * getVariablesModel() = 0;
		//! Set variables on the node
		virtual void setVariableValues(unsigned addr, const VariablesDataVector &data) = 0;
		//! Request the DE to save the current file
		virtual bool saveFile(bool as=false) = 0;
		//! Request the DE to open an existing file
		virtual void openFile() = 0;
		//! Request the DE to create a new empty file
		virtual bool newFile() = 0;
		//! Request the DE to clear the name of the opened file, without any check for change
		virtual void clearOpenedFileName(bool isModified) = 0;
		//! Request the name of the opened file, if any
		virtual QString openedFileName() const = 0;
	};
	
	//! A tool that is specific to a node
	struct NodeToolInterface
	{
		typedef QPair<QString, QDomDocument> SavedContent;
		QString name; //!< name of the interface taken from the registrar
		
		virtual ~NodeToolInterface() {}
		
		virtual void aboutToLoad() {};
		virtual void loadFromDom(const QDomDocument& content, bool fromFile) {};
		virtual QDomDocument saveToDom() const { return QDomDocument(); } 
		SavedContent getSaved() const { return SavedContent(name, saveToDom()); }
		virtual void codeChangedInEditor() {}
		
		virtual QWidget* createMenuEntry() = 0;
		virtual void closeAsSoonAsPossible() = 0;
	};
	
	//! A list of NodeToolInterface pointers
	struct NodeToolInterfaces: std::vector<NodeToolInterface*>
	{
		bool containsNamed(const QString& name) const;
		NodeToolInterface* getNamed(const QString& name) const;
	};

	/*@}*/
} // namespace Aseba

#endif
