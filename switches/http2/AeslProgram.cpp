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

#include <iostream>
#include "../../common/consts.h"
#include "../../common/utils/utils.h"
#include "AeslProgram.h"

using Aseba::Http::AeslProgram;
using std::cerr;
using std::endl;

AeslProgram::AeslProgram(const std::string& filename) :
	loaded(false)
{
	// local file or URL
	xmlDoc *doc(xmlReadFile(filename.c_str(), NULL, 0));
	if(!doc) {
		cerr << "Cannot read AESL script XML from file " << filename << endl;
	} else {
		load(doc);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

AeslProgram::AeslProgram(const char *buffer, const int size) :
	loaded(false)
{
	// local file or URL
	xmlDoc *doc(xmlReadMemory(buffer, size, "vmcode.aesl", NULL, 0));
	if(!doc) {
		cerr << "Cannot read AESL script XML from memory " << buffer << endl;
	} else {
		load(doc);
	}
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

AeslProgram::~AeslProgram()
{

}

void AeslProgram::load(xmlDoc *doc)
{
	loaded = true;

	// clear existing data
	commonDefinitions.events.clear();
	commonDefinitions.constants.clear();

	// load new data
	xmlXPathContextPtr context = xmlXPathNewContext(doc);
	xmlXPathObjectPtr obj;

	// 1. Path network/event
	if((obj = xmlXPathEvalExpression(BAD_CAST"/network/event", context))) {
		xmlNodeSetPtr nodeset = obj->nodesetval;
		for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i) {
			xmlChar *name(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
			xmlChar *size(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("size")));
			if(name && size) {
				int eventSize = atoi((const char *) size);
				if(eventSize > ASEBA_MAX_EVENT_ARG_SIZE) {
					cerr << "Event " << name << " has a length " << eventSize << " larger than maximum" << ASEBA_MAX_EVENT_ARG_SIZE << endl;
					loaded = false;
					break;
				} else commonDefinitions.events.push_back(NamedValue(UTF8ToWString((const char *) name), eventSize));
			}
			xmlFree(name);
			xmlFree(size);
		}
		xmlXPathFreeObject(obj); // also frees nodeset
	}
	// 2. Path network/constant
	if((obj = xmlXPathEvalExpression(BAD_CAST"/network/constant", context))) {
		xmlNodeSetPtr nodeset = obj->nodesetval;
		for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i) {
			xmlChar *name(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
			xmlChar *value(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("value")));
			if(name && value) commonDefinitions.constants.push_back(NamedValue(UTF8ToWString((const char *) name), atoi((const char *) value)));
			xmlFree(name);  // nop if name is NULL
			xmlFree(value); // nop if value is NULL
		}
		xmlXPathFreeObject(obj); // also frees nodeset
	}
	// 3. Path network/keywords
	if((obj = xmlXPathEvalExpression(BAD_CAST"/network/keywords", context))) {
		xmlNodeSetPtr nodeset = obj->nodesetval;
		for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i) {
			xmlChar *flag(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("flag")));
			// do nothing because compiler doesn't pay attention to keywords
			xmlFree(flag);
		}
		xmlXPathFreeObject(obj); // also frees nodeset
	}
	// 4. Path network/node
	if((obj = xmlXPathEvalExpression(BAD_CAST"/network/node", context))) {
		xmlNodeSetPtr nodeset = obj->nodesetval;
		for(int i = 0; i < (nodeset ? nodeset->nodeNr : 0); ++i) {
			xmlChar *name(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("name")));
			xmlChar *storedId(xmlGetProp(nodeset->nodeTab[i], BAD_CAST("nodeId")));
			xmlChar *text(xmlNodeGetContent(nodeset->nodeTab[i]));

			if(text != NULL) {
				NodeEntry entry;
				entry.nodeName = (name == NULL ? "": (const char *) name);
				entry.nodeId = (storedId == NULL ? "": (const char *) storedId);
				entry.code = (const char *) text;

				entries.push_back(entry);
			} else {
				cerr << "Encountered node with no code" << endl;
				loaded = false;
			}

			// free attribute and content
			xmlFree(name);     // nop if name is NULL
			xmlFree(storedId); // nop if name is NULL
			xmlFree(text);     // nop if text is NULL
		}
		xmlXPathFreeObject(obj); // also frees nodeset
	}

	// release memory
	xmlXPathFreeContext(context);

	// check if there was an error
	if(!loaded) {
		entries.clear();
		commonDefinitions.events.clear();
		commonDefinitions.constants.clear();
	}
}
