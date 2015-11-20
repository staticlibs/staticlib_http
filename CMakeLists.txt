# Copyright 2015, alex at staticlibs.net
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required ( VERSION 2.8.12 )

# project
project ( staticlib_httpclient CXX )
set ( ${PROJECT_NAME}_STATICLIB_VERSION 1.3.2 )
set ( ${PROJECT_NAME}_DESCRIPTION "Staticlibs HTTP client library" )
set ( ${PROJECT_NAME}_URL https://github.com/staticlibs/staticlib_httpclient )
include ( ${CMAKE_CURRENT_LIST_DIR}/resources/macros.cmake )

# docs
option ( ${PROJECT_NAME}_ENABLE_DOCS "Generate doxyfile and exit build" OFF )
if ( ${PROJECT_NAME}_ENABLE_DOCS )
    configure_file ( ${CMAKE_CURRENT_LIST_DIR}/resources/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile )
    return ( )
endif ( )

# check deplibs cache
if ( STATICLIB_USE_DEPLIBS_CACHE )
    set ( ${PROJECT_NAME}_CACHED_LIB_PATH ${STATICLIB_DEPLIBS_CACHE_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX} )
    if ( EXISTS ${${PROJECT_NAME}_CACHED_LIB_PATH} )
        add_library( ${PROJECT_NAME} STATIC IMPORTED GLOBAL )
        set_target_properties( ${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION ${${PROJECT_NAME}_CACHED_LIB_PATH} )
        message ( STATUS "Using cached library: [${${PROJECT_NAME}_CACHED_LIB_PATH}]" )
        return ( )
    endif ( )
endif ( )

# options, use SET ( OPTNAME ON CACHE BOOL "") in parent to override
option ( STATICLIB_WITH_ICU "Build with ICU support" OFF )

# deps
set ( ${PROJECT_NAME}_OPTIONS "" )
set ( ${PROJECT_NAME}_DEPS_PUBLIC
        staticlib_pimpl        
        staticlib_utils )
set ( ${PROJECT_NAME}_DEPS_PRIVATE 
        libcurl )
if ( STATICLIB_WITH_ICU )
    set ( ${PROJECT_NAME}_DEPS_PUBLIC ${${PROJECT_NAME}_DEPS_PUBLIC} icu-uc )
    set ( ${PROJECT_NAME}_OPTIONS -DSTATICLIB_WITH_ICU )
endif ( )
staticlib_httpclient_pkg_check_modules ( ${PROJECT_NAME}_DEPS_PUBLIC_PC REQUIRED ${PROJECT_NAME}_DEPS_PUBLIC )
staticlib_httpclient_pkg_check_modules ( ${PROJECT_NAME}_DEPS_PRIVATE_PC REQUIRED ${PROJECT_NAME}_DEPS_PRIVATE )

# library
file ( GLOB_RECURSE ${PROJECT_NAME}_SRC ${CMAKE_CURRENT_LIST_DIR}/src/*.cpp )
file ( GLOB_RECURSE ${PROJECT_NAME}_HEADERS ${CMAKE_CURRENT_LIST_DIR}/include/*.hpp )
add_library ( ${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SRC} ${${PROJECT_NAME}_HEADERS} )
source_group ( "src" FILES ${${PROJECT_NAME}_SRC} )
source_group ( "include" FILES ${${PROJECT_NAME}_HEADERS} )
target_include_directories ( ${PROJECT_NAME} BEFORE PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${${PROJECT_NAME}_DEPS_PUBLIC_PC_INCLUDE_DIRS}
        ${${PROJECT_NAME}_DEPS_PRIVATE_PC_INCLUDE_DIRS} )
target_compile_options ( ${PROJECT_NAME} PRIVATE 
        ${${PROJECT_NAME}_DEPS_PUBLIC_PC_CFLAGS_OTHER}
        ${${PROJECT_NAME}_DEPS_PRIVATE_PC_CFLAGS_OTHER}
        ${${PROJECT_NAME}_OPTIONS} )

# pkg-config
if ( NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY )
    set ( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
endif ( )
staticlib_httpclient_list_to_string ( ${PROJECT_NAME}_PC_CFLAGS_OPTS "" ${PROJECT_NAME}_OPTIONS )
set ( ${PROJECT_NAME}_PC_CFLAGS "-I${CMAKE_CURRENT_LIST_DIR}/include ${${PROJECT_NAME}_PC_CFLAGS_OPTS}" )
set ( ${PROJECT_NAME}_PC_LIBS "-L${CMAKE_LIBRARY_OUTPUT_DIRECTORY} -l${PROJECT_NAME}" )
staticlib_httpclient_list_to_string ( ${PROJECT_NAME}_PC_REQUIRES "" ${PROJECT_NAME}_DEPS_PUBLIC )
staticlib_httpclient_list_to_string ( ${PROJECT_NAME}_PC_REQUIRES_PRIVATE "" ${PROJECT_NAME}_DEPS_PRIVATE )
configure_file ( ${CMAKE_CURRENT_LIST_DIR}/resources/pkg-config.in 
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/pkgconfig/${PROJECT_NAME}.pc )
