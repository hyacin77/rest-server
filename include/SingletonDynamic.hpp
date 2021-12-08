#ifndef __SINGLTONE_DYNAMIC_H
#define __SINGLTONE_DYNAMIC_H

#include <memory>
#include <stdlib.h>

template <typename T>
class SingletonDynamic {
    protected:
        SingletonDynamic() {}
        virtual ~SingletonDynamic() {}
    private:
        static std::shared_ptr<T>    instance;
    public:
        static std::shared_ptr<T> getInstance() {
            if(!instance) {
                instance = std::make_shared<T>();
                atexit(destroyInstance);
            }
            return instance;
        }
        static void destroyInstance() {
            if(instance) instance = nullptr;
        }
        static void setInstance(std::shared_ptr<T> newInstance) {
            instance = newInstance;
            atexit(destroyInstance);
        }
};

template <typename T> std::shared_ptr<T> SingletonDynamic<T>::instance = nullptr;

#endif
