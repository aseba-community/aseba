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

#ifndef ASEBA_AESL_LOADER
#define ASEBA_AESL_LOADER

#include <string>
#include <vector>

#if defined(_WIN32) && defined(__MINGW32__)
/* This is a workaround for MinGW32, see libxml/xmlexports.h */
#define IN_LIBXML
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "../../compiler/compiler.h"

namespace Aseba { namespace Http
{
	/**
	 * Class representing an AESL program with several code entries that can later be compiled and
	 * executed on nodes matching a specified node name or id.
	 *
	 * AESL programs can be constructed either from a file or a memory buffer - in either case make
	 * sure to call isLoaded() to see if they were successfully parsed.
	 */
	class AeslProgram
	{
		public:
			struct NodeEntry {
				std::string nodeName;
				std::string nodeId;
				std::string code;
			};

			AeslProgram(const std::string& filename);
			AeslProgram(const char *buffer, const int size);
			virtual ~AeslProgram();

			virtual bool isLoaded() const { return loaded; }
			virtual const std::vector<NodeEntry>& getEntries() const { return entries; }
			virtual const CommonDefinitions& getCommonDefinitions() const { return commonDefinitions; }

		protected:
			void load(xmlDoc *doc);

		private:
			bool loaded;
			std::vector<NodeEntry> entries;
			CommonDefinitions commonDefinitions;
	};
} }

#endif
