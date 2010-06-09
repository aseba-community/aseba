#include "LocalVirtualMachine.h"
#include "../common/consts.h"
#include <QMessageBox>
#include <QtDebug>

#include <stdio.h>
#include <stdlib.h>

Aseba::LocalVirtualMachine *target = 0;

void AsebaSendEvent(AsebaVMState *vm, uint16 event, uint16 start, uint16 length)
{
	Q_UNUSED(vm);
	target->emitEvent(event, start, length);
	/*uint16 i;
	printf("AsebaIOSendEvent(%d, %d, %d):\n", event, start, length);
	for (i = start; i < start + length; i++)
		printf("	[0x%4x] %d\n", i, asebaVMState.variables[i]);
	printf("\n");*/
}

void AsebaArrayAccessOutOfBoundsException(AsebaVMState *vm, uint16 index)
{
	target->emitArrayAccessOutOfBounds(vm->pc, vm->sp, index);
	/*printf("AsebaArrayAccessOutOfBoundsException:\n");
	printf("	trying to acces element %d out of %d\n", index, asebaVMState.variablesSize);
	printf("	pc = %d, sp = %d\n\n", pc, sp);*/
}

void AsebaDivisionByZeroException(AsebaVMState *vm)
{
	target->emitDivisionByZero(vm->pc, vm->sp);
}

void AsebaNativeFunction(AsebaVMState *vm, uint16 id)
{
	QString message = QObject::tr("Native function %0 (sp = %1)").arg(id).arg(vm->sp);
	QMessageBox::information(0, QObject::tr("Native function called"), message);
}

#ifdef ASEBA_ASSERT

void AsebaAssert(AsebaVMState *vm, AsebaAssertReason reason)
{
	// set message
	uint16 pc = vm->pc;
	uint16 sp = vm->sp;
	QString message = QObject::tr("Assertion at 0x%0 (sp = %2):\n").arg(pc, 16).arg(sp);
	switch (reason)
	{
		case ASEBA_ASSERT_UNKNOWN: message += QObject::tr("unknown"); break;
		case ASEBA_ASSERT_UNKNOWN_BINARY_OPERATOR: message += QObject::tr("unknown arithmetic binary operator"); break;
		case ASEBA_ASSERT_UNKNOWN_BYTECODE: message += QObject::tr("unknown bytecode"); break;
		case ASEBA_ASSERT_STACK_UNDERFLOW: message += QObject::tr("execution stack underflow"); break;
		case ASEBA_ASSERT_STACK_OVERFLOW: message += QObject::tr("stack overflow"); break;
		case ASEBA_ASSERT_OUT_OF_VARIABLES_BOUNDS: message += QObject::tr("out of variables bounds"); break;
		case ASEBA_ASSERT_OUT_OF_BYTECODE_BOUNDS: message += QObject::tr("out of bytecode bounds"); break;
		case ASEBA_ASSERT_STEP_OUT_OF_RUN: message += QObject::tr("step called while not thread running"); break;
		case ASEBA_ASSERT_BREAKPOINT_OUT_OF_BYTECODE_BOUNDS: message += QObject::tr("a breakpoint has been set out of bytecode bounds"); break;
		case ASEBA_ASSERT_EMIT_BUFFER_TOO_LONG: message += QObject::tr("tried to emit a buffer too long"); break;
		default: message = QObject::tr("unknown assert define, your code seems broken"); break;
	}
	
	// display message
	QMessageBox::critical(0, QObject::tr("VM Assertion Failed"), message);
}

#endif /* ASEBA_ASSERT */


/*// create a stock target description
	Aseba::TargetDescription targetDescription;
	targetDescription.variablesSize = 64;
	targetDescription.bytecodeSize = 256;
	targetDescription.stackSize = 32;
	Aseba::TargetDescription::NativeFunction testFunction;
	testFunction.name = "fft";
	testFunction.argumentsSize.push_back(32);
	testFunction.argumentsSize.push_back(1);
	targetDescription.nativeFunctions.push_back(testFunction);

*/
namespace Aseba
{
	LocalVirtualMachine::LocalVirtualMachine()
	{
		AsebaVMInit(&vmState);
		vmState.flags = ASEBA_VM_DEBUGGER_ATTACHED_MASK;
		
		target = this;
		
		description.variablesSize = 64;
		description.bytecodeSize = 256;
		description.stackSize = 32;
		
		sizesChanged();
		runEvent = false;
		startTimer(40);
	}
	
	void LocalVirtualMachine::sizesChanged()
	{
		// Make sure we are not executing anything
		stop();
		
		// set VM from description
		vmBytecode.resize(description.bytecodeSize);
		vmState.bytecodeSize = description.bytecodeSize;
		vmState.bytecode = &vmBytecode[0];
		
		vmVariables.resize(description.variablesSize);
		vmState.variablesSize = description.variablesSize;
		vmState.variables = &vmVariables[0];
		
		vmStack.resize(description.stackSize);
		vmState.stackSize = description.stackSize;
		vmState.stack = &vmStack[0];
	}
	
