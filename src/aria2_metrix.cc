#include <aria2/aria2_metrix.h>

#include <aria2/aria2.h>

#include <mutex>
#include <cstring>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct mx_aria2_session {
    aria2::Session* native;
    mx_aria2_download_event_callback callback;
    void* user_data;
};

struct mx_aria2_download_handle {
    aria2::DownloadHandle* native;
};

namespace {
std::mutex sessions_mutex;
std::unordered_map<aria2::Session*, mx_aria2_session*> sessions;

aria2::KeyVals to_key_values(const mx_aria2_key_value* options, size_t count)
{
    aria2::KeyVals values;
    if (options == nullptr) {
        return values;
    }

    values.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        if (options[index].key == nullptr || options[index].value == nullptr) {
            continue;
        }

        values.emplace_back(options[index].key, options[index].value);
    }

    return values;
}

std::vector<std::string> to_strings(const char* const* values, size_t count)
{
    std::vector<std::string> strings;
    if (values == nullptr) {
        return strings;
    }

    strings.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        if (values[index] != nullptr) {
            strings.emplace_back(values[index]);
        }
    }

    return strings;
}

int download_event_trampoline(
    aria2::Session* native_session,
    aria2::DownloadEvent event,
    aria2::A2Gid gid,
    void*)
{
    mx_aria2_session* session = nullptr;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        const auto match = sessions.find(native_session);
        if (match != sessions.end()) {
            session = match->second;
        }
    }

    if (session == nullptr || session->callback == nullptr) {
        return 0;
    }

    return session->callback(
        session,
        static_cast<mx_aria2_download_event>(event),
        gid,
        session->user_data);
}

aria2::RUN_MODE to_run_mode(mx_aria2_run_mode mode)
{
    return mode == MX_ARIA2_RUN_ONCE ? aria2::RUN_ONCE : aria2::RUN_DEFAULT;
}

mx_aria2_download_status to_download_status(aria2::DownloadStatus status)
{
    return static_cast<mx_aria2_download_status>(status);
}
}

extern "C" {

int mx_aria2_library_init(void)
{
    return aria2::libraryInit();
}

int mx_aria2_library_deinit(void)
{
    return aria2::libraryDeinit();
}

void mx_aria2_session_config_init(mx_aria2_session_config* config)
{
    if (config == nullptr) {
        return;
    }

    config->keep_running = 0;
    config->use_signal_handler = 1;
    config->download_event_callback = nullptr;
    config->user_data = nullptr;
}

int mx_aria2_session_new(
    const mx_aria2_key_value* options,
    size_t option_count,
    const mx_aria2_session_config* config,
    mx_aria2_session** session)
{
    if (session == nullptr) {
        return -1;
    }

    *session = nullptr;

    auto wrapper = new mx_aria2_session{};
    wrapper->callback = config != nullptr ? config->download_event_callback : nullptr;
    wrapper->user_data = config != nullptr ? config->user_data : nullptr;

    aria2::SessionConfig native_config;
    if (config != nullptr) {
        native_config.keepRunning = config->keep_running != 0;
        native_config.useSignalHandler = config->use_signal_handler != 0;
    }

    if (wrapper->callback != nullptr) {
        native_config.downloadEventCallback = download_event_trampoline;
        native_config.userData = wrapper;
    }

    wrapper->native = aria2::sessionNew(to_key_values(options, option_count), native_config);
    if (wrapper->native == nullptr) {
        delete wrapper;
        return -2;
    }

    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        sessions.emplace(wrapper->native, wrapper);
    }

    *session = wrapper;
    return 0;
}

int mx_aria2_session_final(mx_aria2_session* session)
{
    if (session == nullptr) {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lock(sessions_mutex);
        sessions.erase(session->native);
    }

    const int result = aria2::sessionFinal(session->native);
    delete session;
    return result;
}

int mx_aria2_run(mx_aria2_session* session, mx_aria2_run_mode mode)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::run(session->native, to_run_mode(mode));
}

int mx_aria2_shutdown(mx_aria2_session* session, int force)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::shutdown(session->native, force != 0);
}

int mx_aria2_add_uri(
    mx_aria2_session* session,
    mx_aria2_gid* gid,
    const char* const* uris,
    size_t uri_count,
    const mx_aria2_key_value* options,
    size_t option_count,
    int position)
{
    if (session == nullptr || uris == nullptr || uri_count == 0) {
        return -1;
    }

    aria2::A2Gid native_gid = 0;
    const int result = aria2::addUri(
        session->native,
        &native_gid,
        to_strings(uris, uri_count),
        to_key_values(options, option_count),
        position);

    if (gid != nullptr) {
        *gid = native_gid;
    }

    return result;
}

