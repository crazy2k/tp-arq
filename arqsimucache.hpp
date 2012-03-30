#include <stdio.h>
#include <list>
#include <cmath>
#include <sstream>
#include <fstream>
#include "pin.H"


int log2(int n);
string uint_to_string(UINT64 n);
string double_to_string(double n);


class Line {
    private:
        UINT64 tag;
    
    public:
        Line(UINT64 ptag = 0);

        UINT64 get_tag();
};

class Set {
    private:
        UINT64 ways;
        list<Line> lines;

    public:
        Set(UINT64 nways = 1);

        bool is_present(UINT64 tag);
        bool is_full();
        UINT64 unload_tag();
        VOID load_tag(UINT64 tag);
};

class Memory {        
    public:
        virtual VOID read(VOID *addr) = 0;
        virtual VOID write(VOID *addr) = 0;
        virtual VOID output(std::ostream *outstream) = 0;
};

class RAM : public Memory {       
    public:
        virtual VOID read(VOID *addr);
        virtual VOID write(VOID *addr);
        virtual VOID output(std::ostream *outstream);
};

class Cache : public Memory {
    private:
        string description;
        Memory *next;
        vector<Set> sets;
        int  ways, line_len, size;
        int reads, writes, read_hits, write_hits;
        
        UINT64 index_len();
        UINT64 index_mask();
        UINT64 offset_len();
        VOID* make_addr(UINT64 tag, UINT64 index);
        UINT64 get_index(VOID *addr);
        UINT64 get_tag(VOID *addr);

    public:
        Cache(string pdescription = "", Memory *pnext = NULL,
            int psize = 4*1024, int pways = 1, int pline_len = 8);

        virtual VOID read(VOID *addr);
        virtual VOID write(VOID *addr);
        virtual VOID output(std::ostream *outstream);
};
