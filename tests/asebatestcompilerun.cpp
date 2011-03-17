// Aseba
#include "../compiler/compiler.h"
#include "../vm/vm.h"
#include "../common/consts.h"
using namespace Aseba;

// C++
#include <string>
#include <iostream>
#include <fstream>
#include <valarray>

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

extern "C" void AsebaSendMessage(AsebaVMState *vm, uint16 type, void *data, uint16 size)
{
	std::cerr << "AsebaSendMessage of type " << type << ", size " << size << std::endl;
}

extern "C" void AsebaSendVariables(AsebaVMState *vm, uint16 start, uint16 length)
{
	std::cerr << "AsebaSendVariables at pos " << start << ", length " << length << std::endl;
}

extern "C" void AsebaSendDescription(AsebaVMState *vm)
{
	std::cerr << "AsebaSendDescription" << std::endl;
}

extern "C" void AsebaPutVmToSleep(AsebaVMState *vm)
{
	std::cerr << "AsebaPutVmToSleep" << std::endl;
}

extern "C" void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	//nativeFunctions[id](vm);
	std::cerr << "AsebaNativeFunction" << std::endl;
	// TODO: add native functions
}

extern "C" void AsebaWriteBytecode(AsebaVMState *vm)
{
	std::cerr << "AsebaWriteBytecode" << std::endl;
}

extern "C" void AsebaResetIntoBootloader(AsebaVMState *vm)
{
	std::cerr << "AsebaResetIntoBootloader" << std::endl;
}

static bool wasAssert(false);

extern "C" void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	std::cerr << "\nFatal error: ";
	switch (vm->nodeId)
	{
		case 1: std::cerr << "left motor module"; break;
		case 2:	std::cerr << "right motor module"; break;
		case 3: std::cerr << "proximity sensors module"; break;
		case 4: std::cerr << "distance sensors module"; break;
		default: std::cerr << "unknown module"; break;
	}
	std::cerr << " has produced exception: ";
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: std::cerr << "undefined"; break;
		case ASEBA_ASSERT_UNKNOWN_UNARY_OPERATOR: std::cerr << "unknown unary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: std::cerr << "unknown binary operator"; break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: std::cerr << "unknown bytecode"; break;
		case ASEBA_ASSERT_STACK_OVERFLOW: std::cerr << "stack overflow"; break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: std::cerr << "stack underflow"; break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: std::cerr << "out of variables bounds"; break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "out of bytecode bounds"; break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: std::cerr << "step out of run"; break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: std::cerr << "breakpoint out of bytecode bounds"; break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: std::cerr << "tried to emit a buffer too long"; break;
		default: std::cerr << "unknown exception"; break;
	}
	std::cerr << ".\npc = " << vm->pc << ", sp = " << vm->sp;
	std::cerr << "\nResetting VM" << std::endl;
	wasAssert = true;
	AsebaVMInit(vm);
}

struct AsebaNode
{
	AsebaVMState vm;
	std::valarray<unsigned short> bytecode;
	std::valarray<signed short> stack;
	TargetDescription d;
	
	struct Variables
	{
		sint16 id;
		sint16 source;
		sint16 args[32];
		sint16 user[256];
	} variables;

	AsebaNode()
	{
		// create VM
		vm.nodeId = 0;
		bytecode.resize(512);
		vm.bytecode = &bytecode[0];
		vm.bytecodeSize = bytecode.size();
		
		stack.resize(64);
		vm.stack = &stack[0];
		vm.stackSize = stack.size();
		
		vm.variables = reinterpret_cast<sint16 *>(&variables);
		vm.variablesSize = sizeof(variables) / sizeof(sint16);
		
		AsebaVMInit(&vm);
		
		variables.id = 0;
		
		// fill description accordingly
		d.name = "testvm";
		d.protocolVersion = ASEBA_PROTOCOL_VERSION;
		
		d.bytecodeSize = vm.bytecodeSize;
		d.variablesSize = vm.variablesSize;
		d.stackSize = vm.stackSize;
		
		d.namedVariables.push_back(TargetDescription::NamedVariable("id", 1));
		d.namedVariables.push_back(TargetDescription::NamedVariable("source", 1));
		d.namedVariables.push_back(TargetDescription::NamedVariable("args", 32));
	}
	
	const TargetDescription* getTargetDescription() const
	{
		return &d;
	}

	bool loadBytecode(const BytecodeVector& bytecode)
	{
		size_t i = 0;
		for (BytecodeVector::const_iterator it(bytecode.begin()); it != bytecode.end(); ++it)
		{
			if (i == vm.bytecodeSize)
				return false;
			const BytecodeElement& be(*it);
			vm.bytecode[i++] = be.bytecode;
		}
		return true;
	}
	
	void run()
	{
		// run VM
		AsebaVMSetupEvent(&vm, ASEBA_EVENT_INIT);
		AsebaVMRun(&vm, 1000);
	}
};

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
	AsebaNode node;
	CommonDefinitions definitions;

	BytecodeVector bytecode;
	unsigned int varCount;
	Error outError;

	// compile
	compiler.setTargetDescription(node.getTargetDescription());
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
			// ok, cool, we can continue
		}
	}
	
	// run
	if (!node.loadBytecode(bytecode))
	{
		std::cerr << "Load bytecode failure" << std::endl;
		return EXIT_FAILURE;
	}
	node.run();
	if (wasAssert)
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
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

