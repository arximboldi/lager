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

// https://github.com/LowCostCustoms/cereal-inline/blob/master/cereal_inline.hpp

#pragma once

#include "cereal/cereal.hpp"
#include "cereal/details/traits.hpp"

#include <type_traits>

#ifndef CEREAL_LOAD_INLINE_FUNCTION_NAME
#define CEREAL_LOAD_INLINE_FUNCTION_NAME load_inline
#endif // CEREAL_LOAD_INLINE_FUNCTION_NAME

#ifndef CEREAL_SAVE_INLINE_FUNCTION_NAME
#define CEREAL_SAVE_INLINE_FUNCTION_NAME save_inline
#endif // CEREAL_SAVE_INLINE_FUNCTION_NAME

/**
 * @namespace cereal
 * Cereal serialization methods
 */
namespace cereal {

// you can use it later
#define PROCESS_IF(name)                                                       \
    traits::EnableIf<cereal::traits::has_##name<T, ArchiveType>::value> =      \
        traits::sfinae

/**
 * Loads `flat` cereal value (without need to process prologue and epilogue)
 */
template <class ArchiveType, class T, PROCESS_IF(member_serialize)>
void CEREAL_LOAD_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    value.CEREAL_SERIALIZE_FUNCTION_NAME(archive);
}

template <class ArchiveType, class T, PROCESS_IF(non_member_serialize)>
void CEREAL_LOAD_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    CEREAL_SERIALIZE_FUNCTION_NAME(archive, value);
}

template <class ArchiveType, class T, PROCESS_IF(member_load)>
void CEREAL_LOAD_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    value.CEREAL_LOAD_FUNCTION_NAME(archive);
}

template <class ArchiveType, class T, PROCESS_IF(non_member_load)>
void CEREAL_LOAD_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    CEREAL_LOAD_FUNCTION_NAME(archive, value);
}

/**
 * Saves `flat` cereal value (without prologue and epilogue)
 */
template <class ArchiveType, class T, PROCESS_IF(member_serialize)>
void CEREAL_SAVE_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    value.CEREAL_SERIALIZE_FUNCTION_NAME(archive);
}

template <class ArchiveType, class T, PROCESS_IF(non_member_serialize)>
void CEREAL_SAVE_INLINE_FUNCTION_NAME(ArchiveType& archive, T& value)
{
    CEREAL_SERIALIZE_FUNCTION_NAME(archive, value);
}

template <class ArchiveType, class T, PROCESS_IF(member_save)>
void CEREAL_SAVE_INLINE_FUNCTION_NAME(ArchiveType& archive, const T& value)
{
    value.CEREAL_SAVE_FUNCTION_NAME(archive);
}

template <class ArchiveType, class T, PROCESS_IF(non_member_save)>
void CEREAL_SAVE_INLINE_FUNCTION_NAME(ArchiveType& archive, const T& value)
{
    CEREAL_SAVE_FUNCTION_NAME(archive, value);
}

// nope ;)
#undef PROCESS_IF

} // namespace cereal
