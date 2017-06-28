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

#include <functional>
#include <unordered_set>
#include <QCoreApplication>
#include "../../common/zeroconf/zeroconf-qt.h"

class QtTargetLister : public QCoreApplication
{
	Q_OBJECT

public:
	Aseba::QtZeroconf targets;

public:
	QtTargetLister(int argc, char* argv[]);

public slots:
	void browseCompleted();
	void resolveCompleted(const Aseba::Zeroconf::TargetInformation& target);

private:
	std::unordered_set<std::reference_wrapper<const Aseba::Zeroconf::TargetInformation>> todo;
};
