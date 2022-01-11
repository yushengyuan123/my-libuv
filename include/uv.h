struct uv_loop_s {
    void* data;
    unsigned int active_handles;
    void *wq[2];
    struct {                                                                    \
        void* min;                                                                \
        unsigned int nelts;                                                       \
    } timer_heap;
};

typedef uv_loop_s uv_loop_t