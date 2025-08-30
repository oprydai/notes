#pragma once

#include <Qt>

namespace Roles {
    enum : int {
        NoteSnippetRole = Qt::UserRole + 1,
        NoteDateRole    = Qt::UserRole + 2,
        NotePinnedRole  = Qt::UserRole + 3,
        NoteContentRole = Qt::UserRole + 4
    };
}


