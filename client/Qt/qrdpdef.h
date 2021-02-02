#ifndef QRDPDEF_H
#define QRDPDEF_H

#ifdef QRDP_LIBRARY
    #define QRDP_EXPORT Q_DECL_EXPORT
#else
    #define QRDP_EXPORT Q_DECL_IMPORT
#endif

#endif // QRDPDEF_H