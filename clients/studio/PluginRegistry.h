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

#ifndef PLUGIN_REGISTRY_H
#define PLUGIN_REGISTRY_H

#include <QLabel>
#include <QDomDocument>
#include <QMultiMap>
#include <dashel/dashel.h>
#include <stdexcept>
#include <iostream>
#include <set>
#include "../../compiler/compiler.h"

namespace Aseba
{
	/** \addtogroup studio */
	/*@{*/
	
	class NodeTab;
	class Target;
	class TargetVariablesModel;
	class MainWindow;
	
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
} // namespace Aseba

#endif
