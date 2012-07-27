
#include "CompilerTranslator.h"
#include "../compiler/errors_code.h"

#include <CompilerTranslator.moc>

namespace Aseba
{
	CompilerTranslator::CompilerTranslator()
	{

	}

	const std::wstring CompilerTranslator::translate(ErrorCode error)
	{
		QString msg;

		switch (error)
		{

			case ERROR_01:
				msg = tr("arg");
				break;

			case ERROR_02:
				msg = tr("oups");
				break;

			case ERROR_03:
				msg = tr("another error %0");
				break;

			default:
				msg = tr("Unknown error");
		}

		return msg.toStdWString();
	}
};
