#ifndef USER_TA_HEADER_DEFINES_H
#define USER_TA_HEADER_DEFINES_H

#define TA_UUID                { 0x8f6f7b8a, 0x21a4, 0x4de8, { 0x9b, 0x8d, 0x7c, 0x0b, 0x96, 0x64, 0x8c, 0x19 } }
#define TA_FLAGS               (TA_FLAG_SINGLE_INSTANCE | TA_FLAG_INSTANCE_KEEP_ALIVE)
#define TA_STACK_SIZE          (2 * 1024)
#define TA_DATA_SIZE           (32 * 1024)
#define TA_VERSION             "1.0"
#define TA_DESCRIPTION         "BKK API key secure storage TA"
#define TA_CURRENT_TA_EXT_PROPERTIES \
    { "gp.ta.description", USER_TA_PROP_TYPE_STRING, TA_DESCRIPTION }, \
    { "gp.ta.version", USER_TA_PROP_TYPE_STRING, TA_VERSION }

#endif