int mx_aria2_gid_to_hex(mx_aria2_gid gid, char* buffer, size_t buffer_size)
{
    if (buffer == nullptr || buffer_size == 0) {
        return -1;
    }

    const std::string hex = aria2::gidToHex(gid);
    if (hex.size() + 1 > buffer_size) {
        return -2;
    }

    std::memcpy(buffer, hex.c_str(), hex.size() + 1);
    return 0;
}

mx_aria2_gid mx_aria2_hex_to_gid(const char* hex)
{
    if (hex == nullptr) {
        return 0;
    }

    return aria2::hexToGid(hex);
}

int mx_aria2_is_null_gid(mx_aria2_gid gid)
{
    return aria2::isNull(gid) ? 1 : 0;
}

int mx_aria2_get_active_downloads(
    mx_aria2_session* session,
    mx_aria2_gid* gids,
    size_t capacity,
    size_t* count)
{
    if (session == nullptr || count == nullptr) {
        return -1;
    }

    const std::vector<aria2::A2Gid> active = aria2::getActiveDownload(session->native);
    *count = active.size();
    if (gids == nullptr || capacity == 0) {
        return 0;
    }

    const size_t copy_count = active.size() < capacity ? active.size() : capacity;
    for (size_t index = 0; index < copy_count; ++index) {
        gids[index] = active[index];
    }

    return copy_count == active.size() ? 0 : -2;
}

int mx_aria2_remove_download(mx_aria2_session* session, mx_aria2_gid gid, int force)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::removeDownload(session->native, gid, force != 0);
}

int mx_aria2_pause_download(mx_aria2_session* session, mx_aria2_gid gid, int force)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::pauseDownload(session->native, gid, force != 0);
}

int mx_aria2_unpause_download(mx_aria2_session* session, mx_aria2_gid gid)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::unpauseDownload(session->native, gid);
}

int mx_aria2_change_global_option(
    mx_aria2_session* session,
    const mx_aria2_key_value* options,
    size_t option_count)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::changeGlobalOption(session->native, to_key_values(options, option_count));
}

int mx_aria2_change_option(
    mx_aria2_session* session,
    mx_aria2_gid gid,
    const mx_aria2_key_value* options,
    size_t option_count)
{
    if (session == nullptr) {
        return -1;
    }

    return aria2::changeOption(session->native, gid, to_key_values(options, option_count));
}

int mx_aria2_get_global_stat(mx_aria2_session* session, mx_aria2_global_stat* stat)
{
    if (session == nullptr || stat == nullptr) {
        return -1;
    }

    const aria2::GlobalStat native_stat = aria2::getGlobalStat(session->native);
    stat->download_speed = native_stat.downloadSpeed;
    stat->upload_speed = native_stat.uploadSpeed;
    stat->num_active = native_stat.numActive;
    stat->num_waiting = native_stat.numWaiting;
    stat->num_stopped = native_stat.numStopped;
    return 0;
}

mx_aria2_download_handle* mx_aria2_get_download_handle(
    mx_aria2_session* session,
    mx_aria2_gid gid)
{
    if (session == nullptr) {
        return nullptr;
    }

    aria2::DownloadHandle* native_handle = aria2::getDownloadHandle(session->native, gid);
    if (native_handle == nullptr) {
        return nullptr;
    }

    return new mx_aria2_download_handle{native_handle};
}

void mx_aria2_delete_download_handle(mx_aria2_download_handle* handle)
{
    if (handle == nullptr) {
        return;
    }

    aria2::deleteDownloadHandle(handle->native);
    delete handle;
}

int mx_aria2_get_download_stat(
    mx_aria2_download_handle* handle,
    mx_aria2_download_stat* stat)
{
    if (handle == nullptr || stat == nullptr) {
        return -1;
    }

    stat->status = to_download_status(handle->native->getStatus());
    stat->total_length = handle->native->getTotalLength();
    stat->completed_length = handle->native->getCompletedLength();
    stat->upload_length = handle->native->getUploadLength();
    stat->download_speed = handle->native->getDownloadSpeed();
    stat->upload_speed = handle->native->getUploadSpeed();
    stat->connections = handle->native->getConnections();
    stat->error_code = handle->native->getErrorCode();
    return 0;
}

}
