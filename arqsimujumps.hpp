#include "arqsimucommons.h"


class Predictor {
    private:
        string description;

    protected:
        UINT64 predictions;
        UINT64 hits;

    public:
        Predictor(string pdescription = "");
        virtual bool analyze(VOID *ip, VOID *target, bool taken) = 0;
        virtual VOID output(std::ostream *outstream);
};


class NeverJumpPredictor : public Predictor {
    public:
        NeverJumpPredictor();
        virtual bool analyze(VOID *ip, VOID *target, bool taken);
};


class AlwaysJumpPredictor : public Predictor {
    public:
        AlwaysJumpPredictor();

        virtual bool analyze(VOID *ip, VOID *target, bool taken);
};


class JumpIfTargetIsLowerPredictor : public Predictor {
    public:
        JumpIfTargetIsLowerPredictor();

        virtual bool analyze(VOID *ip, VOID *target, bool taken);
};

enum history_counter {N, n, t, T};

class HistoryPredictor : public Predictor {
    protected:
        map<UINT64, history_counter> history;

    public:
        HistoryPredictor(string pdescription = "");
};

class OneBitHistoryPredictor : public HistoryPredictor {
    public:
        OneBitHistoryPredictor();
        virtual bool analyze(VOID *ip, VOID *target, bool taken);
};

class TwoBitSaturationHistoryPredictor : public HistoryPredictor {
    public:
        TwoBitSaturationHistoryPredictor();
        virtual bool analyze(VOID *ip, VOID *target, bool taken);
};

class TwoBitHysteresisHistoryPredictor : public HistoryPredictor {
    public:
        TwoBitHysteresisHistoryPredictor();
        virtual bool analyze(VOID *ip, VOID *target, bool taken);
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


bool NeverJumpPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    if (!taken) {
        hits++;
        return true;
    }
    return false;
}


//AlwaysJumpPredictor methods
AlwaysJumpPredictor::AlwaysJumpPredictor() :
    Predictor("Always Jump Predictor") {}

bool AlwaysJumpPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    if (taken) {
        hits++;
        return true;
    }
    return false;
}


//JumpIfTargetIsLowerPredictor methods
JumpIfTargetIsLowerPredictor::JumpIfTargetIsLowerPredictor() :
    Predictor("Jump If Target Is Lower Predictor") {}

bool JumpIfTargetIsLowerPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    if (target < ip) {
        if (taken) {
            hits++;
            return true;
        }
    }
    else {
        if (!taken) {
            hits++;
            return true;
        }
    }
    return false;
}


HistoryPredictor::HistoryPredictor(string pdescription) :
    Predictor(pdescription) {}

OneBitHistoryPredictor::OneBitHistoryPredictor() :
    HistoryPredictor("1 Bit History Predictor") {}


bool OneBitHistoryPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    UINT64 uint_ip = (UINT64)ip;

    // create the record if it doesn't exist
    if (!history.count(uint_ip))
        history[uint_ip] = T;

    history_counter *hc = &history[uint_ip];

    if (((*hc == T) && taken) || ((*hc == N) && !taken)) {
        hits++;
        return true;
    } else {
        *hc = (*hc == T) ? N : T;
        return false;
    }
}

TwoBitSaturationHistoryPredictor::TwoBitSaturationHistoryPredictor() :
    HistoryPredictor("2 Bit Saturation History Predictor") {}

bool TwoBitSaturationHistoryPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    UINT64 uint_ip = (UINT64)ip;

    // create the record if it doesn't exist
    if (!history.count(uint_ip))
        history[uint_ip] = T;

    history_counter *hc = &history[uint_ip];

    if (*hc == T) {
        if (taken) {
            hits++;
            return true;
        }

        return false;
    }

    if (*hc == t) {
        if (taken) {
            hits++;
            *hc = T;
            return true;
        }

        *hc = n;
        return false;
    }

    if (*hc == n) {
        if (taken) {
            *hc = t;
            return false;
        }

        hits++;
        *hc = N;
        return true;
    }

    if (*hc == N) {
        if (taken) {
            *hc = n;
            return false;
        }

        hits++;
        return true;
    }

    // should never get here
    return false;

}

TwoBitHysteresisHistoryPredictor::TwoBitHysteresisHistoryPredictor() :
    HistoryPredictor("2 Bit Hysteresis History Predictor") {}

bool TwoBitHysteresisHistoryPredictor::analyze(VOID *ip, VOID *target, bool taken) {
    predictions++;

    UINT64 uint_ip = (UINT64)ip;

    // create the record if it doesn't exist
    if (!history.count(uint_ip))
        history[uint_ip] = T;

    history_counter *hc = &history[uint_ip];

    if (*hc == T) {
        if (taken) {
            hits++;
            return true;
        }

        return false;
    }

    if (*hc == t) {
        if (taken) {
            hits++;
            *hc = T;
            return true;
        }

        *hc = N;
        return false;
    }

    if (*hc == n) {
        if (taken) {
            *hc = T;
            return false;
        }

        hits++;
        *hc = N;
        return true;
    }

    if (*hc == N) {
        if (taken) {
            *hc = n;
            return false;
        }

        hits++;
        return true;
    }

    // should never get here
    return false;
}
