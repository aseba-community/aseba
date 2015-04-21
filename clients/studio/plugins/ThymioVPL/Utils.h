#ifndef VPL_UTILS_H
#define VPL_UTILS_H

#include <QPixmap>
#include <QString>

namespace Aseba { namespace ThymioVPL
{
	
	QPixmap pixmapFromSVG(const QString& fileName, int size);
	
} } // namespace ThymioVPL / namespace Aseba

#endif // VPL_UTILS_H
