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

#ifndef ASEBA_ABOUT_DIALOG
#define ASEBA_ABOUT_DIALOG

#include <QDialog>
#include <QString>
#include <QUrl>
#include <set>

namespace Aseba
{
	/** \addtogroup common */
	/*@{*/

	class AboutBox : public QDialog
	{
		Q_OBJECT

	public:
		using Tags = std::set<std::string>;
		struct Parameters
		{
			QString applicationName;
			QString iconFile;
			QString description;
			QUrl helpUrl;
			QString usage;
			Tags tags;
		};

	public:
		AboutBox(QWidget* parent, const Parameters& parameters);
	};

	/*@}*/
} // namespace Aseba

#endif // ASEBA_ABOUT_DIALOG
