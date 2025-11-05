#ifndef MEM_WITH_LEN_H
#define MEM_WITH_LEN_H

namespace vke_ds
{
    class Memory
    {
    public:
        unsigned char *data;
        size_t size;

        Memory() : data(nullptr), size(0) {}
        Memory(size_t size) : size(size) { data = new unsigned char[size]; }

        Memory &operator=(const Memory &) = delete;
        Memory(const Memory &) = delete;

        Memory &operator=(Memory &&ano)
        {
            if (this != &ano)
            {
                data = ano.data;
                size = ano.size;
                ano.data = nullptr;
                ano.size = 0;
            }
            return *this;
        }

        Memory(Memory &&ano) : data(ano.data), size(ano.size)
        {
            ano.data = nullptr;
            ano.size = 0;
        }

        ~Memory()
        {
            if (data != nullptr)
                delete[] data;
        }
    };
}

#endif