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
#include "../../../vm/vm.h"

namespace Aseba
{
	using namespace std;
	using namespace Dashel;

	// FIXME: this is currently a low-level interface (bytecode index),
	// probably a high-level line-based makes more sense.
	// In that case, we should make sure that we are aware if the code on the target
	// does not correspond to our source.

	//! Register all debugs-related handlers
	void HttpDispatcher::registerDebugsHandlers()
	{
		REGISTER_HANDLER(getDebugsHandler, GET, { "debugs" }, { R"#json#(
			{
				"description" : "Obtain current debug state for all nodes.",
				"operationId" : "GET-debugs",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get All Debug States",
						"schema" : {
							"items" : {
								"properties" : {
									"id" : {
										"type" : "integer"
									},
									"state" : {
										"$ref" : "#/definitions/vm-execution-state"
									}
								},
								"type" : "object"
							},
							"type" : "array"
						}
					}
				},
				"summary" : "Get All Debug States",
				"tags" : [
					"Debug"
				]
			})#json#"_json });
		REGISTER_HANDLER(getDebugsNodeHandler, GET, { "debugs", "{node}" }, { R"#json#(
			{
				"description" : "Obtain current debug state for the given node.",
				"operationId" : "GET-debugs-node",
				"produces" : [
					"application/json"
				],
				"responses" : {
					"200" : {
						"description" : "Get Debug State",
						"schema" : {
							"$ref" : "#/definitions/vm-execution-state"
						}
					}
				},
				"summary" : "Get Debug State",
				"tags" : [
					"Debug"
				]
			})#json#"_json });
		REGISTER_HANDLER(postDebugsNodeHandler, POST, { "debugs", "{node}" }, { R"#json#(
			{
				"consumes" : [
					"application/json"
				],
				"description" : "Change the debug state of a node instance.\n\nPOST because this is a controller.",
				"operationId" : "POST-debugs-node",
				"parameters" : [
					{
						"in" : "body",
						"name" : "body",
						"schema" : {
							"$ref" : "#/definitions/vm-execution-state",
							"example" : {
								"flags" : {
									"vm_step_by_step" : false
								}
							}
						}
					}
				],
				"produces" : [
					"application/json"
				],
				"responses" : {
					"204" : {
						"description" : "Change Debug State",
						"schema" : {
							"type" : "null"
						}
					},
					"404" : {
						"description" : "Unknown node",
						"schema" : {
							"type" : "string"
						}
					}
				},
				"summary" : "Change Debug State",
				"tags" : [
					"Nodes",
					"Debug"
				]
			})#json#"_json });
	}

	//! handler for GET /debugs -> application/json
	void HttpDispatcher::getDebugsHandler(HandlerContext& context)
	{
		json response(json::array());
		for (const auto& nodeKV : context.asebaSwitch->nodes)
		{
			// clang-format off
			response.push_back({
				{ "id", nodeKV.first },
				{ "state", 
					jsonDebugsNodeHandler({
						nodeKV.first,
						polymorphic_downcast<Switch::NodeWithProgram*>(nodeKV.second.get())
					})
				}
			});
			// clang-format on
		}
		HttpResponse::fromJSON(response).send(context.stream);
	}

	//! handler for GET /debugs/{node} -> application/json
	void HttpDispatcher::getDebugsNodeHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;

		HttpResponse::fromJSON(jsonDebugsNodeHandler(node)).send(context.stream);
	}

	//! handler for POST /debugs/{node} -> application/json
	void HttpDispatcher::postDebugsNodeHandler(HandlerContext& context)
	{
		const NodeEntry node(findNode(context));
		if (!node.second)
			return;

		// TODO: implement
		HttpResponse::fromJSON({}).send(context.stream);
	}

	// support

	//! Return the variables part of the symbol table of a node in JSON
	json HttpDispatcher::jsonDebugsNodeHandler(const NodeEntry& node) const
	{
		// clang-format off
		return {
			{ "flags", {
				{ "eventActive", bool(AsebaMaskIsSet(node.second->executionFlags, ASEBA_VM_EVENT_ACTIVE_MASK)) },
				{ "stepByStep", bool(AsebaMaskIsSet(node.second->executionFlags, ASEBA_VM_STEP_BY_STEP_MASK)) }
			}},
			{ "pc", node.second->pc }
		};
		// clang-format on
	}

} // namespace Aseba
