#pragma once

#define MUAECL_VERSION_MAJOR 2
#define MUAECL_VERSION_MINOR 0
#define MUAECL_VERSION_REVISION 8

#define MUAECL_VERSION_STRING_MAJOR _CRT_STRINGIZE(MUAECL_VERSION_MAJOR)
#define MUAECL_VERSION_STRING_MINOR _CRT_STRINGIZE(MUAECL_VERSION_MINOR)
#define MUAECL_VERSION_STRING_REVISION _CRT_STRINGIZE(MUAECL_VERSION_REVISION)
#define MUAECL_VERSION_STRING \
_CRT_CONCATENATE(_CRT_CONCATENATE(MUAECL_VERSION_STRING_MAJOR,_CRT_CONCATENATE(".",MUAECL_VERSION_STRING_MINOR)),_CRT_CONCATENATE(".",MUAECL_VERSION_STRING_REVISION))
