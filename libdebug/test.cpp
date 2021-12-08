#include "Debug.hpp"

int log_test() {
    LOG_INFO("test");
    LOG_DEBUG("error test");
    return 0;
}

int main(void) {
    auto debug = Debug::getInstance();
    debug->init(LOG_SO | LOG_IM | LOG_DM | LOG_EM, 1, 10, "/root/q-kms/branch/pilot/log/logtest");

    log_test();

    return 0;
}
