#pragma once

#ifdef WIN32
#ifdef ONSCLIENT_EXPORTS

#ifndef SWIG
#define ONSCLIENT_API __declspec(dllexport)
#else
#define ONSCLIENT_API
#endif

#else
#define ONSCLIENT_API __declspec(dllimport)
#endif
#else
#define ONSCLIENT_API
#endif

#ifndef ONS_NAMESPACE
#define ONS_NAMESPACE ons
#endif


#ifndef ONS_NAMESPACE_BEGIN
#define ONS_NAMESPACE_BEGIN namespace ONS_NAMESPACE {
#endif

#ifndef ONS_NAMESPACE_END
#define ONS_NAMESPACE_END }
#endif