// A basic mempool for preallocated objects of type T. eRPC has a faster,
// hugepage-backed one.
template <class T>
class AppMemPool
{
public:
    size_t num_to_alloc = 1;
    std::vector<T *> backing_ptr_vec;
    std::vector<T *> pool;

    void extend_pool()
    {
        T *backing_ptr = new T[num_to_alloc];
        for (size_t i = 0; i < num_to_alloc; i++)
            pool.push_back(&backing_ptr[i]);
        backing_ptr_vec.push_back(backing_ptr);
        num_to_alloc *= 2;
    }

    T *alloc()
    {
        if (pool.empty())
            extend_pool();
        T *ret = pool.back();
        pool.pop_back();
        return ret;
    }

    void free(T *t) { pool.push_back(t); }

    AppMemPool() {}
    ~AppMemPool()
    {
        for (T *ptr : backing_ptr_vec)
            delete[] ptr;
    }
};