	void LocalVirtualMachine::uploadBytecode(const BytecodeVector &bytecode)
	{
		Q_ASSERT(bytecode.size() <= vmBytecode.size());
		
		for (size_t i = 0; i < bytecode.size(); i++)
			vmBytecode[i] = bytecode[i].bytecode;
		debugBytecode = bytecode;
	}
	
	void LocalVirtualMachine::setupEvent(unsigned id)
	{
		AsebaVMSetupEvent(&vmState, id);
		
		if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			Q_ASSERT(vmState.pc < debugBytecode.size());
			if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK))
			{
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
				emit executionPosChanged(debugBytecode[vmState.pc].line);
			}
		}
	}
	
	void LocalVirtualMachine::next()
	{
		Q_ASSERT(vmState.pc < debugBytecode.size());
		
		// run until we have switched line
		unsigned short previousPC = vmState.pc;
		unsigned previousLine = debugBytecode[previousPC].line;
		do
		{
			// execute
			AsebaVMStep(&vmState);
			Q_ASSERT(!AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK) || (vmState.pc < debugBytecode.size()));
		}
		while (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK) && (vmState.pc != previousPC) && (debugBytecode[vmState.pc].line == previousLine));
		
		// emit states changes
		emit variablesMemoryChanged(vmVariables);
		if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
			emit executionPosChanged(debugBytecode[vmState.pc].line);
		else
			emit executionModeChanged(EXECUTION_INACTIVE);
	}
	
	void LocalVirtualMachine::runStepSwitch()
	{
		if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			if (runEvent)
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
			else
				emit executionModeChanged(EXECUTION_RUN_EVENT);
			runEvent = !runEvent;
		}
	}
	
	void LocalVirtualMachine::runBackground()
	{
		runEvent = false;
		AsebaMaskSet(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK);
		emit executionModeChanged(EXECUTION_RUN_BACKGROUND);
	}
	
	void LocalVirtualMachine::stop()
	{
		runEvent = false;
		AsebaMaskClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK | ASEBA_VM_RUN_BACKGROUND_MASK);
		emit executionModeChanged(EXECUTION_INACTIVE);
	}
	
	bool LocalVirtualMachine::setBreakpoint(unsigned line)
	{
		for (size_t i = 0; i < debugBytecode.size(); i++)
			if (debugBytecode[i].line == line)
				return AsebaVMSetBreakpoint(&vmState, i) != 0;
		return false;
	}
	
	bool LocalVirtualMachine::clearBreakpoint(unsigned line)
	{
		for (size_t i = 0; i < debugBytecode.size(); i++)
			if (debugBytecode[i].line == line)
				return AsebaVMClearBreakpoint(&vmState, i) != 0;
		return false;
	}
	
	void LocalVirtualMachine::clearBreakpoints()
	{
		AsebaVMClearBreakpoints(&vmState);
	}
	
	void LocalVirtualMachine::timerEvent(QTimerEvent *event)
	{
		Q_UNUSED(event);
		
		// in some case we do not want to execute anything in background
		if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
			return;
		if (!runEvent && AsebaMaskIsClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK))
			return;
		
		// execute
		const unsigned maxStepsPerTimeSlice = 100;
		unsigned i = 0;
		while (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK) && (i < maxStepsPerTimeSlice))
		{
			// execute
			if (AsebaVMStepBreakpoint(&vmState))
			{
				runEvent = false;
				AsebaMaskClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK);
				emit executionModeChanged(EXECUTION_STEP_BY_STEP);
				break;
			}
			i++;
		}
		
		// emit states changes
		if (i != 0)
			emit variablesMemoryChanged(vmVariables);
		// check if we are still running
		if (AsebaMaskIsSet(vmState.flags, ASEBA_VM_EVENT_ACTIVE_MASK))
		{
			Q_ASSERT(vmState.pc < debugBytecode.size());
			emit executionPosChanged(debugBytecode[vmState.pc].line);
		}
		else if (AsebaMaskIsClear(vmState.flags, ASEBA_VM_RUN_BACKGROUND_MASK))
			stop();
	}
	
	void LocalVirtualMachine::emitEvent(unsigned short id, unsigned short start, unsigned short length)
	{
		emit variablesMemoryChanged(vmVariables);
		emit event(id, start, length);
	}
	
	void LocalVirtualMachine::emitArrayAccessOutOfBounds(unsigned short pc, unsigned short sp, unsigned short index)
	{
		Q_UNUSED(sp);
		
		stop();
		emit variablesMemoryChanged(vmVariables);
		
		Q_ASSERT(pc < debugBytecode.size());
		unsigned line = debugBytecode[pc].line;
		emit arrayAccessOutOfBounds(line, index);
	}
	
	void LocalVirtualMachine::emitDivisionByZero(unsigned short pc, unsigned short sp)
	{
		Q_UNUSED(sp);
		
		stop();
		emit variablesMemoryChanged(vmVariables);
		
		Q_ASSERT(pc < debugBytecode.size());
		unsigned line = debugBytecode[pc].line;
		emit divisionByZero(line);
	}
}; // Aseba
