#ifndef SCANME_QR_H
#define SCANME_QR_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
  #ifdef SCANME_QR_EXPORTS
    #define SCANME_QR_API __declspec(dllexport)
  #else
    #define SCANME_QR_API __declspec(dllimport)
  #endif
#elif defined(SCANME_QR_NO_EXPORT)
  /* The php extension compiles this core into itself rather than linking
     libscanme_qr, and PHP dlopens extensions into the global namespace: leaving
     the C API at default visibility would let it interpose on the identically
     named entry points in libscanme_qr.so, so a process using both the
     extension and FFI would run one library's code against the other's. */
  #define SCANME_QR_API
#else
  #define SCANME_QR_API __attribute__((visibility("default")))
#endif

typedef struct {
    uint8_t* modules;
    int      size;
    int      version;
} scanme_qr_result_t;

/* New matrix-based result for better integration */
typedef struct {
    int version;
    int size;
    /* Packed bits: 1 byte per module (0 or 1) for simplicity in PHP,
       or we could use packed bits. Let's stick to uint8_t for now
       as it's easiest to convert to PHP array or string. */
    uint8_t* data;
} scanme_qr_matrix_t;

SCANME_QR_API int scanme_qr_encode(
    const char*         data,
    size_t              len,
    int                 ecl,
    scanme_qr_result_t* out
);

SCANME_QR_API void scanme_qr_result_free(scanme_qr_result_t* out);

/* Improved API */
SCANME_QR_API scanme_qr_matrix_t* scanme_qr_encode_matrix(
    const char* data,
    size_t      len,
    int         ecl
);

SCANME_QR_API void scanme_qr_matrix_free(scanme_qr_matrix_t* matrix);

SCANME_QR_API const char* scanme_qr_version(void);

#ifdef __cplusplus
}
#endif

#endif /* SCANME_QR_H */
