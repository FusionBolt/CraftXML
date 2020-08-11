//
// Created by fusionbolt on 2020/8/10.
//

#ifndef CRAFT_MEMORYPOOL_HPP
#define CRAFT_MEMORYPOOL_HPP

#include <array>

namespace Craft
{
    // page size often 4096
    template<typename T, unsigned BlockSize = 4096>
    class MemoryPool : public std::allocator<T>
    {
    public:
        using value_type = T;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = const value_type*;
        using const_reference = const value_type&;
        using size_type = std::size_t;
        using difference_type = ptrdiff_t;
        using mem_type = unsigned char*;
        using propagate_on_container_copy_assignment = std::false_type;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;

        MemoryPool() : _currentBlock(new MemBlock()), _freeList(nullptr) {}

        ~MemoryPool()
        {
            auto *currentBlock = _currentBlock;
            while (currentBlock->next != _currentBlock)
            {
                auto *prevBlock = currentBlock;
                currentBlock = currentBlock->next;
                delete prevBlock;
            }
            delete currentBlock;
        }

        MemoryPool(const MemoryPool &) = delete;
        MemoryPool& operator=(const MemoryPool &) = delete;

        MemoryPool(MemoryPool &&rhs) noexcept
        {
            swap(this, rhs);
        }

        MemoryPool& operator=(MemoryPool &&rhs) noexcept
        {
            swap(this, rhs);
            return *this;
        }

        template <typename U>
        struct rebind
        {
            using other = MemoryPool<U>;
        };

        struct MemBlock
        {
            MemBlock()
            {
                next = this;
                memBlock =
                    reinterpret_cast<mem_type>(operator new(BlockSize));
                // align
                currentPosition = memBlock + BlockSize % sizeof(T);
                lastPosition = memBlock + BlockSize;
            }

            ~MemBlock() { operator delete(memBlock);}

            MemBlock *next;

            mem_type memBlock;

            mem_type currentPosition;

            mem_type lastPosition;

            bool IsFull()
            {
                return currentPosition == lastPosition;
            }

            pointer Allocate()
            {
                auto *p = currentPosition;
                currentPosition += sizeof(T);
                return reinterpret_cast<pointer>(p);
            }
        };

        template<typename... Args>
        pointer New(Args &&... args)
        {
            auto p = allocate();
            construct(p, std::forward<Args>(args)...);
            return p;
        }

        void Delete(pointer p) noexcept
        {
            if(p != nullptr)
            {
                destroy(p);
                deallocate(p);
            }
        }

        size_type max_size() const noexcept
        {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }

        pointer address(reference x) const noexcept
        {
            return &x;
        }

        const_pointer address(const_reference x) const noexcept
        {
            return &x;
        }

        pointer allocate(size_type n = 1, const_pointer hint = 0)
        {
            if(_freeList != nullptr)
            {
                auto *p = _freeList;
                _freeList = _freeList->next;
                return reinterpret_cast<pointer>(p);
            }
            else
            {
                if(_currentBlock->IsFull())
                {
                    _AllocNewBlock(_currentBlock);
                    _currentBlock = _currentBlock->next;
                }
                return _currentBlock->Allocate();
            }
        }

        void deallocate(pointer p, size_type n = 1)
        {
            if(p != nullptr)
            {
                reinterpret_cast<FreeNode*>(p)->next = _freeList;
                _freeList = reinterpret_cast<FreeNode*>(p);
            }
        }

        template<typename... Args>
        void construct(pointer p, Args &&... args)
        {
            new (p) T(std::forward<Args>(args)...);
        }

        void destroy(pointer p)
        {
            p->~T();
        }

    private:
        MemBlock *_currentBlock;

        union FreeNode{
            T val;
            FreeNode *next;
        };
        FreeNode *_freeList;

        void swap(MemoryPool& lhs, MemoryPool& rhs)
        {
            std::swap(_currentBlock);
            std::swap(_freeList);
        }

        // 申请的内存不够时再增加新的block
        void _AllocNewBlock(MemBlock *currentBlock)
        {
            auto *block = new MemBlock();
            currentBlock->next = block;
        }
    };
} // namespace Craft
#endif // CRAFT_MEMORYPOOL_HPP
