#ifndef _HAL_TYPES_H_
#define _HAL_TYPES_H_

#include <stdint.h>
#include <stdbool.h>


typedef void (*voidFuncPtr)(void);
typedef uint64_t Handler;

#define __attr_flash __attribute__((section (".USER_FLASH")))
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#define __io volatile
#define __deprecated __attribute__((__deprecated__))
#define __weak __attribute__((weak))
#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif
#ifndef __unused
#define __unused __attribute__((unused))
#endif

#ifndef NULL
#define NULL 0
#endif

#endif

