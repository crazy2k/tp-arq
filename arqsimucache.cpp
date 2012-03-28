#include <stdio.h>
#include <cmath>
#include "pin.H"

int log2(int n) {
    return (log10(n))/(log10(2));
}

class Line {
    private:
        UINT64 tag;
        bool flag_present;
    
    public:
        Line(UINT64 ptag = 0) {
            tag = ptag;
            flag_present = false;
        }
        
        bool is_present() {
            return flag_present;
        }
        
        UINT64 get_tag() {
            return tag;
        }
};

class Set {
    private:
        int ways;
        list<Line> lines;
        
    public:
        Set(int nways = 1) : ways(nways), list(lines) {}
        
        bool is_present(UINT64 tag) {
            for (int i = 0; (UINT64)i < ways.size(); i++)
                if (ways[i].is_present() && ways[i].get_tag() == tag)
                    return true;

            return false;
        }
        
        bool is_full() {
            for (int i = 0; (UINT64)i < ways.size(); i++)
                if (!ways[i].is_present())
                    return false;

            return true;
        }
        
        VOID unload_tag() {
            
        }
};

class Memory {        
        virtual VOID read(VOID *addr) = 0;
        virtual VOID write(VOID *addr) = 0;
};

class RAM : Memory {       
        virtual VOID read(VOID *addr) {};
        virtual VOID write(VOID *addr) {};
};

class Cache : Memory {
    private:
        Memory* next;
        vector<Set> sets;
        int  ways, line_len, size;
        int reads, writes, hits;
        
        UINT64 index_len() {
            return log2(size/(ways*line_len));
        }
        
        UINT64 index_mask() {
            return 0xFFFFFFFF >> (64 - index_len());
        }
        
        UINT64 offset_len() {
            return log2(line_len);
        }
        
        UINT64 make_addr(UINT64 tag, UINT64 index) {
            return ((tag << index_len()) | index) << offset_len();
        }
  
  
        UINT64 get_tag(VOID *addr) {
            return ((UINT64)addr >> offset_len()) & index_mask();
        }

  
        UINT64 get_index(VOID *addr) {
            return (UINT64)addr >> (offset_len() + index_len());
        }


    public:
        Cache(Memory *pnext = NULL, int psize = 4*1024, int pways = 1,
            int pline_len = 8) : next(pnext), ways(pways), line_len(pline_len),
            size(psize) {
             
            int sets_number = size/(ways*line_len);
             
            for (int i = 0; i < sets_number; i++)
                sets.push_back(Set(ways));

        }

        virtual VOID read(VOID *addr) {
            reads++;
            UINT64 tag = get_tag(addr), index = get_index(addr);
                       
            if (sets[index].is_present(tag)) {
                hits++;
            } else {
                if (sets[index].is_full())
                    next->write(make_addr(sets[index].unload_tag(), index));
                
                next->read(addr);                
                sets[index].load_tag(tag);
            }
        }
        
        virtual VOID write(VOID *addr) {
            writes++;
            UINT64 tag = get_tag(addr), index = get_index(addr);        
            
            if (sets[index].is_present(tag)) {
                hits++;
            } else {
                if (sets[index].is_full())
                    next->write(make_addr(sets[index].unload_tag(), index));
                
                next->read(addr);
                sets[index].load_tag(tag);
            }
        }
};



FILE * trace;

VOID rec_memread(VOID * ip, VOID * addr) {
    fprintf(trace,"%p: R %p\n", ip, addr);
}

VOID rec_memwrite(VOID * ip, VOID * addr) {
    fprintf(trace,"%p: W %p\n", ip, addr);
}

VOID instrument_instruction(INS ins, VOID *v) {
    UINT32 memops = INS_MemoryOperandCount(ins);

    for (UINT32 memop = 0; memop < memops; memop++) {
        if (INS_MemoryOperandIsRead(ins, memop)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)rec_memread, IARG_INST_PTR,
                IARG_MEMORYOP_EA, memop, IARG_END);
        }

        if (INS_MemoryOperandIsWritten(ins, memop)) {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE, (AFUNPTR)rec_memwrite, IARG_INST_PTR,
                IARG_MEMORYOP_EA, memop, IARG_END);
        }
    }
}

VOID finalize(INT32 code, VOID *v)
{
    fprintf(trace, "#eof\n");
    fclose(trace);
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

    trace = fopen("arqsimucache.out", "w");

    INS_AddInstrumentFunction(instrument_instruction, 0);
    PIN_AddFiniFunction(finalize, 0);

    // start program and never return
    PIN_StartProgram();
    
    return 0;
}
