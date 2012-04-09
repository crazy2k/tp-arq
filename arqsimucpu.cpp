#include "arqsimucache.hpp"
#include "arqsimujumps.hpp"
#include <algorithm>

static VOID process_memread_wrap(VOID *ip, VOID *addr);

static VOID process_condbranch_wrap(VOID *ip, VOID *target, bool taken);

class CPU {
    private:
        UINT64 cycles;
        UINT64 instrs;

        Memory *front_memory;
        Predictor *predictor;

        // Mechanism that allows a certain amount of parallelism while reading
        // from memory
        list<REG> recent_regs;
        UINT64 spare_cycles;

        VOID consume_cycles(UINT64 n, bool can_exec_in_parallel) {
            if (n == 0)
                return;

            if (can_exec_in_parallel) {
                if (spare_cycles > n)
                    spare_cycles -= n;
                else
                    spare_cycles = 0;
            }
            // if it's a memory operation or another operation that reads
            // or writes to a register being used by a memory operation,
            // then it comes here
            else {
                // if that's the case, spare cycles have to be consumed 
                if (spare_cycles > 0)
                    cycles += spare_cycles;
            }

            cycles += n;
        }

        VOID process_memop(UINT64 op_cycles) {
            // if a memory operation is being done, we have to wait for it to
            // finish in order to perform the read
            consume_cycles(1, false);
            spare_cycles = op_cycles - 1;
        }

    public:
        CPU(Memory *pfront_memory, Predictor *ppredictor) :
            front_memory(pfront_memory), predictor(ppredictor) {
            cycles = 0;
            instrs = 0;
            spare_cycles = 0;
        }

        VOID process_memread(VOID *ip, VOID *addr) {
            process_memop(front_memory->read(addr));
        }

        VOID process_condbranch(VOID *ip, VOID *target, bool taken) {
            if (predictor->analyze(ip, target, taken))
                consume_cycles(1, true);
            else
                consume_cycles(5, true);
        }



        VOID process_instruction(INS ins) {
            instrs++;

            // if it's a memory read,
            if (INS_IsMemoryRead(ins)) {
                UINT32 memops = INS_MemoryOperandCount(ins);

                // process each address separately
                for (UINT32 memop = 0; memop < memops; memop++) {
                    if (INS_MemoryOperandIsRead(ins, memop)) {
                        INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                            (AFUNPTR)process_memread_wrap, IARG_INST_PTR,
                            IARG_MEMORYOP_EA, memop, IARG_END);
                    }
                }

                // record registers that are being written
                recent_regs.clear();
                UINT32 wregs = INS_MaxNumWRegs(ins);
                for (UINT32 wreg = 0; wreg < wregs; wreg++)
                    recent_regs.push_back(INS_RegW(ins, wreg));
            }
            // if it's a memory write,
            if (INS_IsMemoryWrite(ins)) {
                // only use one cycle (but since it's a memory operation, it
                // will have to wait for other memory operations to complete)
                process_memop(1);
            }
            else if (INS_IsBranchOrCall(ins) && INS_HasFallThrough(ins)) {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                    (AFUNPTR)process_condbranch_wrap, IARG_INST_PTR,
                    IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN,
                    IARG_END);
            }
            else {
                // if the instruction doesn't either read or write to memory
                // nor is a conditional branch, it consumes one cycle and can
                // be executed in parallel if it doesn't need a currently
                // being used register

                // check whether the instruction uses a register that's being
                // written to by a memory operation
                bool uses_recent_reg = false;

                UINT32 wregs = INS_MaxNumWRegs(ins);
                for (UINT32 reg = 0; reg < wregs; reg++)
                    if (count(recent_regs.begin(), recent_regs.end(),
                        INS_RegW(ins, reg)))
                        uses_recent_reg = true;

                UINT32 rregs = INS_MaxNumRRegs(ins);
                for (UINT32 reg = 0; reg < rregs; reg++)
                    if (count(recent_regs.begin(), recent_regs.end(),
                        INS_RegR(ins, reg)))
                        uses_recent_reg = true;

                // it can be excecuted in parallel if it doesn't
                consume_cycles(1, !uses_recent_reg);
            }

        }

        VOID output(std::ostream *outstream) {
            *outstream << "\tcycles/instructions: " <<
                uint_to_string(cycles) << " / " << uint_to_string(instrs) <<
                " = " << double_to_string(cycles/(double)instrs) <<
                std::endl;
        }
};

static INT32 usage() {
    PIN_ERROR("This Pintool simulates a cache hierarchy\n" +
        KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}

static std::ofstream outfile;
static CPU *cpu;

static VOID instrument_instruction(INS ins, VOID *v) {
    cpu->process_instruction(ins);
}

static VOID finalize(INT32 code, VOID *v) {
    cpu->output(&outfile);

    outfile.close();
}

static VOID process_memread_wrap(VOID *ip, VOID *addr) {
    cpu->process_memread(ip, addr);
}

static VOID process_condbranch_wrap(VOID *ip, VOID *target, bool taken) {
    cpu->process_condbranch(ip, target, taken);
}

int main(int argc, char *argv[])
{

    if (PIN_Init(argc, argv))
        return usage();

    outfile.open("arqsimucpu.out");

    INS_AddInstrumentFunction(instrument_instruction, 0);
    PIN_AddFiniFunction(finalize, 0);

    // create memory hierarchy
    RAM *ram = new RAM();
    Cache *l2 = new Cache("L2", ram, 1000*1024, 2, 16);
    Cache *l1 = new Cache("L1", l2, 64*1024, 2, 16);

    Predictor *predictor = new HistoryPredictor(2);

    cpu = new CPU(l1, predictor);

    // start program and never return
    PIN_StartProgram();
    
    return 0;
}
