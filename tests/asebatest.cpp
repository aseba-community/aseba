// Aseba
#include "../compiler/compiler.h"
#include "../vm/vm.h"
#include "../vm/natives.h"
#include "../common/consts.h"
#include "../common/utils/utils.h"
#include "../common/utils/FormatableString.h"
using namespace Aseba;

// C++
#include <string>
#include <iostream>
#include <locale>
#include <fstream>
#include <sstream>
#include <valarray>

// C
#include <getopt.h>		// getopt_long()
#include <stdlib.h>		// exit()

// defines
#define DEFAULT_STEPS	1000

extern "C" bool AsebaExecutionErrorOccurred();

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	ASEBA_NATIVES_STD_DESCRIPTIONS,
	0
};

extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescriptions;
}

// helper function
std::wstring read_source(const std::string& filename);
void dump_source(const std::wstring& source);

static const char short_options [] = "fcepnsdmi:";
static const struct option long_options[] = { 
	{ "fail",	no_argument,			NULL,	'f'},
	{ "comp_fail",	no_argument,		NULL,	'c'},
	{ "exec_fail",	no_argument,		NULL,	'e'},
	{ "post_fail",	no_argument,		NULL,	'p'},
	{ "memcmp_fail",no_argument,		NULL,	'n'},
	{ "source",		no_argument,		NULL,	's'},
	{ "dump",		no_argument,		NULL,	'd'},
	{ "memdump",	no_argument,		NULL,	'u'},
	{ "memcmp", 	required_argument,	NULL,	'm'},
	{ "steps", 		required_argument,	NULL,	'i'},
	{ 0, 0, 0, 0 } 
};

static void usage (int argc, char** argv)
{
	std::cerr 	<< "Usage: " << argv[0] << " [options] source" << std::endl << std::endl
			<< "Options:" << std::endl
			<< "    -f | --fail         Return EXIT_SUCCESS if anything fails" << std::endl
			<< "    -c | --comp_fail    Return EXIT_SUCCESS if compilation fails" << std::endl
			<< "    -e | --exec_fail    Return EXIT_SUCCESS if execution fails" << std::endl
			<< "    -p | --post_fail    Return EXIT_SUCCESS if post-execution state fails" << std::endl
			<< "    -n | --memcmp_fail  Return EXIT_SUCCESS if memory comparison fails" << std::endl
			<< "    -s | --source       Dump the source code" << std::endl
			<< "    -d | --dump         Dump the compilation result (tokens, tree, bytecode)" << std::endl
			<< "    -u | --memdump      Dump the memory content at the end of the execution" << std::endl
			<< "    -m | --memcmp file  Compare result of the VM execution with file" << std::endl
			<< "    -i | --steps        Number of VM execution steps (default: " << DEFAULT_STEPS << ")" << std::endl;
}




struct AsebaNode
{
	AsebaVMState vm;
	std::valarray<unsigned short> bytecode;
	std::valarray<signed short> stack;
	TargetDescription d;
	
	struct Variables
	{
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
		
		// fill description accordingly
		d.name = L"testvm";
		d.protocolVersion = ASEBA_PROTOCOL_VERSION;
		
		d.bytecodeSize = vm.bytecodeSize;
		d.variablesSize = vm.variablesSize;
		d.stackSize = vm.stackSize;
		
		/*d.namedVariables.push_back(TargetDescription::NamedVariable("id", 1));
		d.namedVariables.push_back(TargetDescription::NamedVariable("source", 1));
		d.namedVariables.push_back(TargetDescription::NamedVariable("args", 32));*/
		
		const AsebaNativeFunctionDescription* const* nativeDescs(AsebaGetNativeFunctionsDescriptions(&vm));
		while (*nativeDescs)
		{
			const AsebaNativeFunctionDescription* nativeDesc(*nativeDescs);
			std::string name(nativeDesc->name);
			std::string doc(nativeDesc->doc);
			
			TargetDescription::NativeFunction native(
				std::wstring(name.begin(), name.end()),
				std::wstring(doc.begin(), doc.end())
			);
			
			const AsebaNativeFunctionArgumentDescription* params(nativeDesc->arguments);
			while (params->size)
			{
				AsebaNativeFunctionArgumentDescription param(*params);
				name = param.name;
				int size = param.size;
				native.parameters.push_back(
					TargetDescription::NativeFunctionParameter(std::wstring(name.begin(), name.end()), size)
				);
				++params;
			}
			
			d.nativeFunctions.push_back(native);
			
			++nativeDescs;
		}
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
	
