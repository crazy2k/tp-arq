#include <stdio.h>
#include <list>
#include <cmath>
#include <sstream>
#include <fstream>
#include <iostream>
#include "pin.H"


std::ofstream outfile;

int log2(int n) {
    return (log10(n))/(log10(2));
}

string uint_to_string(UINT64 n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}


class Line {
    private:
        UINT64 tag;
    
    public:
        Line(UINT64 ptag = 0) {
            tag = ptag;
        }
        
        UINT64 get_tag() {
            return tag;
        }
};

class Set {
    private:
        UINT64 ways;
        list<Line> lines;

    public:
        Set(UINT64 nways = 1) : ways(nways) {}

        bool is_present(UINT64 tag) {
            list<Line>::iterator it;
            for (it = lines.begin(); it != lines.end(); it++) {
                if (it->get_tag() == tag)
                    return true;
            }

            return false;
        }

        bool is_full() {
            return (lines.size() == ways);
        }

        UINT64 unload_tag() {
            UINT64 unloaded_tag = lines.front().get_tag();
            lines.pop_front();
            return unloaded_tag;
        }

        VOID load_tag(UINT64 tag) {
            // TODO: handle the case when set is full
            lines.push_back(tag);
        }
};

class Memory {        
    public:
        virtual VOID read(VOID *addr) = 0;
        virtual VOID write(VOID *addr) = 0;
        virtual VOID output(std::ostream *outstream) = 0;
};

class RAM : public Memory {       
    public:
        virtual VOID read(VOID *addr) {}
        virtual VOID write(VOID *addr) {}
        virtual VOID output(std::ostream *outstream) {}
};

class Cache : public Memory {
    private:
        Memory *next;
        vector<Set> sets;
        int  ways, line_len, size;
        int reads, writes, hits;
        
        UINT64 index_len() {
            return log2(size/(ways*line_len));
        }
        
        UINT64 index_mask() {
            return 0xFFFFFFFFFFFFFFFF >> (64 - index_len());
        }
        
        UINT64 offset_len() {
            return log2(line_len);
        }
        
        UINT64 make_addr(UINT64 tag, UINT64 index) {
            return ((tag << index_len()) | index) << offset_len();
        }
  
  
        UINT64 get_index(VOID *addr) {
            return ((UINT64)addr >> offset_len()) & index_mask();
        }

  
        UINT64 get_tag(VOID *addr) {
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
                    next->write((VOID *)make_addr(sets[index].unload_tag(), index));
                
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
                    next->write((VOID *)make_addr(sets[index].unload_tag(), index));
                
                next->read(addr);
                sets[index].load_tag(tag);
            }
        }

        virtual VOID output(std::ostream *outstream) {
            *outstream << "reads: " + uint_to_string(reads) << std::endl;
        }
};


std::ofstream outfile;
Memory *front_memory;

VOID rec_memread(VOID *ip, VOID *addr) {
    front_memory->read(addr);
}

VOID rec_memwrite(VOID * ip, VOID * addr) {
    front_memory->write(addr);
}

VOID instrument_instruction(INS ins, VOID *v) {
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

VOID finalize(INT32 code, VOID *v) {
    front_memory->output(&outfile);
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


    outfile.open("arqsimucache.out");

    INS_AddInstrumentFunction(instrument_instruction, 0);
    PIN_AddFiniFunction(finalize, 0);

    RAM ram;
    Cache l2(&ram, 256*1024, 2, 16);
    Cache l1(&l2, 8*1024, 2, 16);

    front_memory = &l1;

    // start program and never return
    PIN_StartProgram();
    
    return 0;
}
