// -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
//
// The contents of this file are subject to the Netscape Public
// License Version 1.1 (the "License"); you may not use this file
// except in compliance with the License. You may obtain a copy of
// the License at http://www.mozilla.org/NPL/
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express oqr
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
// The Original Code is the JavaScript 2 Prototype.
//
// The Initial Developer of the Original Code is Netscape
// Communications Corporation.  Portions created by Netscape are
// Copyright (C) 1998 Netscape Communications Corporation. All
// Rights Reserved.

#include "interpreter.h"
#include "world.h"

#include <map>

namespace JavaScript {

#define op1(i) (i->itsOperand1)
#define op2(i) (i->itsOperand2)
#define op3(i) (i->itsOperand3)

JSValue interpret(InstructionStream& iCode, const JSValues& args)
{
	JSValue result;
	JSValues frame(args);
	JSValues registers(32);
	static std::map<String, JSValue> globals;
	
	InstructionIterator pc = iCode.begin();
    while (pc != iCode.end()) {
        Instruction* instruction = *pc;
	    switch (instruction->opcode()) {
		case MOVE_TO:
			{
				Move* i = static_cast<Move*>(instruction);
				registers[op2(i)] = registers[op1(i)];
			}
			break;
                case LOAD_NAME:
			{
				LoadName* i = static_cast<LoadName*>(instruction);
				registers[op2(i)] = globals[*op1(i)];
			}
			break;
		case SAVE_NAME:
			{
				SaveName* i = static_cast<SaveName*>(instruction);
				globals[*op1(i)] = registers[op2(i)];
			}
			break;
		case LOAD_IMMEDIATE:
			{
				LoadImmediate* i = static_cast<LoadImmediate*>(instruction);
				registers[op2(i)] = JSValue(op1(i));
			}
			break;
		case LOAD_VAR:
			{
				LoadVar* i = static_cast<LoadVar*>(instruction);
				registers[op2(i)] = frame[op1(i)];
			}
			break;
		case SAVE_VAR:
			{
				SaveVar* i = static_cast<SaveVar*>(instruction);
				frame[op1(i)] = registers[op2(i)];
			}
			break;
		case BRANCH:
			{
				Branch* i = static_cast<Branch*>(instruction);
				pc = iCode.begin() + op1(i);
				continue;
			}
			break;
		case BRANCH_LT:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 < 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case BRANCH_LE:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 <= 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case BRANCH_EQ:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 == 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case BRANCH_NE:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 != 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case BRANCH_GE:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 >= 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case BRANCH_GT:
			{
				BranchCond* i = static_cast<BranchCond*>(instruction);
				if (registers[op2(i)].i32 > 0) {
					pc = iCode.begin() + op1(i);
					continue;
				}
			}
			break;
		case ADD:
			{
				// could get clever here with Functional forms.
				Arithmetic* i = static_cast<Arithmetic*>(instruction);
				registers[op3(i)] = JSValue(registers[op1(i)].f64 + registers[op2(i)].f64);
			}
			break;
		case SUBTRACT:
			{
				// could get clever here with Functional forms.
				Arithmetic* i = static_cast<Arithmetic*>(instruction);
				registers[op3(i)] = JSValue(registers[op1(i)].f64 - registers[op2(i)].f64);
			}
			break;
		case MULTIPLY:
			{
				// could get clever here with Functional forms.
				Arithmetic* i = static_cast<Arithmetic*>(instruction);
				registers[op3(i)] = JSValue(registers[op1(i)].f64 * registers[op2(i)].f64);
			}
			break;
		case DIVIDE:
			{
				// could get clever here with Functional forms.
				Arithmetic* i = static_cast<Arithmetic*>(instruction);
				registers[op3(i)] = JSValue(registers[op1(i)].f64 / registers[op2(i)].f64);
			}
			break;
		case COMPARE_LT:
		case COMPARE_LE:
		case COMPARE_EQ:
		case COMPARE_NE:
		case COMPARE_GT:
		case COMPARE_GE:
			{
				Arithmetic* i = static_cast<Arithmetic*>(instruction);
				float64 diff = (registers[op1(i)].f64 - registers[op2(i)].f64);
				registers[op3(i)].i32 = (diff == 0.0 ? 0 : (diff > 0.0 ? 1 : -1));
			}
			break;
		case NOT:
			{
				Move* i = static_cast<Move*>(instruction);
				registers[op2(i)].i32 = !registers[op1(i)].i32;
			}
			break;
		case RETURN:
		    {
		        Return* i = static_cast<Return*>(instruction);
		        result = registers[op1(i)];
		        pc = iCode.end();
		        continue;
		    }
        default:
            break;
    	}
    	
    	// increment the program counter.
    	++pc;
    }

	return result;
}

}
