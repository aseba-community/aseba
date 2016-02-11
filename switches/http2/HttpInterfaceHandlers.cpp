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

#include <algorithm>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include "../../common/utils/utils.h"
#include "HttpInterface.h"
#include "HttpInterfaceHandlers.h"

using Aseba::Http::EventsHandler;
using Aseba::Http::HttpDashelTarget;
using Aseba::Http::HttpInterface;
using Aseba::Http::InterfaceHttpHandler;
using Aseba::Http::LoadHandler;
using Aseba::Http::NodeInfoHandler;
using Aseba::Http::NodesHandler;
using Aseba::Http::OptionsHandler;
using Aseba::Http::ResetHandler;
using Aseba::Http::VariableOrEventHandler;
using std::cerr;
using std::endl;
using std::map;
using std::pair;
using std::set;
using std::string;
using std::stringstream;
using std::vector;

OptionsHandler::OptionsHandler()
{

}

OptionsHandler::~OptionsHandler()
{

}

bool OptionsHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return request->getMethod() == "OPTIONS";
}

void OptionsHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	// we only use OPTIONS for CORS preflighting, so just reply to the client that everything is okay
	request->respond().setHeader("Access-Control-Allow-Origin", "*");
	request->respond().setHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
	request->respond().setHeader("Access-Control-Allow-Headers", "Content-Type");
}

NodesHandler::NodesHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("nodes");

	addSubhandler(new NodeInfoHandler(interface));
	addSubhandler(new VariableOrEventHandler(interface));
}

NodesHandler::~NodesHandler()
{

}

EventsHandler::EventsHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("events");
}

EventsHandler::~EventsHandler()
{

}

void EventsHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	if(tokens.size() == 1) {
		getInterface()->addEventSubscription(request, "*");
	} else {
		for(vector<string>::const_iterator i = tokens.begin() + 1; i != tokens.end(); ++i) {
			getInterface()->addEventSubscription(request, *i);
		}
	}

	request->respond().setHeader("Content-Type", "text/event-stream");
	request->respond().setHeader("Cache-Control", "no-cache");
	request->respond().setHeader("Connection", "keep-alive");

	try {
		request->respond().send(); // send header immediately
		request->setBlocking(true); // connection must stay open!
	} catch(Dashel::DashelException e) {
		if(getInterface()->isVerbose()) {
			cerr << request << " Failed to immediately send header back for events request: " << e.what() << endl;
		}
	}
}

ResetHandler::ResetHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{
	addToken("reset");
	addToken("reset_all");
}

ResetHandler::~ResetHandler()
{

}

void ResetHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	map<Dashel::Stream *, HttpDashelTarget *>& targets = getInterface()->getTargets();

	map<Dashel::Stream *, HttpDashelTarget *>::iterator end = targets.begin();
	for(std::map<Dashel::Stream *, HttpDashelTarget *>::iterator iter = targets.begin(); iter != end; ++iter) {
		Dashel::Stream *stream = iter->first;
		HttpDashelTarget *target = iter->second;
		const map<unsigned, HttpDashelTarget::Node>& nodes = target->getNodes();

		map<unsigned, HttpDashelTarget::Node>::const_iterator nodesEnd = nodes.end();
		for(map<unsigned, HttpDashelTarget::Node>::const_iterator nodesIter = nodes.begin(); nodesIter != nodesEnd; ++nodesIter) {
			const HttpDashelTarget::Node& node = nodesIter->second;

			try {
				Reset(node.localId).serialize(stream); // reset node
				stream->flush();
				Run(node.localId).serialize(stream); // re-run node
				stream->flush();
			} catch(Dashel::DashelException e) {
				if(getInterface()->isVerbose()) {
					cerr << request << " Failed to reset and re-run node: " << e.what() << endl;
				}
				return;
			}

			if(node.name.find("thymio-II") == 0) { // Special case for Thymio-II. Should we instead just check whether motor.*.target exists?
				vector<string> tokens;
				tokens.push_back("motor.left.target");
				tokens.push_back("0");
				if(!target->sendSetVariable(node.globalId, tokens)) {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
					request->respond().setContent("Failed to set Thymio left motor target");
					return;
				}

				tokens[0] = "motor.right.target";
				if(!target->sendSetVariable(node.globalId, tokens)) {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
					request->respond().setContent("Failed to set Thymio left motor target");
					return;
				}
			}

			size_t eventPos;
			if(getInterface()->getProgram().getCommonDefinitions().events.contains(UTF8ToWString("reset"), &eventPos)) {
				vector<string> data;
				data.push_back("reset");

				if(!getInterface()->sendEvent(data)) {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
					request->respond().setContent("Failed to send reset event");
					return;
				}
			}
		}
	}

	request->respond();
}

LoadHandler::LoadHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

LoadHandler::~LoadHandler()
{

}

