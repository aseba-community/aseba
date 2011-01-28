/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2010:
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

#include "Plugin.h"
#include "DashelTarget.h"
#include "MainWindow.h"
#include "../utils/utils.h"
#include "../common/productids.h"

// plugins

#include "plugins/VariablesViewPlugin.h"
#include "plugins/ThymioBootloader.h"

namespace Aseba
{	
	/** \addtogroup studio */
	/*@{*/

	InvasivePlugin::InvasivePlugin(NodeTab* nodeTab) : nodeTab(nodeTab) {};

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
	
	TargetVariablesModel * InvasivePlugin::getVariablesModel()
	{
		return nodeTab->vmMemoryModel;
	}
	
	//! Create an instance of C, passing node to its constructor
	template<typename C>
	NodeToolInterface* createInstance(NodeTab* node)
	{
		return new C(node);
	}
	
	void NodeToolRegistrar::reg(const ProductId pid, const CreatorFunc func)
	{
		creators.insert(std::pair<ProductId, CreatorFunc>(pid, func));
	}
	
	NodeToolInterfaces NodeToolRegistrar::create(const ProductId pid, NodeTab* node)
	{
		typedef CreatorMap::const_iterator ConstIt;
		typedef std::pair<ConstIt, ConstIt> ConstItPair;
		ConstItPair range(creators.equal_range(pid));
		if (range.first == range.second)
		{
			std::cerr << "No tool registered for product id " << pid << ". Known ones are:\n";
			dump(std::cerr);
			throw std::runtime_error("Trying to instanciate unknown element from NodeToolRegistrar");
		}
		NodeToolInterfaces interfaces;
		for (ConstIt it(range.first); it != range.second; ++it)
		{
			CreatorFunc creatorFunc(it->second);
			interfaces.push_back(creatorFunc(node));
		}
		return interfaces;
	}
	
	void NodeToolRegistrar::dump(std::ostream &stream)
	{
		for (CreatorMap::const_iterator it = creators.begin(); it != creators.end(); ++it)
			stream << "- " << it->first << "\n";
	}
	
	NodeToolRegistrer::NodeToolRegistrer()
	{
		reg(ASEBA_PID_CHALLENGE, &createInstance<LinearCameraViewPlugin>);
		reg(ASEBA_PID_PLAYGROUND, &createInstance<LinearCameraViewPlugin>);
		reg(ASEBA_PID_EPUCK, &createInstance<LinearCameraViewPlugin>);
		reg(ASEBA_PID_SMARTROB, &createInstance<LinearCameraViewPlugin>);
		
		reg(ASEBA_PID_THYMIO2, &createInstance<ThymioBootloaderDialog>);
	}
	
	/*@}*/
}; // Aseba