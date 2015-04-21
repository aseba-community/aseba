#include "Utils.h"
#include <QSvgRenderer>
#include <QPainter>

namespace Aseba { namespace ThymioVPL
{
	
	QPixmap pixmapFromSVG(const QString& fileName, int size)
	{
		QSvgRenderer renderer(fileName);
		QPixmap pixmap(size, size);
		pixmap.fill(Qt::transparent);
		QPainter painter(&pixmap);
		renderer.render(&painter);
		return pixmap;
	}

} } // namespace ThymioVPL / namespace Aseba