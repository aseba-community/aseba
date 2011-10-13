/*
	Aseba - an event-based framework for distributed robot control
	Copyright (C) 2007--2011:
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

#ifndef HELP_VIEWER_H
#define HELP_VIEWER_H

#include <QWidget>
#include <QString>
#include <QTextBrowser>

namespace Aseba
{
	class HelpViewer: public QWidget
	{
		public:
			enum helpType {
				USERMANUAL,
				STUDIO,
				LANGUAGE
			};

		public:
			HelpViewer();
			virtual ~HelpViewer() {};

			void setLanguage(QString lang);
			void showHelp(helpType type);


		protected:
			QTextBrowser* viewer;
			QString language;
	};
}

#endif // HELP_VIEWER_H

