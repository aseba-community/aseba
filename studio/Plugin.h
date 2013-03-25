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

#ifndef PLUGIN_H
#define PLUGIN_H

#include <QLabel>
#include <QDomDocument>
#include <QMultiMap>
#include <dashel/dashel.h>
#include <stdexcept>
#include <iostream>
#include <set>
#include <memory>
#include "../compiler/compiler.h"


namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class Target;
	class TargetVariablesModel;
	
	// TODO: constify DevelopmentEnvironmentInterface
	
	//! To access private members of the development environment (MainWindow and its children), a plugin must use the method of a subclass of this class
	struct DevelopmentEnvironmentInterface
	{
		virtual ~DevelopmentEnvironmentInterface() {}
		virtual Target * getTarget() = 0;
		virtual unsigned getNodeId() = 0;
		virtual void displayCode(const QList<QString>& code, int line) = 0;
		virtual void loadNrun() = 0;
		virtual void stop() = 0;
		virtual TargetVariablesModel * getVariablesModel() = 0;
		virtual void setVariableValues(unsigned, const VariablesDataVector &) = 0;
		virtual bool saveFile(bool as=false) = 0;
		virtual void openFile() = 0;
		virtual bool newFile() = 0;
	};
	
	//! A tool that is specific to a node
	struct NodeToolInterface
	{
		typedef QPair<QString, QDomDocument> SavedContent;
		QString name; //!< name of the interface taken from the registrar
		
		virtual ~NodeToolInterface() {}
		
		virtual void loadFromDom(const QDomDocument& content, bool fromFile) {};
		virtual QDomDocument saveToDom() const { return QDomDocument(); } 
		SavedContent getSaved() const { return SavedContent(name, saveToDom()); }
		
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
}; // Aseba

#endif
