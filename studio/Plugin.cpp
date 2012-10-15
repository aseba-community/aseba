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

#include <QMenu>
#include <QtDebug>

#include "Plugin.h"
#include "DashelTarget.h"
#include "MainWindow.h"
#include "../utils/utils.h"
#include "../common/productids.h"
#include "NamedValuesVectorModel.h"

// plugins

#include "plugins/VariablesViewPlugin.h"
#include "plugins/ThymioBootloader.h"
#include "plugins/ThymioVPL/ThymioVisualProgramming.h"

namespace Aseba
{	
	/** \addtogroup studio */
	/*@{*/

	InvasivePlugin::InvasivePlugin(NodeTab* nodeTab) : nodeTab(nodeTab) { mainWindow = nodeTab->mainWindow; };

	Dashel::Stream* InvasivePlugin::getDashelStream()
	{
		DashelTarget* target = 
			polymorphic_downcast<DashelTarget*>(nodeTab->target);
		return target->dashelInterface.stream;
	}

	Target* InvasivePlugin::getTarget()
	{
		return nodeTab->target;
	}
	
	unsigned InvasivePlugin::getNodeId()
	{
		return nodeTab->id;
	}
	
	void InvasivePlugin::displayCode(QList<QString> code, int line)
	{
		nodeTab->displayCode(code, line);
	}
	
	void InvasivePlugin::loadNrun()
	{
		nodeTab->loadClicked();
		nodeTab->target->run(nodeTab->id);
	}

	void InvasivePlugin::stop()
	{
		nodeTab->target->stop(nodeTab->id);
	}
	
	bool InvasivePlugin::saveFile(bool as)
	{
		if( as )
			return mainWindow->saveFile();
		
		return mainWindow->save();
	}

	void InvasivePlugin::openFile()
	{
		mainWindow->openFile();
	}
	
	bool InvasivePlugin::newFile()
	{
		return mainWindow->newFile();
	}
	
	TargetVariablesModel * InvasivePlugin::getVariablesModel()
	{
		return nodeTab->vmMemoryModel;
	}
	
	void InvasivePlugin::setVariableValues(unsigned addr, const VariablesDataVector &data)
	{
		nodeTab->setVariableValues(addr, data);
	}
	
	//! Create an instance of C, passing node to its constructor
	template<typename C>
	NodeToolInterface* createInstance(NodeTab* node)
	{
		return new C(node);
	}
	
	//! Returns whether there is a tool named name in the list
	bool NodeToolInterfaces::containsNamed(const QString& name) const
	{
		for (const_iterator it(begin()); it != end(); ++it)
			if ((*it)->name == name)
				return true;
		return false;
	}
	
	//! Returns a tool of a given name
	NodeToolInterface* NodeToolInterfaces::getNamed(const QString& name) const
	{
		for (const_iterator it(begin()); it != end(); ++it)
			if ((*it)->name == name)
				return *it;
		return 0;
	}
	
	//! Register a class with a name/a list of PIDs by storing a pointer to its constructor
	void NodeToolRegistrar::reg(const QString& name, const ProductIds& pid, const CreatorFunc func)
	{
		for (ProductIds::const_iterator it(pid.begin()); it != pid.end(); ++it)
			reg(name, *it, func);
	}
	
	//! Register a class with a name/a pid by storing a pointer to its constructor
	void NodeToolRegistrar::reg(const QString& name, const ProductId pid, const CreatorFunc func)
	{
		pidCreators.insert(pid, CreatorFuncNamePair(func, name));
		namedCreators[name] = func;
	}
	
	//! Update tool list with onse for given pid
	void NodeToolRegistrar::update(const ProductId pid, NodeTab* node, NodeToolInterfaces& tools) const
	{
		typedef PidCreatorMap::const_iterator ConstIt;
		ConstIt it(pidCreators.find(pid));
		while (it != pidCreators.end() && it.key() == pid)
		{
			const QString& name(it.value().second);
			if (!tools.containsNamed(name))
			{
				const CreatorFunc& creatorFunc(it.value().first);
				tools.push_back(creatorFunc(node));
				tools.back()->name = name;
			}
			++it;
		}
	}
	
	//! Create tool list with one of a given name
	void NodeToolRegistrar::update(const QString& name, NodeTab* node, NodeToolInterfaces& tools) const
	{
		if (tools.containsNamed(name))
			return;
		typedef NamedCreatorMap::const_iterator ConstIt;
		ConstIt it(namedCreators.find(name));
		if (it == namedCreators.end())
			return;
		const CreatorFunc& creatorFunc(it.value());
		tools.push_back(creatorFunc(node));
		tools.back()->name = name;
	}
	
	//! Print the list of registered classes to stream
	void NodeToolRegistrar::dump(std::ostream &stream)
	{
		for (PidCreatorMap::const_iterator it = pidCreators.begin(); it != pidCreators.end(); ++it)
			stream << "- " << it->first << "\n";
	}
	
	NodeToolRegistrer::NodeToolRegistrer()
	{
		ProductIds linearCameraPids;
		linearCameraPids << ASEBA_PID_CHALLENGE << ASEBA_PID_PLAYGROUND << ASEBA_PID_EPUCK << ASEBA_PID_SMARTROB;
		reg("LinearCameraViewPlugin", linearCameraPids, &createInstance<LinearCameraViewPlugin>);
		reg("ThymioBootloaderDialog", ASEBA_PID_THYMIO2, &createInstance<ThymioBootloaderDialog>);
		reg("ThymioVisualProgramming", ASEBA_PID_THYMIO2, &createInstance<ThymioVisualProgramming>);
	}
	
	/*@}*/
}; // Aseba
