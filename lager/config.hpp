//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#pragma once

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define LAGER_HAS_CPP17 1
#endif

#if defined(_MSC_VER) && !defined(LAGER_DISABLE_STORE_DEPENDENCY_CHECKS)
#define LAGER_DISABLE_STORE_DEPENDENCY_CHECKS
#endif

#if !defined(LAGER_USE_EXCEPTIONS) && !defined(LAGER_NO_EXCEPTIONS)
#if defined(_MSC_VER)
#if !_HAS_EXCEPTIONS
#define LAGER_NO_EXCEPTIONS
#endif
#else
#if !__cpp_exceptions
#define LAGER_NO_EXCEPTIONS
#endif
#endif
#endif

#ifdef LAGER_NO_EXCEPTIONS
#define LAGER_TRY if (true)
#define LAGER_CATCH(expr) else
#define LAGER_THROW(expr)                                                      \
    do {                                                                       \
        assert(!#expr);                                                        \
        std::terminate();                                                      \
    } while (false)
#define LAGER_RETHROW
#else
#define LAGER_TRY try
#define LAGER_CATCH(expr) catch (expr)
#define LAGER_THROW(expr) throw expr
#define LAGER_RETHROW throw
#endif
