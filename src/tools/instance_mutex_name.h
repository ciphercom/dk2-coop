#ifndef FLAME_INSTANCE_MUTEX_NAME_H
#define FLAME_INSTANCE_MUTEX_NAME_H

#include <string>

namespace patch::instance_mutex {

/** Return the stable mutex name that identifies an executable's installation directory. */
std::string nameForExecutablePath(std::wstring executablePath);

/** Resolve the running executable and return its installation-scoped mutex name. */
std::string currentName();

}

#endif //FLAME_INSTANCE_MUTEX_NAME_H
