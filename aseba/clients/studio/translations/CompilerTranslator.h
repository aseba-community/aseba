#ifndef _TRANSLATOR_H
#define _TRANSLATOR_H

#include "../../../compiler/errors_code.h"
#include <QtCore>
#include <string>

namespace Aseba
{
	class CompilerTranslator: public QObject
	{
		Q_OBJECT

		public:
			CompilerTranslator();

			static const std::wstring translate(ErrorCode error);
	};
};

#endif // _TRANSLATOR_H

