// Aseba
#include "../compiler/compiler.h"
using namespace Aseba;

// C++
#include <string>
#include <iostream>
#include <fstream>

// C
#include <getopt.h>		// getopt_long()
#include <stdlib.h>		// exit()

// helper function
void dump_source(std::string filename);

static const char short_options [] = "fsd";
static const struct option long_options[] = { 
	{ "fail",	no_argument,		NULL,		'f'},
	{ "source",	no_argument,		NULL,		's'},
	{ "dump",	no_argument,		NULL,		'd'},
	{ 0, 0, 0, 0 } 
};

static void usage (int argc, char** argv)
{
	std::cerr 	<< "Usage: " << argv[0] << " [options] source" << std::endl << std::endl
			<< "Options:" << std::endl
			<< "    -f | --fail    Return EXIT_SUCCESS if compilation fail" << std::endl
			<< "    -s | --source  Dump the source code" << std::endl
			<< "    -d | --dump    Dump the result (tokens, tree, bytecode)" << std::endl;
}

int main(int argc, char** argv)
{
	bool should_fail = false;
	bool source = false;
	bool dump = false;

	// parse the arguments
	for(;;)
	{
		int index;
		int c;

		c = getopt_long(argc, argv, short_options, long_options, &index);

		if (c==-1)
			break;

		switch(c)
		{
			case 0: // getopt_long() flag
				break;
			case 'f':
				should_fail = true;
				break;
			case 's':
				source = true;
				break;
			case 'd':
				dump = true;
				break;
			default:
				usage(argc, argv);
				exit(EXIT_FAILURE);
		}
        }

	std::string filename;
	
	// check for the source file
	if (optind == (argc-1))
	{
		filename.assign(argv[optind]);
	}
	else
	{
		usage(argc, argv);
		exit(EXIT_FAILURE);
	}

	// dump source
	if (source)
		dump_source(filename);

	// open file
	std::fstream ifs;
	ifs.open( filename.data(), std::ifstream::in);
	if (!ifs.is_open())
	{
		std::cerr << "Error opening source file " << filename << std::endl;
		exit(EXIT_FAILURE);
	}

	Compiler compiler;

	// fake target description
	// FIXME: load from a configuration file?
	TargetDescription description;
	description.name = "Test Node";
	description.protocolVersion = 1;
	description.bytecodeSize = 2000;
	description.variablesSize = 200;
	description.stackSize = 5;
	CommonDefinitions definitions;

	BytecodeVector bytecode;
	unsigned int varCount;
	Error outError;

	// compile
	compiler.setTargetDescription(&description);
	compiler.setCommonDefinitions(&definitions);
	if (dump)
		compiler.compile(ifs, bytecode, varCount, outError, &(std::cout));
	else
		compiler.compile(ifs, bytecode, varCount, outError, NULL);

	ifs.close();

	// check for errors
	if (outError.message != "not defined")
	{
		// errors
		std::cerr << "An error occured:" << std::endl;
		std::cerr << outError.message << std::endl;
		if (should_fail)
		{
			// this was expected
			std::cerr << "Failure was expected" << std::endl;
			exit(EXIT_SUCCESS);
		}
		else
		{
			// oops
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		// no errors
		std::cerr << "Compilation successful" << std::endl;
		if (should_fail)
		{
			// it should have failed...
			std::cerr << "... and this was NOT expected!" << std::endl;
			exit(EXIT_FAILURE);
		}
		else
		{
			// ok, cool
			exit(EXIT_SUCCESS);
		}
	}
}

// dump program source
void dump_source(std::string filename)
{
	std::fstream ifs;
	ifs.open( filename.data() , std::ifstream::in );
	if (!ifs.is_open())
	{
		std::cerr << "Error opening source file " << filename << std::endl;
		exit(EXIT_FAILURE);
	}
		
	char c;
	std::cout << "Program:" << std::endl;
	int line = 1;
	bool header = true;
	while (ifs)
	{
		if (header)
		{
			std::cout << line << "  ";
			header = false;
		}
		ifs.get(c);
		std::cout << c;
		if (c == '\n')
		{
			header = true;
			line++;
		}
	}
	std::cout << std::endl;

	ifs.close();
}