bool LoadHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return request->getMethod() == "PUT";
}

void LoadHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	if(request->getContent().find("file=") == 0) {
		string xml(request->getContent().c_str() + 5, request->getContent().size() - 5);
		AeslProgram program(xml.c_str(), xml.size());

		if(program.isLoaded()) {
			getInterface()->setProgram(program);

			std::string errorString;
			if(getInterface()->runProgram(errorString)) {
				request->respond();
			} else {
				request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
				request->respond().setContent(errorString);
			}
		} else {
			request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
			request->respond().setContent("Failed to parse provided AESL file");
		}
	} else {
		request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
		request->respond().setContent("No file= payload present");
	}
}

NodeInfoHandler::NodeInfoHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

NodeInfoHandler::~NodeInfoHandler()
{

}

bool NodeInfoHandler::checkIfResponsible(HttpRequest *request, const std::vector<std::string>& tokens) const
{
	return tokens.size() <= 1;
}

void NodeInfoHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	int n = (int) tokens.size();

	vector<string> parts;

	if(n == 0) { // list all nodes
		std::map<Dashel::Stream *, HttpDashelTarget *>& targets = getInterface()->getTargets();
		std::map<Dashel::Stream *, HttpDashelTarget *>::iterator end = targets.end();
		for(std::map<Dashel::Stream *, HttpDashelTarget *>::iterator iter = targets.begin(); iter != end; ++iter) {
			HttpDashelTarget *target = iter->second;

			const std::map<unsigned, HttpDashelTarget::Node>& nodes = target->getNodes();
			std::map<unsigned, HttpDashelTarget::Node>::const_iterator nodesEnd = nodes.end();
			for(std::map<unsigned, HttpDashelTarget::Node>::const_iterator nodesIter = nodes.begin(); nodesIter != nodesEnd; ++nodesIter) {
				const HttpDashelTarget::Node& node = nodesIter->second;

				bool ok;
				const TargetDescription *description = target->getDescription(node.localId, &ok);

				if(ok) {
					std::stringstream part;
					part << "{\"node\":" << node.globalId << ",\"name\":\"" << node.name << "\",\"protocolVersion\":" << description->protocolVersion << "}";
					parts.push_back(part.str());
				} else {
					stringstream errorStream;
					errorStream << "Target " << target->getAddress() << " failed to get description for node " << node.globalId << " (" << node.name << ")";

					request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
					request->respond().setContent(errorStream.str());

					if(getInterface()->isVerbose()) {
						cerr << errorStream.str() << endl;
					}
				}
			}
		}
	} else {
		const CommonDefinitions& commonDefinitions = getInterface()->getProgram().getCommonDefinitions();

		for(int i = 0; i < n; i++) {
			set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> > matchingNodes = getInterface()->getNodesByNameOrId(tokens[i]);
			set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> >::iterator end = matchingNodes.end();
			for(set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> >::iterator iter = matchingNodes.begin(); iter != end; ++iter) {
				HttpDashelTarget *target = iter->first;
				const HttpDashelTarget::Node& node = *(iter->second);

				bool ok;
				const TargetDescription *description = target->getDescription(node.localId, &ok);

				if(ok) {
					std::stringstream part;
					part << "{\"node\":" << node.globalId;
					part << ",\"name\":\"" << node.name << "\"";
					part << ",\"protocolVersion\":" << description->protocolVersion;
					part << ",\"bytecodeSize\":" << description->bytecodeSize;
					part << ",\"variablesSize\":" << description->variablesSize;
					part << ",\"stackSize\":" << description->stackSize;

					// named variables
					part << ",\"namedVariables\":{";
					bool seenNamedVariables = false;
					VariablesMap::const_iterator vmEnd = node.variablesMap.end();
					for(VariablesMap::const_iterator vmIter = node.variablesMap.begin(); vmIter != vmEnd; ++vmIter) {
						part << (vmIter == node.variablesMap.begin() ? "" : ",") << "\"" << WStringToUTF8(vmIter->first) << "\":" << vmIter->second.second;
						seenNamedVariables = true;
					}
					if(!seenNamedVariables) {
						// failsafe: if compiler hasn't found any variables, get them from the node description
						int numVariables = description->namedVariables.size();
						for(int j = 0; j < numVariables; j++) {
							part << (j == 0 ? "" : ",") << "\"" << WStringToUTF8(description->namedVariables[j].name) << "\":" << description->namedVariables[j].size;
						}
					}
					part << "}";

					// local events variables
					part << ",\"localEvents\":{";
					int numEvents = (int) description->localEvents.size();
					for(int j = 0; j < numEvents; j++) {
						part << (j == 0 ? "" : ",") << "\"" << WStringToUTF8(description->localEvents[j].name) << "\":" << "\"" << WStringToUTF8(description->localEvents[j].description) << "\"";
					}
					part << "}";

					// constants from introspection
					part << ",\"constants\":{";
					int numConstants = (int) commonDefinitions.constants.size();
					for(int j = 0; j < numConstants; j++) {
						part << (j == 0 ? "" : ",") << "\"" << WStringToUTF8(commonDefinitions.constants[j].name) << "\":" << commonDefinitions.constants[j].value;
					}
					part << "}";

					// events from introspection
					part << ",\"events\":{";
					int numCommonEvents = commonDefinitions.events.size();
					for(int j = 0; j < numCommonEvents; j++) {
						part << (j == 0 ? "" : ",") << "\"" << WStringToUTF8(commonDefinitions.events[j].name) << "\":" << commonDefinitions.events[j].value;
					}
					part << "}";

					part << "}"; // end node

					parts.push_back(part.str());
				} else {
					stringstream errorStream;
					errorStream << "Target " << target->getAddress() << " failed to get description for node " << node.globalId << " (" << node.name << ")";

					request->respond().setStatus(HttpResponse::HTTP_STATUS_INTERNAL_SERVER_ERROR);
					request->respond().setContent(errorStream.str());

					if(getInterface()->isVerbose()) {
						cerr << errorStream.str() << endl;
					}
				}
			}
		}
	}

	request->respond().setContent("[" + join(parts, ",") + "]");
}

