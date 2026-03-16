#ifndef GAR_EXPORTS_H
#define GAR_EXPORTS_H

#ifdef _WIN32
#ifdef gar_EXPORTS
#define GAR_API __declspec(dllexport)
#else
#define GAR_API __declspec(dllimport)
#endif
#else
#define GAR_API
#endif

#endif
