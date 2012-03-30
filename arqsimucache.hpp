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


int log2(int n) {
    return (log10(n))/(log10(2));
}

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


//Line methods
Line::Line(UINT64 ptag) {
    tag = ptag;
}

UINT64 Line::get_tag() {
    return tag;
}


//Set methods
Set::Set(UINT64 nways) : ways(nways) {}

bool Set::is_present(UINT64 tag) {
    list<Line>::iterator it;
    for (it = lines.begin(); it != lines.end(); it++) {
        if (it->get_tag() == tag)
            return true;
    }

    return false;
}

bool Set::is_full() {
    return (lines.size() == ways);
}

UINT64 Set::unload_tag() {
    UINT64 unloaded_tag = lines.front().get_tag();
    lines.pop_front();
    return unloaded_tag;
}

VOID Set::load_tag(UINT64 tag) {
    // TODO: handle the case when set is full
    lines.push_back(tag);
}


//Ram methods
VOID RAM::read(VOID *addr) {}
VOID RAM::write(VOID *addr) {}
VOID RAM::output(std::ostream *outstream) {}


//Cache methods
UINT64 Cache::index_len() {
    return log2(size/(ways*line_len));
}

UINT64 Cache::index_mask() {
    return 0xFFFFFFFFFFFFFFFF >> (64 - index_len());
}

UINT64 Cache::offset_len() {
    return log2(line_len);
}

VOID* Cache::make_addr(UINT64 tag, UINT64 index) {
    return (VOID *)(((tag << index_len()) | index) << offset_len());
}


UINT64 Cache::get_index(VOID *addr) {
    return ((UINT64)addr >> offset_len()) & index_mask();
}


UINT64 Cache::get_tag(VOID *addr) {
    return (UINT64)addr >> (offset_len() + index_len());
}

Cache::Cache(string pdescription, Memory *pnext,
    int psize, int pways, int pline_len) :
    description(pdescription), next(pnext), ways(pways),
    line_len(pline_len), size(psize) {
     
    int sets_number = size/(ways*line_len);
     
    for (int i = 0; i < sets_number; i++)
        sets.push_back(Set(ways));

}

VOID Cache::read(VOID *addr) {
    reads++;
    UINT64 tag = get_tag(addr), index = get_index(addr);
               
    if (sets[index].is_present(tag)) {
        read_hits++;
    } else {
        if (sets[index].is_full()) {
            next->write(make_addr(sets[index].unload_tag(), index));
        }
        
        next->read(addr);                
        sets[index].load_tag(tag);
    }
}

VOID Cache::write(VOID *addr) {
    writes++;
    UINT64 tag = get_tag(addr), index = get_index(addr);        
    
    if (sets[index].is_present(tag)) {
        write_hits++;
    } else {
        if (sets[index].is_full()) {
            next->write(make_addr(sets[index].unload_tag(), index));
        }
        
        next->read(addr);
        sets[index].load_tag(tag);
    }
}

VOID Cache::output(std::ostream *outstream) {
    *outstream << "=====" << std::endl;
    *outstream << description << ":" << std::endl;

    *outstream << "\tread hits/reads: " <<
        uint_to_string(read_hits) << " / " << uint_to_string(reads) <<
        " = " << double_to_string(read_hits/(double)reads) <<
        std::endl;

    *outstream << "\twrite hits/writes: " <<
        uint_to_string(write_hits) << " / " <<
        uint_to_string(writes) << " = " <<
        double_to_string(write_hits/(double)writes) << std::endl;
    next->output(outstream);
}