VariableOrEventHandler::VariableOrEventHandler(HttpInterface *interface) :
	InterfaceHttpHandler(interface)
{

}

VariableOrEventHandler::~VariableOrEventHandler()
{

}

void VariableOrEventHandler::handleRequest(HttpRequest *request, const std::vector<std::string>& tokens)
{
	const CommonDefinitions& commonDefinitions = getInterface()->getProgram().getCommonDefinitions();

	int n = (int) tokens.size();

	if(n < 2) {
		request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
		return;
	}

	set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> > matchingNodes = getInterface()->getNodesByNameOrId(tokens[0]);
	if(matchingNodes.empty()) {
		request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
		return;
	}

	set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> >::iterator end = matchingNodes.end();
	for(set< pair<HttpDashelTarget *, const HttpDashelTarget::Node *> >::iterator iter = matchingNodes.begin(); iter != end; ++iter) {
		HttpDashelTarget *target = iter->first;
		const HttpDashelTarget::Node& node = *(iter->second);

		size_t eventPos;
		if(commonDefinitions.events.contains(UTF8ToWString(tokens[1]), &eventPos)) {
			// this is an event
			// arguments are tokens 1..N
			vector<string> data;
			data.push_back(tokens[1]);
			if(tokens.size() >= 3) {
				for(size_t i = 2; i < tokens.size(); ++i) {
					data.push_back((tokens[i].c_str()));
				}
			} else if(request->getMethod().find("POST") == 0) {
				// Parse POST form data
				parseJsonForm(std::string(request->getContent(), request->getContent().size()), data);
			}

			if(target->sendEvent(data)) {
				request->respond(); // or perhaps {"return_value":null,"cmd":"sendEvent","name":nodeName}?
			} else {
				request->respond().setStatus(HttpResponse::HTTP_STATUS_NOT_FOUND);
			}
		} else if(request->getMethod().find("POST") == 0 || tokens.size() >= 3) { // set variable value
			vector<string> values;
			if(tokens.size() >= 3) {
				values.assign(tokens.begin() + 1, tokens.end());
			} else {
				// Parse POST form data
				values.push_back(tokens[1]);
				parseJsonForm(request->getContent(), values);
			}

			if(!values.empty()) {
				if(target->sendSetVariable(node.globalId, values)) {
					request->respond();
				} else {
					request->respond().setStatus(HttpResponse::HTTP_STATUS_NOT_FOUND);
				}
			} else {
				request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
			}
		} else { // get variable value
			vector<string> values;
			values.assign(tokens.begin() + 1, tokens.begin() + 2);

			if(!target->sendGetVariables(node.globalId, values, request)) {
				request->respond().setStatus(HttpResponse::HTTP_STATUS_BAD_REQUEST);
			}
		}
	}
}

void VariableOrEventHandler::parseJsonForm(std::string content, std::vector<std::string>& values)
{
	std::string buffer = content;
	buffer.erase(std::remove_if(buffer.begin(), buffer.end(), ::isspace), buffer.end());
	std::stringstream ss(buffer);

	if(ss.get() == '[') {
		int i;
		while(ss >> i) {
			// values.push_back(std::to_string(short(i)));
			std::stringstream valss;
			valss << short(i);
			values.push_back(valss.str());
			if(ss.peek() == ']') break;
			else if(ss.peek() == ',') ss.ignore();
		}
		if(ss.get() != ']') values.erase(values.begin(), values.end());
	}

	return;
}
