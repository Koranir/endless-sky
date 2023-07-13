#ifndef TRIPLE_BUFFER_H_
#define TRIPLE_BUFFER_H_

#include <atomic>
#include <mutex>

template <typename T>
class TripleBuffer {
public:
    std::mutex isReadingBuffer;
    int_fast8_t readingBuffer;
    T buffers[3];

    inline TripleBufferAccess GetReadBuffer() {
        return TripleBufferAccess(this);
    }

    class TripleBufferAccess {
    public:
        TripleBufferAccess(TripleBuffer* tripleBuffer) {
            lockGuard = std::lock_guard(tripleBuffer->isReadingBuffer);
            
        }
        ~TripleBufferAccess() {
            
        }
    private:
        T &buffer;
        TripleBuffer *parent;
        std::lock_guard lockGuard;
    }
};

#endif