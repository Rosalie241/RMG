#ifndef QTKEYTOSDL2KEY_HPP
#define QTKEYTOSDL2KEY_HPP

#include <QObject>

namespace Utilities
{
int QtKeyToSdl2Key(int);
int QtModKeyToSdl2ModKey(Qt::KeyboardModifiers);
} // namespace Utilities

#endif // QTKEYTOSDL2KEY_HPP
