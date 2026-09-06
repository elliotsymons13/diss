#ifndef EVERYWHERE_COMMON_H
#define EVERYWHERE_COMMON_H

enum class err {
    none,
    timeout_expired, 
};

// TODO move to lower in include heirarchy - should not be needed by library users, abstracted away into internals?
enum class comm {
    mode_invalid = -1,
    mode_duplex = 0,
    mode_rx = 1,
    mode_tx = 2
};

#endif // EVERYWHERE_COMMON_H
