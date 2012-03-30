#include "arqsimucache.hpp"


static std::ofstream outfile;
static Memory *front_memory;

static VOID rec_memread(VOID *ip, VOID *addr) {
    front_memory->read(addr);
}

static VOID rec_memwrite(VOID * ip, VOID * addr) {
    front_memory->write(addr);
}

static VOID instrument_instruction(INS ins, VOID *v) {
    UINT32 memops = INS_MemoryOperandCount(ins);

    for (UINT32 memop = 0; memop < memops; memop++) {
        if (INS_MemoryOperandIsRead(ins, memop)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)rec_memread,
                IARG_INST_PTR, IARG_MEMORYOP_EA, memop, IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memop)) {
            INS_InsertPredicatedCall(ins, IPOINT_BEFORE,
                (AFUNPTR)rec_memwrite, IARG_INST_PTR, IARG_MEMORYOP_EA, memop,
                IARG_END);
        }
    }
}

static VOID finalize(INT32 code, VOID *v) {
    front_memory->output(&outfile);
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


