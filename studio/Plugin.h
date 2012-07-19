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
#include <dashel/dashel.h>
#include <stdexcept>
#include <iostream>
#include <set>

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
	
	//! To access private members of MainWindow and its children, a plugin must inherit from this class
	struct InvasivePlugin
	{
		NodeTab* nodeTab;
		
		InvasivePlugin(NodeTab* nodeTab);
		virtual ~InvasivePlugin() {}
		
		Dashel::Stream* getDashelStream();
		Target * getTarget();
		unsigned getNodeId();
		void displayCode(QList<QString> code);
		void loadNrun();
		void stop();
		TargetVariablesModel * getVariablesModel();
	};
	
	//! A tool that is specific to a node
	struct NodeToolInterface
	{
		virtual ~NodeToolInterface() {}
		
	public:
		virtual QWidget* createMenuEntry() = 0;
		virtual void closeAsSoonAsPossible() = 0;
		//! wether this tool should survive tab destruction, useful for flashers for instance
		virtual bool surviveTabDestruction() const { return false; }
		
	};
	typedef std::vector<NodeToolInterface*> NodeToolInterfaces;
	
	//! Node tools are available per product id
	struct NodeToolRegistrar
	{
		//! A product ID from Aseba
		typedef int ProductId;
		//! A function which creates an instance of a node tool
		typedef NodeToolInterface* (*CreatorFunc)(NodeTab* node);
		
		//! Register a class by storing a pointer to its constructor
		void reg(const ProductId pid, const CreatorFunc func);
		
		//! Create a tool for a given name
		NodeToolInterfaces create(const ProductId pid, NodeTab* node);
		
		//! Print the list of registered classes to stream
		void dump(std::ostream &stream);
		
	protected:
		typedef std::multimap<ProductId, CreatorFunc> CreatorMap;
		CreatorMap creators;
	};
	
	struct NodeToolRegistrer: NodeToolRegistrar
	{
		NodeToolRegistrer();
	};
	
	static NodeToolRegistrer nodeToolRegistrer;

	/*@}*/
}; // Aseba

#endif
