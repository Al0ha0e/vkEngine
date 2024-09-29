#ifndef IDALLOC_H
#define IDALLOC_H

#include <optional>
#include <vector>

namespace vke_ds
{
    using id32_t = uint32_t;
    using id64_t = uint64_t;

    template <typename T>
    class NaiveIDAllocator
    {
    public:
        T id;
        NaiveIDAllocator() : id(0) {}
        NaiveIDAllocator(T st) : id(st) {}

        T Alloc()
        {
            return id++;
        }

        void SetIDMonoInc(T x)
        {
            id = std::max(id, x);
        }
    };

    template <typename T>
    class DynamicIDAllocator
    {
    public:
        size_t top;
        T maxID;

        DynamicIDAllocator() : top(0), maxID(0) {}

        DynamicIDAllocator(size_t cap) : ids(cap)
        {
            for (int i = 0; i < cap; i++)
                ids[i] = i;
            top = cap;
            maxID = cap - 1;
        }

        T Alloc()
        {
            if (top == 0)
            {
                maxID = ids.size();
                ids.push_back(maxID);
                return maxID;
            }
            return ids[--top];
        }

        void Free(T id)
        {
            ids[top++] = id;
        }

    private:
        std::vector<T> ids;
    };
}

#endif