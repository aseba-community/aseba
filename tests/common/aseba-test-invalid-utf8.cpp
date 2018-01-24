#include "common/utils/utils.h"
#include <iostream>
#include <array>

using namespace std;

int main(int argc, char*argv[])
{
	const array<string, 3> invalidUTF8s = {
		string{ static_cast<char>(192) },
		string{ static_cast<char>(224) },
		string{ static_cast<char>(224), 'a' }
	};
	const string longString("long string to avoid small string optimisation");
	for (auto invalidUTF8: invalidUTF8s)
	{
		try
		{
			auto s = Aseba::UTF8ToWString(longString + invalidUTF8);
			wcout << s << endl;
		}
		catch (const exception& e)
		{
			cerr << "Error in string conversion: " << e.what() << endl;
		}
	}
	return 0;
}


