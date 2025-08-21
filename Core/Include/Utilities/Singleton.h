#ifndef SINGLETON_H
#define SINGLETON_H

#include "Asserts.h"
#include "Defines.h"

namespace Utility{
    template <class T> 
    class Singleton {
        public:
        static void Init() {
            AQUILA_CORE_ASSERT(!s_Instance && "Singleton already initialized!");
            s_Instance = new T();
        }

        static T* Get(){
            AQUILA_CORE_ASSERT(s_Instance && "Instance should exist before accessing");
            return s_Instance;
        }

        static void Shutdown(){
            AQUILA_CORE_ASSERT(s_Instance && "Instance should exist before deleting");
            delete s_Instance;
            s_Instance = nullptr;
        }


        protected:
        Singleton() {};
        ~Singleton() {}; 

        private:
        static inline T* s_Instance = nullptr;
        AQUILA_NONMOVEABLE(Singleton);
    };
}
 


#endif // SINGLETON_H