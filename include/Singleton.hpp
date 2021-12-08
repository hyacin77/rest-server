#ifndef __SINGLTONE_H
#define __SINGLTONE_H

template <typename T>
class Singleton {
    protected:
        Singleton() = default;
    private:
        Singleton(const Singleton&) = delete;
        Singleton& operator =(const Singleton&) = delete;
    public:
        static T& getInstance() {
            static T instance;
            return instance;
        }
};

#endif
