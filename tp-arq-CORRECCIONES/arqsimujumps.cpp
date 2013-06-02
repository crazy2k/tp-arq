#include "arqsimujumps.hpp"

std::ofstream outfile;
list<Predictor*> predictors;

static VOID analyze_condbranch(VOID *ip, VOID *target, bool taken) {
    list<Predictor*>::iterator it;
    for (it = predictors.begin(); it != predictors.end(); it++) {
        Predictor *predictor = *it;
        predictor->analyze(ip, target, taken);
    }
}

static VOID instrument_instruction(INS ins, VOID *v) {
    // we only care about conditional branches
    if (!(INS_IsBranchOrCall(ins) && INS_HasFallThrough(ins)))
        return;

    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)analyze_condbranch,
        IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
}

static VOID finalize(INT32 code, VOID *v) {
    list<Predictor*>::iterator it;
    for (it = predictors.begin(); it != predictors.end(); it++) {
        Predictor *predictor = *it;
        predictor->output(&outfile);
    }
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


    outfile.open("arqsimujumps.out");

    INS_AddInstrumentFunction(instrument_instruction, 0);
    PIN_AddFiniFunction(finalize, 0);

    predictors.push_back(new AlwaysJumpPredictor());
    predictors.push_back(new NeverJumpPredictor());
    predictors.push_back(new JumpIfTargetIsLowerPredictor());
    predictors.push_back(new HistoryPredictor(1));
    predictors.push_back(new HistoryPredictor(2));

    // start program and never return
    PIN_StartProgram();
    
    return 0;
}
