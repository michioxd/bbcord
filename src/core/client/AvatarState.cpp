#include "AvatarState.hpp"

AvatarState::AvatarState() { reset(); }

void AvatarState::reset() {
  pendingAvatars.clear();
  pendingGuildIcons.clear();
  avatarCacheRequests.clear();
  guildIconCacheRequests.clear();
  avatarSourcesByUserId.clear();
  loadingAvatarUserId.clear();
  loadingAvatarUserId2.clear();
  loadingGuildIconId.clear();
  loadingGuildIconId2.clear();
  queuedAvatarUserIds.clear();
  loadedAvatarUserIds.clear();
  queuedGuildIconIds.clear();
  loadedGuildIconIds.clear();
}
