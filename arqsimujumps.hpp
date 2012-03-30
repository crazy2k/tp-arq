#include "arqsimucommons.h"


class Predictor {
    private:
        string description;

    protected:
        UINT64 predictions;
        UINT64 hits;

    public:
        Predictor(string pdescription = "");
        virtual VOID analyze(VOID *ip, VOID *target, bool taken) = 0;
        virtual VOID output(std::ostream *outstream);
};


class NeverJumpPredictor : public Predictor {
    public:
        NeverJumpPredictor();
        virtual VOID analyze(VOID *ip, VOID *target, bool taken);
};


class AlwaysJumpPredictor : public Predictor {
    public:
        AlwaysJumpPredictor();

        virtual VOID analyze(VOID *ip, VOID *target, bool taken);
};


class JumpIfTargetIsLowerPredictor : public Predictor {
    public:
        JumpIfTargetIsLowerPredictor();

        virtual VOID analyze(VOID *ip, VOID *target, bool taken);
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
        HistoryPredictor(UINT64 pbits = 1);

        virtual VOID analyze(VOID *ip, VOID *target, bool taken);
};


//Predictor methods
Predictor::Predictor(string pdescription) : description(pdescription) {}

VOID Predictor::output(std::ostream *outstream) {
    *outstream << "=====" << std::endl;
    *outstream << description << std::endl;
    *outstream << "\thits/predictions: " <<
        uint_to_string(hits) << " / " <<
        uint_to_string(predictions) << " = " <<
        double_to_string(hits/(double)predictions) << std::endl;
}


//NeverJumpPredictor methods
NeverJumpPredictor::NeverJumpPredictor() : Predictor("Never Jump Predictor") {
}

VOID NeverJumpPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    if (!taken)
        hits++;
}


//AlwaysJumpPredictor methods
AlwaysJumpPredictor::AlwaysJumpPredictor() :
    Predictor("Always Jump Predictor") {}

VOID AlwaysJumpPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    if (taken)
        hits++;
}


//JumpIfTargetIsLowerPredictor methods
JumpIfTargetIsLowerPredictor::JumpIfTargetIsLowerPredictor() :
    Predictor("Jump If Target Is Lower Predictor") {}

VOID JumpIfTargetIsLowerPredictor::analyze(VOID *ip, VOID *target, bool taken) {
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


//HistoryPredictor methods
HistoryPredictor::HistoryPredictor(UINT64 pbits) :
    Predictor("History Predictor - " + uint_to_string(pbits) +
        "bits"), bits(pbits) {}

VOID HistoryPredictor::analyze(VOID *ip, VOID *target, bool taken) {
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


