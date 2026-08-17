#ifndef METRIX_ARIA2_H
#define METRIX_ARIA2_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#if defined(METRIX_ARIA2_BUILD)
#define METRIX_ARIA2_API __declspec(dllexport)
#else
#define METRIX_ARIA2_API __declspec(dllimport)
#endif
#else
#define METRIX_ARIA2_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t mx_aria2_gid;

typedef struct mx_aria2_session mx_aria2_session;
typedef struct mx_aria2_download_handle mx_aria2_download_handle;

typedef enum mx_aria2_run_mode {
    MX_ARIA2_RUN_DEFAULT = 0,
    MX_ARIA2_RUN_ONCE = 1
} mx_aria2_run_mode;

typedef enum mx_aria2_download_event {
    MX_ARIA2_EVENT_ON_DOWNLOAD_START = 1,
    MX_ARIA2_EVENT_ON_DOWNLOAD_PAUSE = 2,
    MX_ARIA2_EVENT_ON_DOWNLOAD_STOP = 3,
    MX_ARIA2_EVENT_ON_DOWNLOAD_COMPLETE = 4,
    MX_ARIA2_EVENT_ON_DOWNLOAD_ERROR = 5,
    MX_ARIA2_EVENT_ON_BT_DOWNLOAD_COMPLETE = 6
} mx_aria2_download_event;

typedef enum mx_aria2_download_status {
    MX_ARIA2_DOWNLOAD_ACTIVE = 0,
    MX_ARIA2_DOWNLOAD_WAITING = 1,
    MX_ARIA2_DOWNLOAD_PAUSED = 2,
    MX_ARIA2_DOWNLOAD_COMPLETE = 3,
    MX_ARIA2_DOWNLOAD_ERROR = 4,
    MX_ARIA2_DOWNLOAD_REMOVED = 5
} mx_aria2_download_status;

typedef struct mx_aria2_key_value {
    const char* key;
    const char* value;
} mx_aria2_key_value;

typedef int (*mx_aria2_download_event_callback)(
    mx_aria2_session* session,
    mx_aria2_download_event event,
    mx_aria2_gid gid,
    void* user_data);

typedef struct mx_aria2_session_config {
    int keep_running;
    int use_signal_handler;
    mx_aria2_download_event_callback download_event_callback;
    void* user_data;
} mx_aria2_session_config;

typedef struct mx_aria2_global_stat {
    int download_speed;
    int upload_speed;
    int num_active;
    int num_waiting;
    int num_stopped;
} mx_aria2_global_stat;

typedef struct mx_aria2_download_stat {
    mx_aria2_download_status status;
    int64_t total_length;
    int64_t completed_length;
    int64_t upload_length;
    int download_speed;
    int upload_speed;
    int connections;
    int error_code;
} mx_aria2_download_stat;

METRIX_ARIA2_API int mx_aria2_library_init(void);
METRIX_ARIA2_API int mx_aria2_library_deinit(void);

METRIX_ARIA2_API void mx_aria2_session_config_init(
    mx_aria2_session_config* config);

METRIX_ARIA2_API int mx_aria2_session_new(
    const mx_aria2_key_value* options,
    size_t option_count,
    const mx_aria2_session_config* config,
    mx_aria2_session** session);

METRIX_ARIA2_API int mx_aria2_session_final(mx_aria2_session* session);

METRIX_ARIA2_API int mx_aria2_run(
    mx_aria2_session* session,
    mx_aria2_run_mode mode);

METRIX_ARIA2_API int mx_aria2_shutdown(
    mx_aria2_session* session,
    int force);

METRIX_ARIA2_API int mx_aria2_add_uri(
    mx_aria2_session* session,
    mx_aria2_gid* gid,
    const char* const* uris,
    size_t uri_count,
    const mx_aria2_key_value* options,
    size_t option_count,
    int position);

METRIX_ARIA2_API int mx_aria2_gid_to_hex(
    mx_aria2_gid gid,
    char* buffer,
    size_t buffer_size);

METRIX_ARIA2_API mx_aria2_gid mx_aria2_hex_to_gid(const char* hex);

METRIX_ARIA2_API int mx_aria2_is_null_gid(mx_aria2_gid gid);

METRIX_ARIA2_API int mx_aria2_get_active_downloads(
    mx_aria2_session* session,
    mx_aria2_gid* gids,
    size_t capacity,
    size_t* count);

METRIX_ARIA2_API int mx_aria2_remove_download(
    mx_aria2_session* session,
    mx_aria2_gid gid,
    int force);

METRIX_ARIA2_API int mx_aria2_pause_download(
    mx_aria2_session* session,
    mx_aria2_gid gid,
    int force);

METRIX_ARIA2_API int mx_aria2_unpause_download(
    mx_aria2_session* session,
    mx_aria2_gid gid);

METRIX_ARIA2_API int mx_aria2_change_global_option(
    mx_aria2_session* session,
    const mx_aria2_key_value* options,
    size_t option_count);

METRIX_ARIA2_API int mx_aria2_change_option(
    mx_aria2_session* session,
    mx_aria2_gid gid,
    const mx_aria2_key_value* options,
    size_t option_count);

METRIX_ARIA2_API int mx_aria2_get_global_stat(
    mx_aria2_session* session,
    mx_aria2_global_stat* stat);

METRIX_ARIA2_API mx_aria2_download_handle* mx_aria2_get_download_handle(
    mx_aria2_session* session,
    mx_aria2_gid gid);

METRIX_ARIA2_API void mx_aria2_delete_download_handle(
    mx_aria2_download_handle* handle);

METRIX_ARIA2_API int mx_aria2_get_download_stat(
    mx_aria2_download_handle* handle,
    mx_aria2_download_stat* stat);

#ifdef __cplusplus
}
#endif

#endif
