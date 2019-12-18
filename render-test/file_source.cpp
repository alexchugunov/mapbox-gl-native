#include <mbgl/storage/resource_options.hpp>

#include "file_source.hpp"

namespace mbgl {

std::atomic_size_t requestCount{0};
std::atomic_size_t transferredSize{0};
std::atomic_bool active{false};

ProxyFileSource::ProxyFileSource(const std::string& cachePath, const std::string& assetPath)
    : DefaultFileSource(cachePath, assetPath, false) {}

ProxyFileSource::~ProxyFileSource() = default;

std::unique_ptr<AsyncRequest> ProxyFileSource::request(const Resource& resource, Callback callback) {
    auto transformed = resource;

    // This is needed for compatibility with the style tests that
    // are using local:// instead of file:// which is the schema
    // we support.
    if (transformed.url.compare(0, 8, "local://") == 0) {
        transformed.url.replace(0, 8, "http://");

        if (transformed.kind == Resource::Kind::Tile && transformed.tileData) {
            transformed.tileData->urlTemplate.replace(0, 8, "http://");
        }
    }

    if (transformed.url.compare(0, 22, "http://localhost:2900/") == 0) {
        transformed.url.replace(0, 22, "http://");

        if (transformed.kind == Resource::Kind::Tile && transformed.tileData) {
            transformed.tileData->urlTemplate.replace(0, 22, "http://");
        }
    }

    transformed.loadingMethod = Resource::LoadingMethod::CacheOnly;

    return DefaultFileSource::request(transformed, [=](Response response) mutable {
        std::size_t size = response.data != nullptr ? response.data->size() : 0;
        if (active) {
            requestCount++;
            transferredSize += size;
        }
        callback(response);
    });
}

std::shared_ptr<FileSource> FileSource::createPlatformFileSource(const ResourceOptions& options) {
    auto fileSource = std::make_shared<ProxyFileSource>(options.cachePath(), options.assetPath());
    fileSource->setAccessToken(options.accessToken());
    fileSource->setAPIBaseURL(options.baseURL());
    return fileSource;
}

// static
void ProxyFileSource::setTrackingActive(bool active_) {
    active = active_;
    requestCount = 0;
    transferredSize = 0;
}

// static
bool ProxyFileSource::isTrackingActive() {
    return active;
}

// static
size_t ProxyFileSource::getRequestCount() {
    return requestCount;
}

// static
size_t ProxyFileSource::getTransferredSize() {
    return transferredSize;
}

} // namespace mbgl
