#pragma once

#ifdef __x86_64__
    #define CT_ENV_VAR_DOTNET_PATH L"__CT_DOTNET_PATH_x64"
#elif defined __i386__
    #define CT_ENV_VAR_DOTNET_PATH L"__CT_DOTNET_PATH_x86"
#else
    #define CT_ENV_VAR_DOTNET_PATH L""
    #error bitness error
#endif

#define CT_ENV_VAR_HOSTFXR_PATH L"__CT_HOSTFXR_PATH"
#define CT_ENV_VAR_PRODUCT_PATH L"__CT_PRODUCT_PATH"
