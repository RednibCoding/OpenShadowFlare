#ifndef OPENSHADOWFLARE_GAMEPLAY_TRANSPORT_RENDERER_HPP
#define OPENSHADOWFLARE_GAMEPLAY_TRANSPORT_RENDERER_HPP

namespace osf {

class GameplayTransport;
class TransportCatalog;

namespace gapi {
class Backend;
class NjpImage;
}

void renderGameplayTransport(
    gapi::Backend& renderer,
    const gapi::NjpImage& status_patterns,
    const gapi::NjpImage& font,
    const GameplayTransport& transport,
    const TransportCatalog& catalog);

}  // namespace osf

#endif
