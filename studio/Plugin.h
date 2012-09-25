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
#include "../compiler/compiler.h"

class QMenu;

namespace Dashel
{
	class Stream;
}

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class NodeTab;
	class Target;
	class TargetVariablesModel;
	class MainWindow;
	
	//! To access private members of MainWindow and its children, a plugin must inherit from this class
	struct InvasivePlugin
	{
		NodeTab* nodeTab;
		MainWindow *mainWindow;
		
		InvasivePlugin(NodeTab* nodeTab);
		virtual ~InvasivePlugin() {}
		
		Dashel::Stream* getDashelStream();
		Target * getTarget();
		unsigned getNodeId();
		void displayCode(QList<QString> code);
		void loadNrun();
		void stop();
		TargetVariablesModel * getVariablesModel();
		void setVariableValues(unsigned, const VariablesDataVector &);
		bool saveFile(bool as=false);
		void openFile();
	};
	
	//! A tool that is specific to a node
	struct NodeToolInterface
	{
		typedef QPair<QString, QDomDocument> SavedContent;
		QString name; //!< name of the interface taken from the registrar
		
		virtual ~NodeToolInterface() {}
		
		virtual void loadFromDom(const QDomDocument& content) {};
		virtual QDomDocument saveToDom() const { return QDomDocument(); } 
		SavedContent getSaved() const { return SavedContent(name, saveToDom()); }
		
		virtual QWidget* createMenuEntry() = 0;
		virtual void closeAsSoonAsPossible() = 0;
		//! wether this tool should survive tab destruction, useful for flashers for instance
		virtual bool surviveTabDestruction() const { return false; }
	};
	
	//! A list of NodeToolInterface pointers
	struct NodeToolInterfaces: std::vector<NodeToolInterface*>
	{
		bool containsNamed(const QString& name) const;
		NodeToolInterface* getNamed(const QString& name) const;
	};
	
	//! Node tools are available per product id
	struct NodeToolRegistrar
	{
		//! A product ID from Aseba
		typedef int ProductId;
		//! A list of product IDs
		typedef QList<ProductId> ProductIds;
		//! A function which creates an instance of a node tool
		typedef NodeToolInterface* (*CreatorFunc)(NodeTab* node);
		
		void reg(const QString& name, const ProductIds& pid, const CreatorFunc func);
		
		void reg(const QString& name, const ProductId pid, const CreatorFunc func);
		
		void update(const ProductId pid, NodeTab* node, NodeToolInterfaces& tools) const;
		
		void update(const QString& name, NodeTab* node, NodeToolInterfaces& tools) const;
		
		void dump(std::ostream &stream);
		
	protected:
		typedef QPair<CreatorFunc, QString> CreatorFuncNamePair;
		typedef QMultiMap<ProductId, CreatorFuncNamePair> PidCreatorMap;
		PidCreatorMap pidCreators;
		typedef QMap<QString, CreatorFunc> NamedCreatorMap;
		NamedCreatorMap namedCreators;
	};
	
	struct NodeToolRegistrer: NodeToolRegistrar
	{
		NodeToolRegistrer();
	};
	
	static NodeToolRegistrer nodeToolRegistrer;

	/*@}*/
}; // Aseba

#endif
