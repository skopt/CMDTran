#ifndef _LOCK_FREE_QUEUE_H_
#define _LOCK_FREE_QUEUE_H_

#include <stdint.h>
#include <string.h>
#include <sched.h>

#define QUEUE_LEN 20*1024*1024
#define CAS(ptr, oldVal, newVal) __sync_bool_compare_and_swap((ptr), (oldVal), (newVal))
#define AtomicAdd(ptr,  cout) __sync_add_and_fetch((ptr),  (cout))
#define AtomicSub(ptr,  cout) __sync_sub_and_fetch((ptr),  (cout))

template <typename ELEM_T>
class CLockFreeQueue
{
public:
    CLockFreeQueue();
    ~CLockFreeQueue();
    bool Push(const ELEM_T &data);
    bool Pop(ELEM_T &data);
    uint32_t Size() const;
    bool IsEmpty() const;
    void Sleep() const;

private:
    ELEM_T m_Queue[QUEUE_LEN + 10];
    uint32_t m_writeIndex;
    uint32_t m_readIndex;
    uint32_t m_maximumReadIndex;
    uint32_t m_size;

private:
    uint32_t CountToIndex(uint32_t cout);
};

template <typename ELEM_T>
CLockFreeQueue<ELEM_T>::CLockFreeQueue()
:m_writeIndex(0), m_readIndex(0), m_maximumReadIndex(0), m_size(0)
{
    //memset(m_Queue, 0, QUEUE_LEN*sizeof(ELEM_T));
}

template <typename ELEM_T>
CLockFreeQueue<ELEM_T>::~CLockFreeQueue()
{
}

template <typename ELEM_T>
uint32_t CLockFreeQueue<ELEM_T>::CountToIndex(uint32_t cout)
{
    return (cout % QUEUE_LEN);
}

template <typename ELEM_T>
bool CLockFreeQueue<ELEM_T>::Push(const ELEM_T &data)
{
    uint32_t currentReadIndex;
    uint32_t currentWriteIndex;

    do
    {
        currentWriteIndex = m_writeIndex;
        currentReadIndex  = m_readIndex;
        if (CountToIndex(currentWriteIndex + 1) == CountToIndex(currentReadIndex))
        {
            // the queue is full
            return false;
        }

    } while (!CAS(&m_writeIndex, currentWriteIndex, (currentWriteIndex + 1)));

    // We know now that this index is reserved for us. Use it to save the data
    m_Queue[CountToIndex(currentWriteIndex)] = data;

    // update the maximum read index after saving the data. It wouldn't fail if there is only one thread 
    // inserting in the queue. It might fail if there are more than 1 producer threads because this
    // operation has to be done in the same order as the previous CAS

    while (!CAS(&m_maximumReadIndex, currentWriteIndex, (currentWriteIndex + 1)))
    {
        // this is a good place to yield the thread in case there are more
        // software threads than hardware processors and you have more
        // than 1 producer thread
        // have a look at sched_yield (POSIX.1b)
        sched_yield();
    }
    AtomicAdd(&m_size, 1);
    return true;
}

template <typename ELEM_T>
bool CLockFreeQueue<ELEM_T>::Pop(ELEM_T &data)
{
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;

    do
    {
        // to ensure thread-safety when there is more than 1 producer thread
        // a second index is defined (m_maximumReadIndex)
        currentReadIndex = m_readIndex;
        currentMaximumReadIndex = m_maximumReadIndex;

        if (CountToIndex(currentReadIndex) == CountToIndex(currentMaximumReadIndex))
        {
            // the queue is empty or
            // a producer thread has allocate space in the queue but is 
            // waiting to commit the data into it
            return false;
        }

        // retrieve the data from the queue
        data = m_Queue[CountToIndex(currentReadIndex)];

        // try to perfrom now the CAS operation on the read index. If we succeed
        // a_data already contains what m_readIndex pointed to before we 
        // increased it
        if (CAS(&m_readIndex, currentReadIndex, (currentReadIndex + 1)))
        {
            AtomicSub(&m_size,1);
            return true;
        }

        // it failed retrieving the element off the queue. Someone else must
        // have read the element stored at countToIndex(currentReadIndex)
        // before we could perform the CAS operation        

    } while(1); // keep looping to try again!

    // Add this return statement to avoid compiler warnings
    return false;
}

template <typename ELEM_T>
uint32_t CLockFreeQueue<ELEM_T>::Size() const
{
    return m_size;
}

template <typename ELEM_T>
bool CLockFreeQueue<ELEM_T>::IsEmpty() const
{
    return (m_size <= 0);
}

template <typename ELEM_T>
void CLockFreeQueue<ELEM_T>::Sleep() const
{
    sched_yield();
}
#endif