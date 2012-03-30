#include "arqsimucache.hpp"
#include "arqsimujumps.hpp"


static std::ofstream outfile;
static Memory *front_memory;
static UINT64 cycles;
static UINT64 instrs;
static Predictor *predictor;

static VOID rec_memread(VOID *ip, VOID *addr) {
    cycles += front_memory->read(addr);
}

static VOID analyze_condbranch(VOID *ip, VOID *target, bool taken) {
    if (!(predictor->analyze(ip, target, taken)))
        cycles += 5;
}

static VOID instrument_instruction(INS ins, VOID *v) {
    instrs++;

    if (INS_IsMemoryRead(ins)) {
        UINT32 memops = INS_MemoryOperandCount(ins);

        for (UINT32 memop = 0; memop < memops; memop++) {
            if (INS_MemoryOperandIsRead(ins, memop)) {
                INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)rec_memread,
                    IARG_INST_PTR, IARG_MEMORYOP_EA, memop, IARG_END);
            }
        }
    }
    else if (INS_IsBranchOrCall(ins) && INS_HasFallThrough(ins)) {
        INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)analyze_condbranch,
            IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
    }
    else {
        // if the instruction doesn't either read memory or is a conditional
        // branch, it counts as one cycle (writes are assumed to eat one cycle
        // too)
        cycles++;
    }
}

static VOID finalize(INT32 code, VOID *v) {
    outfile << "\tcycles/instructions: " <<
        uint_to_string(cycles) << " / " << uint_to_string(instrs) <<
        " = " << double_to_string(cycles/(double)instrs) <<
        std::endl;

    outfile.close();
}

static INT32 usage() {
    PIN_ERROR("This Pintool simulates a cache hierarchy\n" +
        KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
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

    front_memory = l1;

    predictor = new HistoryPredictor(2);

    cycles = 0;
    instrs = 0;

    // start program and never return
    PIN_StartProgram();
    
    return 0;
}
