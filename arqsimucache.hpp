#include "arqsimucommons.h"

#define ARQSIMUCACHE_RAMOH 8
#define ARQSIMUCACHE_L1OH 1
#define ARQSIMUCACHE_L2OH 2

class Line {
    private:
        UINT64 tag;
        bool dirty;
    
    public:
        Line(UINT64 ptag = 0);

        UINT64 get_tag();
        VOID mark_dirty();
        bool is_dirty();
};

class Set {
    private:
        UINT64 ways;
        list<Line> lines;

    public:
        Set(UINT64 nways = 1);

        bool is_present(UINT64 tag);
        bool is_full();
        Line *get_line(UINT64 tag);
        VOID load_line(Line line);
        Line unload_line();
};

class Memory {
    private:
        UINT64 overhead;

    public:
        Memory(UINT64 poverhead = 0);
        // read() and write() return the overhead in cycles of the operation
        virtual UINT64 read(VOID *addr);
        virtual UINT64 write(VOID *addr);
        virtual VOID output(std::ostream *outstream) = 0;
        virtual VOID set_overhead(UINT64 new_overhead);
        virtual UINT64 get_overhead();
};

class RAM : public Memory {       
    public:
        RAM();
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

        virtual UINT64 read(VOID *addr);
        virtual UINT64 write(VOID *addr);
        virtual VOID output(std::ostream *outstream);
};


//Line methods
Line::Line(UINT64 ptag) {
    tag = ptag;
    dirty = false;
}

UINT64 Line::get_tag() {
    return tag;
}

bool Line::is_dirty() {
    return dirty;
}

VOID Line::mark_dirty() {
    dirty = true;
}


//Set methods
Set::Set(UINT64 nways) : ways(nways) {}

bool Set::is_present(UINT64 tag) {
    return (get_line(tag) != NULL);
}

bool Set::is_full() {
    return (lines.size() == ways);
}


Line *Set::get_line(UINT64 tag) {
    list<Line>::iterator it;
    for (it = lines.begin(); it != lines.end(); it++) {
        if (it->get_tag() == tag)
            return &(*it);
    }
    return NULL;
}


Line Set::unload_line() {
    Line unloaded_line = lines.front();
    lines.pop_front();
    return unloaded_line;
}

VOID Set::load_line(Line line) {
    // TODO: handle the case when set is full
    lines.push_back(line);
}

//Memory methods
Memory::Memory(UINT64 poverhead) : overhead(poverhead) {}

UINT64 Memory::read(VOID *addr) {
    return overhead;
}

UINT64 Memory::write(VOID *addr) {
    return overhead;
}

VOID Memory::set_overhead(UINT64 new_overhead) {
    overhead = new_overhead;
}

UINT64 Memory::get_overhead() {
    return overhead;
}


//RAM methods
RAM::RAM() : Memory(ARQSIMUCACHE_RAMOH) {}
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

    UINT64 my_overhead = 0;
    if (description == "L1")
        my_overhead = ARQSIMUCACHE_L1OH;
    else if (description == "L2")
        my_overhead = ARQSIMUCACHE_L2OH;
    set_overhead(my_overhead);

    int sets_number = size/(ways*line_len);
     
    for (int i = 0; i < sets_number; i++)
        sets.push_back(Set(ways));

}

UINT64 Cache::read(VOID *addr) {
    reads++;
    UINT64 tag = get_tag(addr), index = get_index(addr);

    UINT64 total_overhead = 0;
    if (sets[index].is_present(tag)) {
        read_hits++;
    } else {
        if (sets[index].is_full()) {
            Line line = sets[index].unload_line();
            if (line.is_dirty()) {
                total_overhead +=
                    next->write(make_addr(line.get_tag(), index));
            }
        }
        
        total_overhead += next->read(addr);
        sets[index].load_line(Line(tag));
    }

    total_overhead += get_overhead();
    return total_overhead;
}

UINT64 Cache::write(VOID *addr) {
    writes++;
    UINT64 tag = get_tag(addr), index = get_index(addr);        
    
    UINT64 total_overhead = 0;
    if (sets[index].is_present(tag)) {
        write_hits++;
    } else {
        if (sets[index].is_full()) {
            Line line = sets[index].unload_line();
            if (line.is_dirty()) {
                total_overhead +=
                    next->write(make_addr(line.get_tag(), index));
            }
        }
        
        total_overhead += next->read(addr);
        sets[index].load_line(Line(tag));
    }

    sets[index].get_line(tag)->mark_dirty();

    total_overhead += get_overhead();
    return total_overhead;
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
