/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2013:
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

#ifndef __PLAYGROUND_ENKI_GLUE_H
#define __PLAYGROUND_ENKI_GLUE_H

#include <functional>
#include <string>
#include <vector>
#include <enki/PhysicalEngine.h>
#include "../../vm/vm.h"
#include "../../common/utils/utils.h"

namespace Enki
{
	// Interface for Aseba-enabled Enki objects and their native functions
	
	//! A function that returns a world
	typedef std::function<World*()> GetWorld;
	//! A global function that returns the world
	extern GetWorld getWorld;
	
	//! Types of notifications that can be sent to the environment
	enum class EnvironmentNotificationType
	{
		DISPLAY_INFO, //!< an information to be displayed to the user
		LOG_INFO, //!< an information to be logged
		LOG_WARNING, //!< a warning to be logged
		LOG_ERROR, //!< an error to be logged
		FATAL_ERROR //!< a fatal error that should terminate the application
	};
	
	//! A vector of string
	typedef std::vector<std::string> strings;
	//! A function that notifies the environment of something happening inside a robot, takes a type, a description and a vector of arguments
	typedef std::function<void(const EnvironmentNotificationType, const std::string&, const strings&)> NotifyEnvironment;
	//! A global function that notifies the environment
	extern NotifyEnvironment notifyEnvironment;
	
	//! Helper macro to write notification sending in a convenient way
	#define SEND_NOTIFICATION(type, description, ...) \
	if (Enki::notifyEnvironment) \
		Enki::notifyEnvironment(Enki::EnvironmentNotificationType::type, description, {__VA_ARGS__});
	
	//! Return the Enki object of a given type associated with a given vm
	template<typename ObjectType>
	ObjectType *getEnkiObject(AsebaVMState *vm)
	{
		if (!getWorld())
			return nullptr;
		World* world(getWorld());
		if (!world)
			return nullptr;
		for (World::ObjectsIterator objectIt = world->objects.begin(); objectIt != world->objects.end(); ++objectIt)
		{
			ObjectType *enkiObject = dynamic_cast<ObjectType*>(*objectIt);
			if (enkiObject && (&(enkiObject->vm) == vm))
				return enkiObject;
		}
		return nullptr;
	}
	
} // namespace Enki

#endif // __PLAYGROUND_ENKI_GLUE_H
