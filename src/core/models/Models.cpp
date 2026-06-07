#include "Models.hpp"

QString DiscordUser::displayName() const {
  if (!globalName.isEmpty()) {
    return globalName;
  }

  if (!username.isEmpty()) {
    return username;
  }

  return id;
}

bool DiscordMessage::isEdited() const { return !editedTimestamp.isEmpty(); }
