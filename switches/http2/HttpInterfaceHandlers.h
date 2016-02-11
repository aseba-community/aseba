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

#ifndef ASEBA_HTTP_INTERFACE_HANDLERS
#define ASEBA_HTTP_INTERFACE_HANDLERS

#include "HttpHandler.h"

namespace Aseba { namespace Http
{
	/**
	 * Handles OPTIONS requests for CORS preflighting requests
	 */
	class OptionsHandler : public HttpHandler
	{
		public:
			OptionsHandler();
			virtual ~OptionsHandler();

			virtual bool checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const;
			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);
	};

	class NodesHandler : public HierarchicalTokenHttpHandler, public InterfaceHttpHandler
	{
		public:
			NodesHandler(HttpInterface *interface);
			virtual ~NodesHandler();
	};

	class EventsHandler : public TokenHttpHandler, public InterfaceHttpHandler
	{
		public:
			EventsHandler(HttpInterface *interface);
			virtual ~EventsHandler();

			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);
	};

	class ResetHandler : public TokenHttpHandler, public InterfaceHttpHandler
	{
		public:
			ResetHandler(HttpInterface *interface);
			virtual ~ResetHandler();

			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);
	};

	class LoadHandler : public virtual InterfaceHttpHandler
	{
		public:
			LoadHandler(HttpInterface *interface);
			virtual ~LoadHandler();

			virtual bool checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const;
			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);
	};

	class NodeInfoHandler : public virtual InterfaceHttpHandler
	{
		public:
			NodeInfoHandler(HttpInterface *interface);
			virtual ~NodeInfoHandler();

			virtual bool checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const;
			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);
	};

	class VariableOrEventHandler : public virtual WildcardHttpHandler, public virtual InterfaceHttpHandler
	{
		public:
			VariableOrEventHandler(HttpInterface *interface);
			virtual ~VariableOrEventHandler();

			virtual void handleRequest(HttpRequest *request, const std::vector<std::string>& tokens);

		private:
			void parseJsonForm(std::string content, std::vector<std::string>& values);
	};
} }

#endif
