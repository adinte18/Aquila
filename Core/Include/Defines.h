#ifndef AQUILA_DEFINES_H
#define AQUILA_DEFINES_H

/* Enable asserts by default - @todo i should maybe change this or condition it*/
#define AQUILA_ENABLE_ASSERTS

#define AQUILA_OUT(x) (std::cout << x << std::endl)

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

#endif