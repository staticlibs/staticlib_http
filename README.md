HTTP client library for Staticlibs
==================================

[![travis](https://travis-ci.org/staticlibs/staticlib_http.svg?branch=master)](https://travis-ci.org/staticlibs/staticlib_http)
[![appveyor](https://ci.appveyor.com/api/projects/status/github/staticlibs/staticlib_http?svg=true)](https://ci.appveyor.com/project/staticlibs/staticlib-http)

This project is a part of [Staticlibs](http://staticlibs.net/).

This library is built on top of [libcurl](http://curl.haxx.se/libcurl/c/). 
It uses `curl_multi` API to implement streaming access to HTTP resources.

Link to the [API documentation](http://staticlibs.github.io/staticlib_http/docs/html/namespacestaticlib_1_1http.html).

How to build
------------

[CMake](http://cmake.org/) is required for building.

[pkg-config](http://www.freedesktop.org/wiki/Software/pkg-config/) utility is used for dependency management.
For Windows users ready-to-use binary version of `pkg-config` can be obtained from [tools_windows_pkgconfig](https://github.com/staticlibs/tools_windows_pkgconfig) repository.
See [StaticlibsPkgConfig](https://github.com/staticlibs/wiki/wiki/StaticlibsPkgConfig) for Staticlibs-specific details about `pkg-config` usage.

[Perl](https://www.perl.org/) is also required for building, Windows users can obtain ready-to-use
Perl distribution from [tools_windows_perl](https://github.com/staticlibs/tools_windows_perl) repository.

[NASM](http://nasm.us/) is also required for building on Windows x86 
(optional on Windows x86_64 - will be used if present in `PATH`).
You can obtain ready-to-use NASM distribution from 
[tools_windows_nasm](https://github.com/staticlibs/tools_windows_nasm) repository.

To build the library on Windows using Visual Studio 2013 Express run the following commands using
Visual Studio development command prompt 
(`C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\Shortcuts\VS2013 x86 Native Tools Command Prompt`):

    git clone --recursive https://github.com/staticlibs/external_zlib.git
    git clone --recursive https://github.com/staticlibs/external_openssl.git
    git clone --recursive https://github.com/staticlibs/external_curl.git
    git clone https://github.com/staticlibs/staticlib_config.git
    git clone https://github.com/staticlibs/staticlib_support.git
    git clone https://github.com/staticlibs/staticlib_concurrent.git
    git clone https://github.com/staticlibs/staticlib_io.git
    git clone https://github.com/staticlibs/staticlib_http.git
    cd staticlib_http
    mkdir build
    cd build
    cmake ..
    msbuild staticlib_http.sln

Cloning of [external_zlib](https://github.com/staticlibs/external_zlib),
[external_openssl](https://github.com/staticlibs/external_openssl) and
[external_curl](https://github.com/staticlibs/external_curl) is not required on Linux - 
system libraries will be used instead.

See [StaticlibsToolchains](https://github.com/staticlibs/wiki/wiki/StaticlibsToolchains) for 
more information about the toolchain setup and cross-compilation.

License information
-------------------

This project is released under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0).

Changelog
---------

**2018-02-28**

 * version 1.3.2
 * use bundled OpenSSL on mac

**2017-12-31**

 * version 1.3.1
 * vs2017 support

**2017-04-10**

 * version 1.3.0
 * classes renamed

**2016-06-29**

 * version 1.2
 * make `http_session` thread-safe

**2016-06-29**

 * version 1.1
 * support for `HTTP/1.0` requests
 * ICU API removed

**2016-01-12**

 * version 1.0

**2016-01-26**

 * initial public version
