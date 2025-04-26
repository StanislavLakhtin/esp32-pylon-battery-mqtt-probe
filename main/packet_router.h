/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef PACKET_ROUTER_H
#define PACKET_ROUTER_H

#include <stddef.h>
#include <stdbool.h>

void packet_router_init(void);

void my_packet_handler(const char *ascii_packet, size_t len);

void packet_router_set_online(bool online);

#endif
