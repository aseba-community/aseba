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

#include "HttpDispatcher.h"
#include "HttpStatus.h"
#include "HttpHandlers.h"
#include "../Globals.h"
#include "../Switch.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;
	
	//! Hash the key, shift up nodeId 16 bits and xor with pos
	size_t HttpDispatcher::VariableRequestKeyHash::operator()(const VariableRequestKey& key) const
	{
		return hash<unsigned>{}(key.nodeId << 16) ^ hash<unsigned>{}(key.pos);
	}
	
	//! Register all nodes-related handlers
	void HttpDispatcher::registerNodesHandlers()
	{
		REGISTER_HANDLER(getNodesHandler, GET, { "nodes" }, { R"#json#(
			{
				"description" : "List signatures (ids, names, product ids) of connected nodes",
				"operationId" : "GET-nodes",
				"parameters" : [
					{
						"$ref" : "#/parameters/trait:querySelectEmbed:embed"
					},
					{
						"$ref" : "#/parameters/trait:querySelectFields:fields"
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get All Nodes",
						"schema" : {
							"items" : {
								"$ref" : "#/definitions/node-signature"
							},
							"type" : "array"
						}
					}
				},
				"summary" : "Get All Nodes",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(getNodesNodeHandler, GET, { "nodes", "{node}" }, { R"#json#(
			{
				"description" : "Node Description, Variables, and Program",
				"operationId" : "GET-nodes-node",
				"parameters" : [
					{
						"$ref" : "#/parameters/trait:querySelectFields:fields"
					},
					{
						"$ref" : "#/parameters/trait:querySelectEmbed:embed"
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Node",
						"schema" : {
							"properties" : {
								"description" : {
									"$ref" : "#/definitions/node-description"
								},
								"program" : {
									"$ref" : "#/definitions/compilation-result"
								},
								"variables" : {
									"$ref" : "#/definitions/variables"
								}
							},
							"required" : [
								"description",
								"variables"
							],
							"type" : "object"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Node",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(getNodesNodeDescriptionHandler, GET, { "nodes", "{node}", "description" }, { R"#json#(
			{
				"description" : "Return the #model:qnq7DXyawr5Gv8BKo (hardwired properties) of the given node.",
				"operationId" : "GET-nodes-node-description",
				"parameters" : [
					{
						"$ref" : "#/parameters/trait:querySelectFields:fields"
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Node Description",
						"schema" : {
							"$ref" : "#/definitions/node-description"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Node Description",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(putNodesNodeProgramHandler, PUT, { "nodes", "{node}", "program" }, { R"#json#(
			{
				"consumes" : [
					"text/plain"
				],
				"description" : "The request body is on content type `text/plain ` and is interpreted as  the source code of an Aseba program that must be compiled.\n \nA future extension will accept `application/json` requests containing a #model:cZHWP4fZsNXLFQJse and a #model:cFt9qnmHAwFAYXhS5.",
				"operationId" : "PUT-nodes-node-program",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Update Node Program",
						"schema" : {
							"$ref" : "#/definitions/compilation-result"
						}
					},
					"400" : {
						"description" : "Error in Update Node Program",
						"schema" : {
							"$ref" : "#/definitions/compilation-result"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Update Node Program",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(getNodesNodeProgramHandler, GET, { "nodes", "{node}", "program" }, { R"#json#(
			{
				"operationId" : "GET-nodes-node-program",
				"parameters" : [
					{
						"$ref" : "#/parameters/trait:querySelectEmbed:embed"
					},
					{
						"$ref" : "#/parameters/trait:querySelectFields:fields"
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Node Program",
						"schema" : {
							"$ref" : "#/definitions/compilation-result"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Node Program",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(getNodesNodeVariablesHandler, GET, { "nodes", "{node}", "variables" }, { R"#json#(
			{
				"operationId" : "GET-nodes-node-variables",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Node Variables",
						"schema" : {
							"$ref" : "#/definitions/variables"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Get Node Variables",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(putNodesNodeVariablesVariableHandler, PUT, { "nodes", "{node}", "variables", "{variable}" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"description" : "Assign a values vector to a variable. The variable may be a native variable or a variable defined in the program.\n\nError 404 is returned for an unknown node, or an unknown variable in a known node.",
				"operationId" : "PUT-nodes-node-variables-variable",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"example" : [
								1,
								2,
								3
							],
							"items" : {
								"type" : "integer",
								"maximum" : 32767,
								"minimum" : -32768
							},
							"type" : [
								"integer",
								"array"
							],
							"maximum" : 32767,
							"minimum" : -32768
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"204" : {
						"description" : "Update Node Variable",
						"schema" : {
							"type" : "null"
						}
					},
					"400" : {
						"description" : "Invalid values vector",
						"schema" : {
							"type" : "string"
						}
					},
					"404" : {
						"description" : "Unknown node or unknown variable",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Update Node Variable",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
		REGISTER_HANDLER(getNodesNodeVariablesVariableHandler, GET, { "nodes", "{node}", "variables", "{variable}" }, { R"#json#(
			{
				"description" : "Read values vector from a variable.\nThe variable may be a native variable or a variable defined in the program. \n\nError 404 is returned for an unknown node, or an unknown variable in a known node.",
				"operationId" : "GET-nodes-node-variables-variable",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Read Node Variable",
						"schema" : {
							"items" : {
								"type" : "integer"
							},
							"type" : "array"
						}
					},
					"404" : {
						"description" : "Unknown node or unknown variable",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Read Node Variable",
				"tags" : [
					"Nodes"
				]
			})#json#"_json });
	}
	
	// local helpers
	
	template <class Container, class Function>
	typename std::vector<typename std::result_of<Function(const typename Container::value_type&)>::type> apply (const Container &cont, Function fun)
	{
		std::vector<typename std::result_of<Function(const typename Container::value_type&)>::type> ret;
		ret.reserve(cont.size());
		for (const auto &v : cont)
		{
			ret.push_back(fun(v));
		}
		return ret;
	}
	
	// handlers

	//! handler for GET /nodes -> application/json
	void HttpDispatcher::getNodesHandler(HandlerContext& context)
	{
		json response(json::array());
		for (const auto& nodeKV: context.asebaSwitch->nodes)
		{
			const unsigned nodeId(nodeKV.first);
			const NodesManager::Node* node(nodeKV.second.get());
			if (node->isComplete() && node->connected)
			{
				response.push_back({
					{ "id", nodeId },
					{ "name", WStringToUTF8(node->name) },
					{ "pid", -1 } // TODO: get a real pid, or remove from spec
				});
			}
		}
		HttpResponse::fromJSON(response).send(context.stream);
	}

	//! handler for GET /nodes/{node} -> application/json
	void HttpDispatcher::getNodesNodeHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;
		
		// details of node
		json response;
		response["description"] = jsonNodeDescription(node);
		response["program"] = jsonNodeProgram(node);
		response["variables"] = jsonNodeVariables(node);
		HttpResponse::fromJSON(response).send(context.stream);
	}

	//! handler for GET /nodes/{node}/description -> application/json
	void HttpDispatcher::getNodesNodeDescriptionHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;
		
		HttpResponse::fromJSON(jsonNodeDescription(node)).send(context.stream);
	}

	//! handler for PUT /nodes/{node}/program -> application/json
	void HttpDispatcher::putNodesNodeProgramHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;
		
		// copy code and compile
		node.second->code = UTF8ToWString(string(context.request.content.begin(), context.request.content.end()));
		node.second->compile(context.asebaSwitch->commonDefinitions);
		
		if (node.second->success)
			HttpResponse::fromJSON(jsonNodeProgram(node)).send(context.stream);
		else
			HttpResponse::fromJSON(jsonNodeProgram(node), HttpStatus::BAD_REQUEST).send(context.stream);
	}

	//! handler for GET /nodes/{node}/program -> application/json
	void HttpDispatcher::getNodesNodeProgramHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;
		
		HttpResponse::fromJSON(jsonNodeProgram(node)).send(context.stream);
	}

	//! handler for GET /nodes/{node}/variables -> application/json
	void HttpDispatcher::getNodesNodeVariablesHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;
		
		HttpResponse::fromJSON(jsonNodeVariables(node)).send(context.stream);
	}

	//! handler for PUT /nodes/{node}/variables/{variable} -> application/json
	void HttpDispatcher::putNodesNodeVariablesVariableHandler(HandlerContext& context)
	{
		//unsigned nodeId, pos, size;
		const auto variable(findVariable(context));
		if (!variable.found)
			return;
		
		if (context.parsedContent.is_number())
		{
			// we have a single integer
			const sint16 value(context.parsedContent.get<int>());
			context.asebaSwitch->sendMessage(SetVariables(variable.nodeId, variable.pos, { value } ));
		}
		else
		{
			// we have a vector of integers
			const size_t count(context.parsedContent.size());
			if (count > variable.size)
			{
				HttpResponse::fromPlainString(FormatableString("Trying to write %0 elements while variable has only %1 elements").arg(count).arg(variable.size), HttpStatus::BAD_REQUEST).send(context.stream);
				return;
			}
			
			const vector<sint16> data = context.parsedContent;
			context.asebaSwitch->sendMessage(SetVariables(variable.nodeId, variable.pos, data));
		}
		// send success
		HttpResponse::fromStatus(HttpStatus::NO_CONTENT).send(context.stream);
	}

	//! handler for GET /nodes/{node}/variables/{variable} -> application/json
	void HttpDispatcher::getNodesNodeVariablesVariableHandler(HandlerContext& context)
	{
		//unsigned nodeId, pos, size;
		const auto variable(findVariable(context));
		if (!variable.found)
			return;

		// request variable from node and keep track of request
		context.asebaSwitch->sendMessage(GetVariables(variable.nodeId, variable.pos, variable.size));
		pendingReads.emplace(VariableRequestKey{ variable.nodeId, variable.pos }, context.stream);
		
		// do not send any answer yet
	}
	
	// support
	
	//! Return the description of a node in JSON
	json HttpDispatcher::jsonNodeDescription(const NodeEntry& node) const
	{
		return {
			{ "id", node.first },
			{ "name", WStringToUTF8(node.second->name) },
			{ "protocolVersion", node.second->protocolVersion },
			{ "vmProperties", {
				{ "bytecodeSize", node.second->bytecodeSize },
				{ "stackSize", node.second->stackSize },
				{ "variablesSize", node.second->variablesSize }
			}},
			{ "nativeEvents", jsonNodeNativeEvents(node) },
			{ "nativeFunctions", jsonNodeNativeFunctions(node) },
			{ "nativeVariables", jsonNodeNativeVariables(node) }
		};
	}
	
	json HttpDispatcher::jsonNodeNativeEvents(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto& event: node.second->localEvents)
			response.push_back( { {"name", WStringToUTF8(event.name) }, { "description", WStringToUTF8(event.description) } } );
		return response;
	}
	
	json HttpDispatcher::jsonNodeNativeFunctions(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto& function: node.second->nativeFunctions)
		{
			json params(json::array());
			for (auto& param: function.parameters)
			{
				params.push_back({
					{ "name", WStringToUTF8(param.name) },
					{ "size", param.size }
				});
			}
			response.push_back( {
				{ "name", WStringToUTF8(function.name) },
				{ "description", WStringToUTF8(function.description) },
				{ "parameters", params }
			} );
		}
		return response;
	}
	
	json HttpDispatcher::jsonNodeNativeVariables(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto& variable: node.second->namedVariables)
			response.push_back( { {"name", WStringToUTF8(variable.name) }, { "size", variable.size } } );
		return response;
	}
	
	//! Return the compiled program of a node in JSON
	json HttpDispatcher::jsonNodeProgram(const NodeEntry& node) const
	{
		return {
			{ "success", node.second->success },
			{ "error", {
				{ "message", WStringToUTF8(node.second->error.message) },
				{ "pos", node.second->error.pos.character },
				{ "rowcol", { node.second->error.pos.row, node.second->error.pos.column } }
			}},
			{ "bytecode", apply(node.second->bytecode, [](const BytecodeElement& element) { return element.bytecode; } ) },
			{ "symbols", jsonNodeSymbolTable(node) }
		};
	}
	
	//! Return the symbol table of a node in JSON
	json HttpDispatcher::jsonNodeSymbolTable(const NodeEntry& node) const
	{
		return {
			{ "allocatedVariableCount", node.second->allocatedVariablesCount },
			{ "maxStackDepth", node.second->bytecode.maxStackDepth },
			{ "callDepth", node.second->bytecode.callDepth },
			{ "lastLine", node.second->bytecode.lastLine },
			{ "eventsVector", jsonNodeSymbolEventsVector(node) },
			{ "variables" , jsonNodeSymbolVariables(node) },
			{ "subroutines", jsonNodeSymbolSubroutines(node) },
			{ "lineNumbers", apply(node.second->bytecode, [](const BytecodeElement& element) { return element.line; } ) },
		};
		// TODO: add back allocatedVariablesCount, maxStackDepth, callDepth, lastLine, eventsVector to spec
	}
	
	//! Return the events vector part of the symbol table of a node in JSON
	json HttpDispatcher::jsonNodeSymbolEventsVector(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto &eventKV: node.second->bytecode.getEventAddressesToIds())
		{
			response.push_back({
				{ "address", eventKV.first },
				{ "id", eventKV.second } 
			});
		}
		return response;
	}
	
	//! Return the variables part of the symbol table of a node in JSON
	json HttpDispatcher::jsonNodeSymbolVariables(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto &variableKV: node.second->variables)
		{
			response.push_back({
				{ "name", WStringToUTF8(variableKV.first) },
				{ "address", variableKV.second.first },
				{ "size", variableKV.second.second }
			});
		}
		return response;
	}
	
	//! Return the subroutines part of the symbol table of a node in JSON
	json HttpDispatcher::jsonNodeSymbolSubroutines(const NodeEntry& node) const
	{
		json response(json::array());
		for (auto &subroutine: node.second->subroutineTable)
		{
			response.push_back({
				{ "name", WStringToUTF8(subroutine.name) },
				{ "address", subroutine.address },
				{ "line", subroutine.line }
			});
		}
		return response;
	}
	
	
	//! Return the variables of a node in JSON
	json HttpDispatcher::jsonNodeVariables(const NodeEntry& node) const
	{
		// TODO: "#/definitions/variables"
		// FIXME: when and how to update
		
		return {};
	}
	
	//! Return true and fills nodeId, pos and size if a variable is found, 
	HttpDispatcher::VariableSearchResult HttpDispatcher::findVariable(HandlerContext& context) const
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return { false };
		
		// Get variable name from template
		const auto templateIt(context.filledPathTemplates.find("variable"));
		assert(templateIt != context.filledPathTemplates.end());
		const wstring variableName(UTF8ToWString(templateIt->second));
		
		// Find variable
		auto variableIt(node.second->variables.find(variableName));
		if (variableIt == node.second->variables.end())
		{
			HttpResponse::fromPlainString(FormatableString("Node %0 does not contain variable %1").arg(node.first).arg(templateIt->second), HttpStatus::NOT_FOUND).send(context.stream);
			return { false };
		}
		
		return { true, node.first, variableIt->second.first, variableIt->second.second };
	}
	
	//! Return a node from the request, or produce an error and return null if the node is not found
	HttpDispatcher::NodeEntry HttpDispatcher::findNode(HandlerContext& context) const
	{
		const auto templateIt(context.filledPathTemplates.find("node"));
		assert(templateIt != context.filledPathTemplates.end());
		try
		{
			const unsigned nodeId(stoul(templateIt->second));
		
			// retrieve node
			const auto nodeIt(context.asebaSwitch->nodes.find(nodeId));
			if (nodeIt == context.asebaSwitch->nodes.end())
			{
				HttpResponse::fromPlainString(FormatableString("Node %0 does not exist or is not available").arg(nodeId), HttpStatus::NOT_FOUND).send(context.stream);
				return NodeEntry(0, nullptr);
			}
			return NodeEntry(nodeIt->first, polymorphic_downcast<Switch::NodeWithProgram*>(nodeIt->second.get()));
		}
		catch (const invalid_argument& e)
		{
			HttpResponse::fromPlainString(FormatableString("Node parameter %0 is not an integer node identifier").arg(templateIt->second), HttpStatus::NOT_FOUND).send(context.stream);
			return NodeEntry(0, nullptr);
		}
	}
	
} // namespace Aseba
