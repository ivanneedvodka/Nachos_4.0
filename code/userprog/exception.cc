// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions
//	is in machine.h.
//----------------------------------------------------------------------

char *UserToKernel(int addr, int size)
{
	int i;
	int _char;
	char *kernelBuf = NULL;
	kernelBuf = new char[size + 1];
	if (kernelBuf == NULL)
		return kernelBuf;

	memset(kernelBuf, 0, size + 1);

	for (i = 0; i < size; i++)
	{
		kernel->machine->ReadMem(addr + i, 1, &_char);
		kernelBuf[i] = (char)_char;
		if (_char == 0)
			break;
	}
	return kernelBuf;
}

int KerneltoUser(int addr, int len, char *buffer)
{
	if (len < 0)
		return -1;
	if (len == 0)
		return len;
	int i = 0;
	int _char = 0;
	do
	{
		_char = (int)buffer[i];
		kernel->machine->WriteMem(addr + i, 1, _char);
		i++;
	} while (i < len && _char != 0 && _char != '\0');
	return i;
}

void increasePC()
{
	/* set previous programm counter (debugging only)*/
	kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	/* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

	/* set next programm counter for brach execution */
	kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
}

void handleReadChar()
{
	char result = SysReadChar();
	kernel->machine->WriteRegister(2, result);

	increasePC();
}

void handleReadNum()
{
	int number;
	number = SysReadNum();
	kernel->machine->WriteRegister(2, number);

	increasePC();
}

void handlePrintNum()
{
	int printnum;
	printnum = kernel->machine->ReadRegister(4);

	SysPrintNum(printnum);

	increasePC();
}

void handlePrintChar()
{
	char character = (char)kernel->machine->ReadRegister(4);
	SysPrintChar(character);

	increasePC();
}

void handleRandomNum()
{
	int randomnum = SysRandomNum();
	kernel->machine->WriteRegister(2, randomnum);

	increasePC();
}

#define MAX_READ_STRING_LENGTH 255
void handleReadString()
{
	int userBufferAddress = kernel->machine->ReadRegister(4);
	int stringLength = kernel->machine->ReadRegister(5);
	SysReadString(userBufferAddress, stringLength);
	increasePC();
}

void handlePrintString()
{
	int userBufferAddress = kernel->machine->ReadRegister(4);
	SysPrintString(userBufferAddress);
	increasePC();
}

void ExceptionHandler(ExceptionType which)
{
	int type = kernel->machine->ReadRegister(2);

	DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

	switch (which)
	{
	case NoException: // return control to kernel
		kernel->interrupt->setStatus(SystemMode);
		DEBUG(dbgSys, "Switch to system mode\n");
		break;
	case PageFaultException:
	case ReadOnlyException:
	case BusErrorException:
	case AddressErrorException:
	case OverflowException:
	case IllegalInstrException:
	case NumExceptionTypes:
		cerr << "Error " << which << " occurs\n";
		SysHalt();
		ASSERTNOTREACHED();

	case SyscallException:
		switch (type)
		{
		case SC_Halt:
			DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

			SysHalt();

			ASSERTNOTREACHED();
			break;

		case SC_Add:
			DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");

			/* Process SysAdd Systemcall*/
			int result;
			result = SysAdd(/* int op1 */ (int)kernel->machine->ReadRegister(4),
							/* int op2 */ (int)kernel->machine->ReadRegister(5));

			DEBUG(dbgSys, "Add returning with " << result << "\n");
			/* Prepare Result */
			kernel->machine->WriteRegister(2, (int)result);

			increasePC();

			return;

			ASSERTNOTREACHED();

			break;
		case SC_ReadChar:
			handleReadChar();
			return;
		case SC_PrintChar:
			handlePrintChar();
			return;
		case SC_ReadNum:
			handleReadNum();
			return;
		case SC_PrintNum:
			handlePrintNum();
			return;
		case SC_RandomNum:
			handleRandomNum();
			return;
		case SC_ReadString:
			handleReadString();
			return;
		case SC_PrintString:
			handlePrintString();
			return;
		default:
			cerr << "Unexpected system call " << type << "\n";
			break;
		}
		break;
	default:
		cerr << "Unexpected user mode exception" << (int)which << "\n";
		break;
	}
	ASSERTNOTREACHED();
}