	void run(int stepCount)
	{
		// run VM
		AsebaVMSetupEvent(&vm, ASEBA_EVENT_INIT);
		AsebaVMRun(&vm, stepCount);
	}
};

void checkForError(const std::string& module, bool shouldFail, bool wasError, const std::wstring& errorMessage = L"")
{
	if (wasError)
	{
		// errors
		std::cerr << "An error in " << module << " occured";
		if (!errorMessage.empty())
			std::wcerr << ':' << std::endl << errorMessage;
		std::cerr << std::endl;
		if (shouldFail)
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
		std::cerr << module << " was successful" << std::endl;
	}
}

int main(int argc, char** argv)
{
	bool should_compilation_fail = false;
	bool should_execution_fail = false;
	bool should_postexecution_fail = false;
	bool should_memcmp_fail = false;
	bool source = false;
	bool dump = false;
	bool memDump = false;
	bool memCmp = false;
	int stepCount = DEFAULT_STEPS;
	std::string memCmpFileName;
	
	std::locale::global(std::locale(""));
	
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
				should_compilation_fail = true;
				should_execution_fail = true;
				should_postexecution_fail = true;
				should_memcmp_fail = true;
				break;
			case 'c':
				should_compilation_fail = true;
				break;
			case 'e':
				should_execution_fail = true;
				break;
			case 'p':
				should_postexecution_fail = true;
				break;
			case 'n':
				should_memcmp_fail = true;
				break;
			case 's':
				source = true;
				break;
			case 'd':
				dump = true;
				break;
			case 'u':
				memDump = true;
				break;
			case 'm':
				memCmp = true;
				memCmpFileName = optarg;
				break;
			case 'i':
				stepCount = atoi(optarg);
				break;
			default:
				usage(argc, argv);
				exit(EXIT_FAILURE);
		}
	}
	
	const bool should_any_fail(should_compilation_fail || should_execution_fail || should_postexecution_fail || should_memcmp_fail);

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
	
	// read source
	const std::wstring wSource = read_source(filename);
	
	// dump source
	if (source)
		dump_source(wSource);
	
	// parse source
	std::wistringstream ifs(wSource);

	Compiler compiler;

	// fake target description
	AsebaNode node;
	CommonDefinitions definitions;
	definitions.events.push_back(NamedValue(L"event1", 0));
	definitions.events.push_back(NamedValue(L"event2", 3));
	definitions.constants.push_back(NamedValue(L"FOO", 2));

	BytecodeVector bytecode;
	unsigned int varCount;
	Error outError;

	// compile
	compiler.setTargetDescription(node.getTargetDescription());
	compiler.setCommonDefinitions(&definitions);
	if (dump)
		compiler.compile(ifs, bytecode, varCount, outError, &(std::wcout));
	else
		compiler.compile(ifs, bytecode, varCount, outError, NULL);

	//ifs.close();
	
	checkForError("Compilation", should_compilation_fail, (outError.message != L"not defined"), outError.toWString());
	
	// run
	if (!node.loadBytecode(bytecode))
	{
		std::cerr << "Load bytecode failure" << std::endl;
		return EXIT_FAILURE;
	}
	node.run(stepCount);
	
	checkForError("Execution", should_execution_fail, AsebaExecutionErrorOccurred());
	
	const bool stillExecuting(node.vm.flags & ASEBA_VM_EVENT_ACTIVE_MASK);
	
	checkForError("PostExecution", should_postexecution_fail, stillExecuting, WFormatableString(L"VM was still running after %0 steps").arg(stepCount));

	if (memDump)
	{
		std::wcout << L"Memory dump:" << std::endl;
		for (int i = 0; i < node.vm.variablesSize; i++)
		{
			std::wcout << node.vm.variables[i] << std::endl;
		}
	}
	
	if (memCmp)
	{
		std::ifstream ifs;
		ifs.open(memCmpFileName.data(), std::ifstream::in);
		if (!ifs.is_open())
		{
			std::cerr << "Error opening mem dump file " << memCmpFileName << std::endl;
			exit(EXIT_FAILURE);
		}
		size_t i = 0;
		while (!ifs.eof())
		{
			int v;
			ifs >> v;
			if (ifs.eof())
				break;
			if (i >= node.vm.variablesSize)
				break;
			if (node.vm.variables[i] != v)
			{
				std::cerr << "VM variable value at pos " << i << " after execution differs from dump; expected: " << v << ", found: " << node.vm.variables[i] << std::endl;
				if (should_memcmp_fail)
				{
					std::cerr << "Failure was expected" << std::endl;
					exit(EXIT_SUCCESS);
				}
				else
					exit(EXIT_FAILURE);
			}
			++i;
		}
		ifs.close();
	}
	
	if (should_any_fail)
	{
		std::cerr << "All tests passed successfully, but failure was expected" << std::endl;
		exit(EXIT_FAILURE);
	}
	
	return EXIT_SUCCESS;
}

// read source code to a string
std::wstring read_source(const std::string& filename)
{
	std::ifstream ifs;
	ifs.open( filename.c_str(),std::ifstream::binary);
	if (!ifs.is_open())
	{
		std::cerr << "Error opening source file " << filename << std::endl;
		exit(EXIT_FAILURE);
	}
	
	ifs.seekg (0, std::ios::end);
	std::streampos length = ifs.tellg();
	ifs.seekg (0, std::ios::beg);
	
	std::string utf8Source;
	utf8Source.resize(length);
	ifs.read(&utf8Source[0], length);
	ifs.close();
	
	/*for (size_t i = 0; i < utf8Source.length(); ++i)
		std::cerr << "source char " << i << " is 0x" << std::hex << (unsigned)(unsigned char)utf8Source[i] << " (" << char(utf8Source[i]) << ")" << std::endl;
	*/
	const std::wstring s = UTF8ToWString(utf8Source);
	
	/*std::cerr << "len utf8 " << utf8Source.length() << " len final " << s.length() << std::endl;
	for (size_t i = 0; i < s.length(); ++i)
		std::wcerr << "dest char " << i << " is 0x" << std::hex << (unsigned)s[i] << " (" << s[i] << ")"<< std::endl;
	*/
	return s;
}

// dump program source
void dump_source(const std::wstring& source)
{
	std::wistringstream is(source);
	wchar_t c;
	std::cout << "Program:" << std::endl;
	int line = 1;
	bool header = true;
	while (is)
	{
		if (header)
		{
			std::cout << line << "  ";
			header = false;
		}
		c = is.get();
		std::wcout << c;
		if (c == '\n')
		{
			header = true;
			line++;
		}
	}
	std::cout << std::endl;
}

