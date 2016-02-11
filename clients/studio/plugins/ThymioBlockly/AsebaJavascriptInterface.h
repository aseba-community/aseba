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

#ifndef ASEBA_JAVASCRIPT_INTERFACE_H
#define ASEBA_JAVASCRIPT_INTERFACE_H

#include <QObject>

namespace Aseba { namespace ThymioBlockly
{
	/** \addtogroup studio */
	/*@{*/
	class AsebaJavascriptInterface : public QObject
	{
		Q_OBJECT

		public:
			void requestReset() { emit reset(); }
			void requestLoad(const QString& xml) { emit load(xml); }
			void requestSave() { emit save(); }

		signals:
			void reset();
			void load(const QString& xml);
			void save();
			void compiled(const QString& code);
			void saved(const QString& xml);
	};

	/*@}*/
} } // namespace ThymioBlockly / namespace Aseba

#endif
