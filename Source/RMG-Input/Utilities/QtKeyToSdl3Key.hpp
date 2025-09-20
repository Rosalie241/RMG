#ifndef QTKEYTOSDL3KEY_HPP
#define QTKEYTOSDL3KEY_HPP

#include <QObject>

namespace Utilities
{
int QtKeyToSdl3Key(int);
int QtModKeyToSdl3ModKey(Qt::KeyboardModifiers);
} // namespace Utilities

#endif // QTKEYTOSDL3KEY_HPP
