#include "heaptimer.h"

void HeapTimer::siftup_(size_t child) {
    assert(child > 0 && child < heap_.size());
    size_t parent = (child - 1) >> 1;
    while(parent >= 0) {
        if(heap_[parent] < heap_[child]) break;
        SwapNode_(child, parent);
        child = parent;
        parent = (child - 1) >> 1;
    } 
}

void HeapTimer::SwapNode_(size_t a, size_t b) {
    assert(a >= 0 && a < heap_.size());
    assert(b >= 0 && b < heap_.size());  
    std::swap(heap_[a], heap_[b]);
    ref_[heap_[a].id] = a;
    ref_[heap_[b].id] = b;
}

bool HeapTimer::siftdown_(size_t index, size_t n) {
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t parent = index;
    size_t child = index << 1 + 1;
    while(child < n){
        if(child + 1 < heap_.size() && heap_[child + 1] < heap_[child])
            ++child;
        if(heap_[parent] < heap_[child]) break;
        SwapNode_(child, parent);
        parent = child;
        child = parent << 1 + 1;
    }
    return parent > index;
}

void HeapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t insert_point;
    if(ref_.count(id) == 0) {
        //新节点，堆尾插入，调整堆
        insert_point = heap_.size();
        ref_[id] = insert_point;
        heap_.push_back({id, Clock::now() + MS(timeout), cb});
        siftup_(insert_point);
    }
    else {
        insert_point = ref_[id];
        heap_[insert_point].expires = Clock::now() + MS(timeout) ;
        heap_[insert_point].cb = cb;
        if(!siftdown_(insert_point, heap_.size())) {
            siftup_(insert_point);
        }
    }
}

void HeapTimer::doWork(int id) {
    
}