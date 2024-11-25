#include <agile_modbus.h>
#include <agile_modbus_slave_util.h>
#include <string.h>
#include <stdint.h>

static uint16_t hold_regs[10] = {0};

static int get_map_test(const agile_modbus_slave_util_map_t *map, void *buf, int bufsz) {
    uint16_t *ptr = (uint16_t *)buf;
    for (int i = 0; i < 10; i++) {
        *ptr++ = hold_regs[i];
    }

    return 0;
}

static int set_map_test(const agile_modbus_slave_util_map_t *map, int index, int len, void *buf, int bufsz) {
    uint16_t *ptr = (uint16_t *)buf;

    for (int i = 0; i < len; i++) {
        uint16_t data = ptr[index + i];
        if ((index + i >= 0) && (index + i <= 9)) {
            hold_regs[index + i] = data;
        }
    }

    return 0;
}

static const agile_modbus_slave_util_map_t _register_maps[] = {
    {0x0000, 0x0009, get_map_test, set_map_test}
};

const agile_modbus_slave_util_t mb_slave_util = {
    NULL, 0, NULL, 0,    _register_maps, sizeof(_register_maps) / sizeof(_register_maps[0]),
    NULL, 0, NULL, NULL, NULL};
