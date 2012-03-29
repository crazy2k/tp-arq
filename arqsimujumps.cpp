#include <stdio.h>
#include <list>
#include <map>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include "pin.H"


std::ofstream outfile;

string uint_to_string(UINT64 n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}

string double_to_string(double n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}


class Predictor {
    private:
        string description;

    protected:
        UINT64 predictions;
        UINT64 hits;

    public:
        Predictor(string pdescription = "") : description(pdescription) {}
        virtual VOID analyze(VOID *ip, VOID *target, bool taken) = 0;
        virtual VOID output(std::ostream *outstream) {
            *outstream << "=====" << std::endl;
            *outstream << description << std::endl;
            *outstream << "\thits/predictions: " <<
                uint_to_string(hits) << " / " <<
                uint_to_string(predictions) << " = " <<
                double_to_string(hits/(double)predictions) << std::endl;
        }

};


class NeverJumpPredictor : public Predictor {
    public:
        NeverJumpPredictor() : Predictor("Never Jump Predictor") {}
        virtual VOID analyze(VOID *ip, VOID *target, bool taken) {
            predictions++;

            if (!taken)
                hits++;
        }
};


class AlwaysJumpPredictor : public Predictor {
    public:
        AlwaysJumpPredictor() : Predictor("Always Jump Predictor") {}

        virtual VOID analyze(VOID *ip, VOID *target, bool taken) {
            predictions++;

            if (taken)
                hits++;
        }
};


class JumpIfTargetIsLowerPredictor : public Predictor {
    public:
        JumpIfTargetIsLowerPredictor() :
            Predictor("Jump If Target Is Lower Predictor") {}

        virtual VOID analyze(VOID *ip, VOID *target, bool taken) {
            predictions++;

            if (target < ip) {
                if (taken)
                    hits++;
            }
            else {
                if (!taken)
                    hits++;
            }
        }
};


class HistoryPredictor : public Predictor {
    private:
        struct instruction_history {
            bool current_prediction;
            UINT64 wrong;
        };
        UINT64 bits;
        map<UINT64, instruction_history> history;

    public:
        HistoryPredictor(UINT64 pbits = 1) :
            Predictor("History Predictor - " + uint_to_string(pbits) +
                "bits"), bits(pbits) {
        }

        virtual VOID analyze(VOID *ip, VOID *target, bool taken) {
            predictions++;

            UINT64 uint_ip = (UINT64)ip;

            // create the record if it doesn't exist
            if (!history.count(uint_ip)) {
                instruction_history new_ih;
                new_ih.current_prediction = true;
                new_ih.wrong = 0;
            
                history[uint_ip] = new_ih;
            }

            instruction_history *ih = &history[uint_ip];

            // if we mispredicted,
            if (taken != ih->current_prediction) {
                ih->wrong++;
                // if we have been wrong 'bits' times, we have to change our
                // mind for the future
                if (ih->wrong == bits)
                    ih->current_prediction = !(ih->current_prediction);
            }
            // if we predicted correctly,
            else {
                hits++;
                // we forget about the times we were wrong in a row
                // immediately before
                ih->wrong = bits;
            }
        }
};


list<Predictor*> predictors;

VOID analyze_condbranch(VOID *ip, VOID *target, bool taken) {
    list<Predictor*>::iterator it;
    for (it = predictors.begin(); it != predictors.end(); it++) {
        Predictor *predictor = *it;
        predictor->analyze(ip, target, taken);
    }
}

VOID instrument_instruction(INS ins, VOID *v) {
    // we only care about conditional branches
    if (!(INS_IsBranchOrCall(ins) && INS_HasFallThrough(ins)))
        return;

    INS_InsertPredicatedCall(ins, IPOINT_BEFORE, (AFUNPTR)analyze_condbranch,
        IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
}

VOID finalize(INT32 code, VOID *v) {
    list<Predictor*>::iterator it;
    for (it = predictors.begin(); it != predictors.end(); it++) {
        Predictor *predictor = *it;
        predictor->output(&outfile);
    }
    outfile.close();
}

INT32 usage() {
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
